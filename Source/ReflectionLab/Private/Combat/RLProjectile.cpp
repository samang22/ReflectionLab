#include "Combat/RLProjectile.h"

#include "Combat/RLExplosionVisual.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Combat/RLVFXPoolSubsystem.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Enemies/RLEnemyCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundConcurrency.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float GlobalProjectileLifeSeconds = 999.0f;
	const FName ProjectileColorParameterName(TEXT("ProjectileColor"));
	const FName EmissiveIntensityParameterName(TEXT("EmissiveIntensity"));
	const FName FadeOpacityParameterName(TEXT("FadeOpacity"));
}

ARLProjectile::ARLProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(16.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetCanEverAffectNavigation(false);
	CollisionComponent->OnComponentHit.AddDynamic(this, &ThisClass::HandleProjectileHit);
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(
		this,
		&ThisClass::HandleProjectileOverlap);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetCanEverAffectNavigation(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bAutoActivate = false;

	ReflectedTrailComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ReflectedTrailComponent"));
	ReflectedTrailComponent->SetupAttachment(CollisionComponent);
	ReflectedTrailComponent->SetAutoActivate(false);
	ReflectedTrailComponent->SetAutoDestroy(false);

	static ConstructorHelpers::FObjectFinder<USoundBase> ExplosionSoundFinder(
		TEXT("/Game/ReflectionLab/Audio/SFX/Combat/Explosion/"
			 "SFX_ExplosiveDetonation.SFX_ExplosiveDetonation"));
	if (ExplosionSoundFinder.Succeeded())
	{
		ExplosionSound = ExplosionSoundFinder.Object;
	}
}

void ARLProjectile::BeginPlay()
{
	Super::BeginPlay();

	static USoundConcurrency* SharedExplosionSoundConcurrency = nullptr;
	if (!SharedExplosionSoundConcurrency)
	{
		SharedExplosionSoundConcurrency = NewObject<USoundConcurrency>(
			GetTransientPackage(),
			TEXT("RLExplosionSoundConcurrency"));
		if (SharedExplosionSoundConcurrency)
		{
			SharedExplosionSoundConcurrency->AddToRoot();
			SharedExplosionSoundConcurrency->Concurrency.MaxCount = 3;
			SharedExplosionSoundConcurrency->Concurrency.bLimitToOwner = false;
			SharedExplosionSoundConcurrency->Concurrency.ResolutionRule =
				EMaxConcurrentResolutionRule::StopQuietest;
			SharedExplosionSoundConcurrency->Concurrency.RetriggerTime = 0.06f;
		}
	}
	ExplosionSoundConcurrency = SharedExplosionSoundConcurrency;

	DefaultProjectileMeshScale = ProjectileMesh->GetRelativeScale3D();
	DefaultCollisionRadius = CollisionComponent->GetUnscaledSphereRadius();
	CreateReflectedAfterimages();

	DeactivateForPool();
}

void ARLProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsActive)
	{
		return;
	}

	if (bIsFadingOut)
	{
		UpdateFadeOut(DeltaTime);
		return;
	}

	if (IsOutsidePlayArea())
	{
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("Projectile %s left the play area and was returned to the pool."),
			*GetName());
		ReturnToPool();
		return;
	}

	if (bIsExplosive)
	{
		UpdateExplosive(DeltaTime);
	}

	if (!bIsActive || bIsFadingOut)
	{
		return;
	}

	if (bIsDelayedExplosive)
	{
		UpdateDelayedExplosive(DeltaTime);
	}

	if (bIsFakeProjectile)
	{
		UpdateFake(DeltaTime);
	}

	if (bIsRallyProjectile)
	{
		UpdateRally(DeltaTime);
	}

	if (bIsReflected)
	{
		UpdateReflectedAfterimages(DeltaTime);
	}
}

void ARLProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void ARLProjectile::ActivateProjectile(
	const FTransform& SpawnTransform,
	AActor* NewOwner,
	APawn* NewInstigator)
{
	LifeSeconds = GlobalProjectileLifeSeconds;
	SetActorTickEnabled(false);
	ResetReflectedAfterimages();
	bIsReflected = false;
	bIsFadingOut = false;
	bCanBeReflected = true;
	bIsExplosive = false;
	bExplosiveBlinkWarning = false;
	bIsDelayedExplosive = false;
	bDelayedExplosivePaused = false;
	bDelayedExplosiveResumed = false;
	bIsFakeProjectile = false;
	bFakeDormant = false;
	bExplodesOnEnemyImpact = false;
	bIsGuardProjectile = false;
	bSplitsOnParry = false;
	bIsRallyProjectile = false;
	bRallyFinalShot = false;
	RemainingPierces = 0;
	ReflectionChain = 0;
	bWasPerfectParried = false;
	bWasCloseRangeParried = false;
	bWasOverdriveReflected = false;
	ExplosiveElapsedTime = 0.0f;
	ExplosiveNextBlinkTime = 0.0f;
	DelayedExplosivePauseElapsedTime = 0.0f;
	FakeDormantElapsedTime = 0.0f;
	RallyCurrentSpeed = ProjectileSpeed;
	RallySpeedMultiplierPerRally = 1.15f;
	RallyCount = 0;
	MaxRallies = 0;
	RallyTarget.Reset();
	RallyFinalTarget.Reset();
	DelayedExplosiveTarget.Reset();
	FakeTarget.Reset();
	ExplosiveMaterialInstance = nullptr;
	FadeMaterialInstance = nullptr;
	SpecialMaterialInstance = nullptr;
	DeactivateReflectedTrail();
	ReflectedTrailVFX = nullptr;
	ExplosionVFX = nullptr;
	ReflectedTrailScale = 1.0f;
	ExplosionVFXScale = 1.0f;
	ActiveDefinition = nullptr;
	FadeOutElapsedTime = 0.0f;
	ProjectileMesh->SetRelativeScale3D(DefaultProjectileMeshScale);
	ProjectileMesh->SetVisibility(true, true);
	CollisionComponent->SetSphereRadius(DefaultCollisionRadius, false);
	UpdateProjectileMaterial();
	SetOwner(NewOwner);
	SetInstigator(NewInstigator);
	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);

	CollisionComponent->ClearMoveIgnoreActors();
	if (NewOwner)
	{
		CollisionComponent->IgnoreActorWhenMoving(NewOwner, true);

		if (UPrimitiveComponent* OwnerRootComponent =
			Cast<UPrimitiveComponent>(NewOwner->GetRootComponent()))
		{
			OwnerRootComponent->IgnoreActorWhenMoving(this, true);
		}
	}

	SetActorHiddenInGame(false);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->Activate(true);
	ProjectileMovement->Velocity =
		GetActorForwardVector() * ProjectileSpeed;
	ProjectileMovement->UpdateComponentVelocity();

	bIsActive = true;
	SetActorTickEnabled(true);
	GetWorldTimerManager().SetTimer(
		LifetimeTimerHandle,
		this,
		&ThisClass::ReturnToPool,
		FMath::Max(0.1f, LifeSeconds),
		false);
}

void ARLProjectile::InitializeFromDefinition(
	URLProjectileDefinitionDataAsset* Definition,
	AActor* TargetActor,
	bool bConfigureBehavior)
{
	if (!bIsActive || !Definition)
	{
		return;
	}

	ActiveDefinition = Definition;
	ApplyDefinitionStats(*Definition);

	if (!bConfigureBehavior)
	{
		return;
	}

	switch (Definition->Behavior)
	{
	case ERLProjectileBehavior::Explosive:
		ConfigureAsExplosive();
		break;
	case ERLProjectileBehavior::DelayedExplosive:
		ConfigureAsDelayedExplosive(TargetActor);
		break;
	case ERLProjectileBehavior::Fake:
		ConfigureAsFake(TargetActor);
		break;
	case ERLProjectileBehavior::Guard:
		ConfigureAsGuard();
		break;
	case ERLProjectileBehavior::ParrySplit:
		ConfigureAsParrySplit();
		break;
	case ERLProjectileBehavior::Rally:
		ConfigureAsRally(
			TargetActor,
			Definition->RallyRelayCount,
			Definition->RallySpeedMultiplierPerRelay);
		break;
	case ERLProjectileBehavior::Normal:
	default:
		break;
	}
}

void ARLProjectile::ApplyDefinitionStats(
	const URLProjectileDefinitionDataAsset& Definition)
{
	DamageAmount = FMath::Max(0.0f, Definition.DamageAmount);
	ProjectileSpeed = FMath::Max(1.0f, Definition.ProjectileSpeed);
	ReflectedSpeedMultiplier = FMath::Max(1.0f, Definition.ReflectedSpeedMultiplier);
	// Projectile definitions share one long safety lifetime. Escaped projectiles
	// are recycled by the play-area bounds check instead of short per-type timers.
	LifeSeconds = GlobalProjectileLifeSeconds;
	FadeOutDuration = FMath::Max(0.0f, Definition.FadeOutDuration);
	HostileMaterial = Definition.HostileMaterial;
	ReflectedMaterial = Definition.ReflectedMaterial;
	ReflectedTrailVFX = Definition.ReflectedTrailVFX;
	ReflectedTrailScale = FMath::Clamp(Definition.ReflectedTrailScale, 0.1f, 5.0f);
	bExplodesOnEnemyImpact = Definition.bExplodesOnEnemyImpact;

	ExplosiveSpeedMultiplier = FMath::Clamp(Definition.ExplosiveSpeedMultiplier, 0.1f, 1.0f);
	ExplosiveVisualScale = FMath::Max(1.0f, Definition.ExplosiveVisualScale);
	ExplosiveFuseDuration = FMath::Max(0.1f, Definition.ExplosiveFuseDuration);
	const URLProjectileDefinitionDataAsset& ExplosionDefinition =
		Definition.EnemyImpactExplosionDefinition
			? *Definition.EnemyImpactExplosionDefinition
			: Definition;
	ExplosionRadius = FMath::Max(1.0f, ExplosionDefinition.ExplosionRadius);
	ExplosionDamage = FMath::Max(0.0f, ExplosionDefinition.ExplosionDamage);
	ExplosionSound = ExplosionDefinition.ExplosionSound;
	ExplosionSoundVolume = FMath::Clamp(
		ExplosionDefinition.ExplosionSoundVolume,
		0.0f,
		2.0f);
	ExplosionVFX = ExplosionDefinition.ExplosionVFX;
	ExplosionVFXScale = FMath::Clamp(ExplosionDefinition.ExplosionVFXScale, 0.1f, 5.0f);
	ExplosiveBlinkStartInterval = FMath::Max(0.01f, Definition.ExplosiveBlinkStartInterval);
	ExplosiveBlinkEndInterval = FMath::Max(0.01f, Definition.ExplosiveBlinkEndInterval);
	ExplosiveBaseColor = Definition.ExplosiveBaseColor;
	ExplosiveDecalColor = ExplosionDefinition.ExplosiveDecalColor;
	ExplosiveWarningColor = Definition.ExplosiveWarningColor;
	ExplosiveBaseEmissiveIntensity = FMath::Max(0.0f, Definition.ExplosiveBaseEmissiveIntensity);
	ExplosiveWarningEmissiveIntensity = FMath::Max(0.0f, Definition.ExplosiveWarningEmissiveIntensity);

	DelayedExplosiveTriggerDistance = FMath::Max(1.0f, Definition.DelayedExplosiveTriggerDistance);
	DelayedExplosivePauseDuration = FMath::Max(0.0f, Definition.DelayedExplosivePauseDuration);
	DelayedExplosiveResumeSpeedMultiplier = FMath::Max(0.1f, Definition.DelayedExplosiveResumeSpeedMultiplier);
	FakeTriggerDistance = FMath::Max(1.0f, Definition.FakeTriggerDistance);
	FakeRevealDelay = FMath::Max(0.0f, Definition.FakeRevealDelay);
	FakeRealSpeedMultiplier = FMath::Max(0.1f, Definition.FakeRealSpeedMultiplier);
	FakeColor = Definition.FakeColor;
	FakeOpacity = FMath::Clamp(Definition.FakeOpacity, 0.0f, 1.0f);
	GuardColor = Definition.GuardColor;
	ParrySplitColor = Definition.ParrySplitColor;
	ParrySplitFragmentCount = FMath::Clamp(Definition.ParrySplitFragmentCount, 1, 6);
	ParrySplitSpreadAngle = FMath::Clamp(Definition.ParrySplitSpreadAngle, 0.0f, 180.0f);
	ParrySplitFragmentSpeedMultiplier = FMath::Max(0.1f, Definition.ParrySplitFragmentSpeedMultiplier);
	ParrySplitFragmentScale = FMath::Clamp(Definition.ParrySplitFragmentScale, 0.1f, 1.0f);
	RallyVisualScale = FMath::Max(1.0f, Definition.RallyVisualScale);
	RallyMaxSpeedMultiplier = FMath::Max(1.0f, Definition.RallyMaxSpeedMultiplier);
	RallyArrivalRadius = FMath::Max(1.0f, Definition.RallyArrivalRadius);

	const float VisualScale = FMath::Max(0.1f, Definition.VisualScale);
	ProjectileMesh->SetRelativeScale3D(DefaultProjectileMeshScale * VisualScale);
	CollisionComponent->SetSphereRadius(DefaultCollisionRadius * VisualScale, false);
	UpdateProjectileMaterial();
	SetProjectileSpeed(ProjectileSpeed, GetActorForwardVector());
	GetWorldTimerManager().SetTimer(
		LifetimeTimerHandle,
		this,
		&ThisClass::ReturnToPool,
		LifeSeconds,
		false);
}

bool ARLProjectile::Reflect(
	AActor* NewOwner,
	APawn* NewInstigator,
	const FVector& NewDirection,
	const FRLProjectileReflectionParams& ReflectionParams)
{
	if (!bIsActive || !bCanBeReflected || !NewOwner || !NewInstigator || NewDirection.IsNearlyZero())
	{
		return false;
	}
	AActor* OriginalOwner = GetOwner();
	APawn* OriginalInstigator = GetInstigator();
	const bool bShouldSpawnParryFragments = bSplitsOnParry;

	if (AActor* PreviousOwner = GetOwner())
	{
		if (UPrimitiveComponent* PreviousOwnerRoot =
			Cast<UPrimitiveComponent>(PreviousOwner->GetRootComponent()))
		{
			PreviousOwnerRoot->IgnoreActorWhenMoving(this, false);
		}
	}

	SetOwner(NewOwner);
	SetInstigator(NewInstigator);

	CollisionComponent->ClearMoveIgnoreActors();
	CollisionComponent->IgnoreActorWhenMoving(NewOwner, true);
	if (UPrimitiveComponent* NewOwnerRoot =
		Cast<UPrimitiveComponent>(NewOwner->GetRootComponent()))
	{
		NewOwnerRoot->IgnoreActorWhenMoving(this, true);
	}

	const FVector ReflectedDirection = NewDirection.GetSafeNormal();
	const float RequestedSpeedMultiplier = ReflectionParams.SpeedMultiplier > 0.0f
		? ReflectionParams.SpeedMultiplier
		: ReflectedSpeedMultiplier;
	const float ReflectedSpeed = ProjectileSpeed * FMath::Max(1.0f, RequestedSpeedMultiplier);
	SetActorRotation(ReflectedDirection.Rotation());
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->InitialSpeed = ReflectedSpeed;
	ProjectileMovement->MaxSpeed = ReflectedSpeed;
	ProjectileMovement->Velocity = ReflectedDirection * ReflectedSpeed;
	ProjectileMovement->Activate(true);
	ProjectileMovement->UpdateComponentVelocity();
	bIsReflected = true;
	bIsRallyProjectile = false;
	bRallyFinalShot = false;
	RallyTarget.Reset();
	RallyFinalTarget.Reset();
	RemainingPierces = FMath::Max(0, ReflectionParams.PierceCount);
	ReflectionChain = FMath::Max(0, ReflectionParams.ReflectionChain);
	bWasPerfectParried = ReflectionParams.bPerfectParry;
	bWasCloseRangeParried = ReflectionParams.bCloseRangeParry;
	bWasOverdriveReflected = ReflectionParams.bOverdrive;
	bSplitsOnParry = false;
	const float ReflectedScale = FMath::Max(
		0.1f,
		ReflectionParams.VisualScaleMultiplier);
	ProjectileMesh->SetRelativeScale3D(
		DefaultProjectileMeshScale * ReflectedScale);
	CollisionComponent->SetSphereRadius(
		DefaultCollisionRadius * ReflectedScale,
		true);
	UpdateProjectileMaterial();
	ActivateReflectedTrail(ReflectedScale);
	ResetReflectedAfterimages();
	SetActorTickEnabled(true);

	// A reflected projectile starts a fresh lifetime so it has enough time to
	// travel back toward an enemy before being returned to the pool.
	GetWorldTimerManager().SetTimer(
		LifetimeTimerHandle,
		this,
		&ThisClass::ReturnToPool,
		FMath::Max(0.1f, LifeSeconds),
		false);
	if (bShouldSpawnParryFragments)
	{
		SpawnParrySplitFragments(OriginalOwner, OriginalInstigator, NewOwner);
	}

	return true;
}

void ARLProjectile::ConfigureAsExplosive()
{
	if (!bIsActive)
	{
		return;
	}

	bCanBeReflected = false;
	bIsExplosive = true;
	bIsDelayedExplosive = false;
	bIsFakeProjectile = false;
	bIsGuardProjectile = false;
	bSplitsOnParry = false;
	bIsRallyProjectile = false;
	bRallyFinalShot = false;
	RallyTarget.Reset();
	RallyFinalTarget.Reset();
	ExplosiveElapsedTime = 0.0f;
	ExplosiveNextBlinkTime = FMath::Max(0.01f, ExplosiveBlinkStartInterval);
	bExplosiveBlinkWarning = false;
	ProjectileMesh->SetVisibility(true, true);
	ProjectileMesh->SetRelativeScale3D(
		DefaultProjectileMeshScale * FMath::Max(1.0f, ExplosiveVisualScale));
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetSphereRadius(
		DefaultCollisionRadius * FMath::Sqrt(FMath::Max(1.0f, ExplosiveVisualScale)),
		false);
	ExplosiveMaterialInstance = ProjectileMesh->CreateDynamicMaterialInstance(0);
	ApplyExplosiveBlinkColor(false);
	SetProjectileSpeed(
		ProjectileSpeed * FMath::Clamp(ExplosiveSpeedMultiplier, 0.1f, 1.0f),
		GetActorForwardVector());
	GetWorldTimerManager().SetTimer(
		LifetimeTimerHandle,
		this,
		&ThisClass::ReturnToPool,
		FMath::Max(LifeSeconds, ExplosiveFuseDuration + 0.5f),
		false);
	SetActorTickEnabled(true);
}

void ARLProjectile::ConfigureAsDelayedExplosive(AActor* TargetActor)
{
	if (!bIsActive || !IsValid(TargetActor))
	{
		return;
	}

	ConfigureAsExplosive();
	bIsDelayedExplosive = true;
	bDelayedExplosivePaused = false;
	bDelayedExplosiveResumed = false;
	DelayedExplosivePauseElapsedTime = 0.0f;
	DelayedExplosiveTarget = TargetActor;
}

void ARLProjectile::ConfigureAsFake(AActor* TargetActor)
{
	if (!bIsActive || !IsValid(TargetActor))
	{
		return;
	}

	bCanBeReflected = false;
	bIsExplosive = false;
	bIsDelayedExplosive = false;
	bIsFakeProjectile = true;
	bFakeDormant = false;
	bIsGuardProjectile = false;
	bSplitsOnParry = false;
	bIsRallyProjectile = false;
	FakeDormantElapsedTime = 0.0f;
	FakeTarget = TargetActor;
	UpdateProjectileMaterial();
	SetActorTickEnabled(true);
}

void ARLProjectile::ConfigureAsGuard()
{
	if (!bIsActive)
	{
		return;
	}

	bCanBeReflected = true;
	bIsExplosive = false;
	bIsDelayedExplosive = false;
	bIsFakeProjectile = false;
	bIsGuardProjectile = true;
	bSplitsOnParry = false;
	bIsRallyProjectile = false;
	UpdateProjectileMaterial();
}

void ARLProjectile::ConfigureAsParrySplit()
{
	if (!bIsActive)
	{
		return;
	}

	bCanBeReflected = true;
	bIsExplosive = false;
	bIsDelayedExplosive = false;
	bIsFakeProjectile = false;
	bIsGuardProjectile = false;
	bSplitsOnParry = true;
	bIsRallyProjectile = false;
	UpdateProjectileMaterial();
}

bool ARLProjectile::Detonate()
{
	if (!bIsActive || !bIsExplosive)
	{
		return false;
	}

	Explode();
	return true;
}

void ARLProjectile::ConfigureAsRally(
	AActor* FinalTarget,
	int32 InMaxRallies,
	float InSpeedMultiplierPerRally)
{
	if (!bIsActive || !IsValid(FinalTarget))
	{
		return;
	}

	bCanBeReflected = true;
	bIsExplosive = false;
	bIsDelayedExplosive = false;
	bIsFakeProjectile = false;
	bIsGuardProjectile = false;
	bSplitsOnParry = false;
	bIsRallyProjectile = true;
	bRallyFinalShot = false;
	RallyCount = 0;
	MaxRallies = FMath::Max(1, InMaxRallies);
	RallySpeedMultiplierPerRally = FMath::Max(1.0f, InSpeedMultiplierPerRally);
	RallyCurrentSpeed = ProjectileSpeed;
	RallyFinalTarget = FinalTarget;
	ProjectileMesh->SetRelativeScale3D(
		DefaultProjectileMeshScale * FMath::Max(1.0f, RallyVisualScale));

	AActor* FirstRelayTarget = FindNextRallyTarget(GetOwner());
	SetRallyTarget(FirstRelayTarget ? FirstRelayTarget : FinalTarget);
	SetActorTickEnabled(true);
}

void ARLProjectile::ReturnToPool()
{
	if (!bIsActive || bIsFadingOut)
	{
		return;
	}

	bIsFadingOut = true;
	bCanBeReflected = false;
	FadeOutElapsedTime = 0.0f;
	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Deactivate();
	ResetReflectedAfterimages();
	FadeMaterialInstance = ProjectileMesh
		? ProjectileMesh->CreateDynamicMaterialInstance(0)
		: nullptr;
	if (FadeMaterialInstance)
	{
		FadeMaterialInstance->SetScalarParameterValue(FadeOpacityParameterName, 1.0f);
	}
	SetActorTickEnabled(true);

	if (FadeOutDuration <= KINDA_SMALL_NUMBER)
	{
		CompleteReturnToPool();
	}
}

void ARLProjectile::UpdateFadeOut(float DeltaTime)
{
	FadeOutElapsedTime += FMath::Max(0.0f, DeltaTime);
	const float SafeDuration = FMath::Max(KINDA_SMALL_NUMBER, FadeOutDuration);
	const float FadeAlpha = FMath::Clamp(FadeOutElapsedTime / SafeDuration, 0.0f, 1.0f);
	if (FadeMaterialInstance)
	{
		FadeMaterialInstance->SetScalarParameterValue(
			FadeOpacityParameterName,
			1.0f - FadeAlpha);
	}

	if (FadeAlpha >= 1.0f)
	{
		CompleteReturnToPool();
	}
}

void ARLProjectile::CompleteReturnToPool()
{
	if (!bIsActive)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (URLProjectilePoolSubsystem* PoolSubsystem =
			World->GetSubsystem<URLProjectilePoolSubsystem>())
		{
			PoolSubsystem->ReleaseProjectile(this);
			return;
		}
	}

	Destroy();
}

void ARLProjectile::DeactivateForPool()
{
	bIsActive = false;
	bIsFadingOut = false;
	bIsReflected = false;
	bCanBeReflected = true;
	bIsExplosive = false;
	bExplosiveBlinkWarning = false;
	bIsDelayedExplosive = false;
	bDelayedExplosivePaused = false;
	bDelayedExplosiveResumed = false;
	bIsFakeProjectile = false;
	bFakeDormant = false;
	bExplodesOnEnemyImpact = false;
	bIsGuardProjectile = false;
	bSplitsOnParry = false;
	bIsRallyProjectile = false;
	bRallyFinalShot = false;
	RemainingPierces = 0;
	ReflectionChain = 0;
	bWasPerfectParried = false;
	bWasCloseRangeParried = false;
	bWasOverdriveReflected = false;
	ExplosiveElapsedTime = 0.0f;
	ExplosiveNextBlinkTime = 0.0f;
	DelayedExplosivePauseElapsedTime = 0.0f;
	FakeDormantElapsedTime = 0.0f;
	RallyCurrentSpeed = 0.0f;
	RallyCount = 0;
	MaxRallies = 0;
	RallyTarget.Reset();
	RallyFinalTarget.Reset();
	DelayedExplosiveTarget.Reset();
	FakeTarget.Reset();
	ExplosiveMaterialInstance = nullptr;
	FadeMaterialInstance = nullptr;
	SpecialMaterialInstance = nullptr;
	DeactivateReflectedTrail();
	ReflectedTrailVFX = nullptr;
	ExplosionVFX = nullptr;
	ReflectedTrailScale = 1.0f;
	ExplosionVFXScale = 1.0f;
	ActiveDefinition = nullptr;
	FadeOutElapsedTime = 0.0f;
	ProjectileMesh->SetRelativeScale3D(DefaultProjectileMeshScale);
	ProjectileMesh->SetVisibility(true, true);
	CollisionComponent->SetSphereRadius(DefaultCollisionRadius, false);
	SetActorTickEnabled(false);
	ResetReflectedAfterimages();
	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);

	if (AActor* OwningActor = GetOwner())
	{
		if (UPrimitiveComponent* OwnerRootComponent =
			Cast<UPrimitiveComponent>(OwningActor->GetRootComponent()))
		{
			OwnerRootComponent->IgnoreActorWhenMoving(this, false);
		}
	}

	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Deactivate();
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionComponent->ClearMoveIgnoreActors();
	SetActorHiddenInGame(true);
	SetOwner(nullptr);
	SetInstigator(nullptr);
}

void ARLProjectile::CreateReflectedAfterimages()
{
	ReflectedAfterimageMeshes.Reset();
	const int32 AfterimageCount = FMath::Clamp(ReflectedAfterimageCount, 1, 8);
	for (int32 AfterimageIndex = 0; AfterimageIndex < AfterimageCount; ++AfterimageIndex)
	{
		const FName ComponentName(*FString::Printf(
			TEXT("ReflectedAfterimage_%d"),
			AfterimageIndex));
		UStaticMeshComponent* AfterimageMesh = NewObject<UStaticMeshComponent>(this, ComponentName);
		if (!AfterimageMesh)
		{
			continue;
		}

		AfterimageMesh->SetupAttachment(CollisionComponent);
		AfterimageMesh->SetAbsolute(true, true, true);
		AfterimageMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AfterimageMesh->SetCanEverAffectNavigation(false);
		AfterimageMesh->SetCastShadow(false);
		AfterimageMesh->SetReceivesDecals(false);
		AfterimageMesh->SetStaticMesh(ProjectileMesh->GetStaticMesh());
		if (ReflectedMaterial)
		{
			AfterimageMesh->SetMaterial(0, ReflectedMaterial);
		}
		AfterimageMesh->SetVisibility(false, true);
		AfterimageMesh->RegisterComponent();
		ReflectedAfterimageMeshes.Add(AfterimageMesh);
	}
}

void ARLProjectile::ResetReflectedAfterimages()
{
	ReflectedAfterimageHistory.Reset();
	ReflectedAfterimageSampleAccumulator = 0.0f;
	for (UStaticMeshComponent* AfterimageMesh : ReflectedAfterimageMeshes)
	{
		if (AfterimageMesh)
		{
			AfterimageMesh->SetVisibility(false, true);
		}
	}
}

void ARLProjectile::UpdateReflectedAfterimages(float DeltaTime)
{
	if (!ProjectileMesh || ReflectedAfterimageMeshes.IsEmpty())
	{
		return;
	}

	ReflectedAfterimageSampleAccumulator += DeltaTime;
	const float SampleInterval = FMath::Max(0.01f, ReflectedAfterimageSampleInterval);
	if (ReflectedAfterimageSampleAccumulator < SampleInterval)
	{
		return;
	}

	ReflectedAfterimageSampleAccumulator = FMath::Fmod(
		ReflectedAfterimageSampleAccumulator,
		SampleInterval);
	ReflectedAfterimageHistory.Insert(ProjectileMesh->GetComponentTransform(), 0);
	ReflectedAfterimageHistory.SetNum(
		FMath::Min(
			ReflectedAfterimageHistory.Num(),
			ReflectedAfterimageMeshes.Num() + 1),
		EAllowShrinking::No);

	for (int32 AfterimageIndex = 0;
		AfterimageIndex < ReflectedAfterimageMeshes.Num();
		++AfterimageIndex)
	{
		UStaticMeshComponent* AfterimageMesh = ReflectedAfterimageMeshes[AfterimageIndex];
		const int32 HistoryIndex = AfterimageIndex + 1;
		if (!AfterimageMesh || !ReflectedAfterimageHistory.IsValidIndex(HistoryIndex))
		{
			if (AfterimageMesh)
			{
				AfterimageMesh->SetVisibility(false, true);
			}
			continue;
		}

		FTransform AfterimageTransform = ReflectedAfterimageHistory[HistoryIndex];
		const float ScaleMultiplier = FMath::Max(
			0.35f,
			1.0f - ReflectedAfterimageScaleFalloff * static_cast<float>(AfterimageIndex + 1));
		AfterimageTransform.SetScale3D(
			AfterimageTransform.GetScale3D() * ScaleMultiplier);
		AfterimageMesh->SetWorldTransform(
			AfterimageTransform,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		AfterimageMesh->SetVisibility(true, true);
	}
}

void ARLProjectile::UpdateProjectileMaterial()
{
	UMaterialInterface* Material = bIsReflected
		? ReflectedMaterial.Get()
		: HostileMaterial.Get();

	if (ProjectileMesh && Material)
	{
		ProjectileMesh->SetMaterial(0, Material);
	}
	SpecialMaterialInstance = nullptr;
	if (bIsGuardProjectile)
	{
		ApplySpecialColor(GuardColor, 18.0f);
	}
	else if (bSplitsOnParry)
	{
		ApplySpecialColor(ParrySplitColor, 22.0f);
	}
	else if (bIsFakeProjectile)
	{
		ApplySpecialColor(FakeColor, 5.0f, FakeOpacity);
	}

	for (UStaticMeshComponent* AfterimageMesh : ReflectedAfterimageMeshes)
	{
		if (AfterimageMesh && ReflectedMaterial)
		{
			AfterimageMesh->SetMaterial(0, ReflectedMaterial);
		}
	}
}

void ARLProjectile::ActivateReflectedTrail(float VisualScaleMultiplier)
{
	if (!ReflectedTrailComponent || !ReflectedTrailVFX)
	{
		return;
	}

	ReflectedTrailComponent->SetAsset(ReflectedTrailVFX);
	ReflectedTrailComponent->SetRelativeScale3D(
		FVector(ReflectedTrailScale * FMath::Max(0.1f, VisualScaleMultiplier)));
	ReflectedTrailComponent->Activate(true);
}

void ARLProjectile::DeactivateReflectedTrail()
{
	if (!ReflectedTrailComponent)
	{
		return;
	}

	ReflectedTrailComponent->Deactivate();
	ReflectedTrailComponent->SetAsset(nullptr);
}

void ARLProjectile::ApplySpecialColor(
	const FLinearColor& Color,
	float EmissiveIntensity,
	float Opacity)
{
	if (!ProjectileMesh)
	{
		return;
	}

	SpecialMaterialInstance = ProjectileMesh->CreateDynamicMaterialInstance(0);
	if (!SpecialMaterialInstance)
	{
		return;
	}

	SpecialMaterialInstance->SetVectorParameterValue(ProjectileColorParameterName, Color);
	SpecialMaterialInstance->SetScalarParameterValue(
		EmissiveIntensityParameterName,
		FMath::Max(0.0f, EmissiveIntensity));
	SpecialMaterialInstance->SetScalarParameterValue(
		FadeOpacityParameterName,
		FMath::Clamp(Opacity, 0.0f, 1.0f));
}

void ARLProjectile::UpdateExplosive(float DeltaTime)
{
	ExplosiveElapsedTime += FMath::Max(0.0f, DeltaTime);
	const float FuseDuration = FMath::Max(0.1f, ExplosiveFuseDuration);
	if (ExplosiveElapsedTime >= FuseDuration)
	{
		Explode();
		return;
	}

	if (ExplosiveElapsedTime < ExplosiveNextBlinkTime)
	{
		return;
	}

	bExplosiveBlinkWarning = !bExplosiveBlinkWarning;
	ApplyExplosiveBlinkColor(bExplosiveBlinkWarning);
	const float FuseAlpha = FMath::Clamp(ExplosiveElapsedTime / FuseDuration, 0.0f, 1.0f);
	const float BlinkInterval = FMath::Lerp(
		FMath::Max(0.01f, ExplosiveBlinkStartInterval),
		FMath::Max(0.01f, ExplosiveBlinkEndInterval),
		FuseAlpha * FuseAlpha);
	ExplosiveNextBlinkTime = ExplosiveElapsedTime + BlinkInterval;
}

void ARLProjectile::ApplyExplosiveBlinkColor(bool bUseWarningColor)
{
	if (!ExplosiveMaterialInstance)
	{
		return;
	}

	ExplosiveMaterialInstance->SetVectorParameterValue(
		ProjectileColorParameterName,
		bUseWarningColor ? ExplosiveWarningColor : ExplosiveBaseColor);
	ExplosiveMaterialInstance->SetScalarParameterValue(
		EmissiveIntensityParameterName,
		bUseWarningColor
			? FMath::Max(0.0f, ExplosiveWarningEmissiveIntensity)
			: FMath::Max(0.0f, ExplosiveBaseEmissiveIntensity));
}

void ARLProjectile::UpdateDelayedExplosive(float DeltaTime)
{
	if (!bIsDelayedExplosive || bDelayedExplosiveResumed)
	{
		return;
	}

	AActor* TargetActor = DelayedExplosiveTarget.Get();
	if (!IsValid(TargetActor))
	{
		bIsDelayedExplosive = false;
		return;
	}

	if (!bDelayedExplosivePaused)
	{
		if (FVector::DistSquared2D(GetActorLocation(), TargetActor->GetActorLocation()) >
			FMath::Square(FMath::Max(1.0f, DelayedExplosiveTriggerDistance)))
		{
			return;
		}

		bDelayedExplosivePaused = true;
		DelayedExplosivePauseElapsedTime = 0.0f;
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
		return;
	}

	DelayedExplosivePauseElapsedTime += FMath::Max(0.0f, DeltaTime);
	if (DelayedExplosivePauseElapsedTime < FMath::Max(0.0f, DelayedExplosivePauseDuration))
	{
		return;
	}

	bDelayedExplosivePaused = false;
	bDelayedExplosiveResumed = true;
	const FVector ResumeDirection =
		(TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	SetProjectileSpeed(
		ProjectileSpeed * FMath::Max(0.1f, DelayedExplosiveResumeSpeedMultiplier),
		ResumeDirection);
}

void ARLProjectile::UpdateFake(float DeltaTime)
{
	AActor* TargetActor = FakeTarget.Get();
	if (!IsValid(TargetActor))
	{
		ReturnToPool();
		return;
	}

	if (!bFakeDormant)
	{
		if (FVector::DistSquared2D(GetActorLocation(), TargetActor->GetActorLocation()) >
			FMath::Square(FMath::Max(1.0f, FakeTriggerDistance)))
		{
			return;
		}

		bFakeDormant = true;
		FakeDormantElapsedTime = 0.0f;
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ProjectileMesh->SetVisibility(false, true);
		return;
	}

	FakeDormantElapsedTime += FMath::Max(0.0f, DeltaTime);
	if (FakeDormantElapsedTime >= FMath::Max(0.0f, FakeRevealDelay))
	{
		RevealFakeProjectile();
	}
}

void ARLProjectile::RevealFakeProjectile()
{
	AActor* TargetActor = FakeTarget.Get();
	if (!IsValid(TargetActor))
	{
		ReturnToPool();
		return;
	}

	bIsFakeProjectile = false;
	bFakeDormant = false;
	bCanBeReflected = true;
	FakeDormantElapsedTime = 0.0f;
	ProjectileMesh->SetVisibility(true, true);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	UpdateProjectileMaterial();
	const FVector RealShotDirection =
		(TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	SetProjectileSpeed(
		ProjectileSpeed * FMath::Max(0.1f, FakeRealSpeedMultiplier),
		RealShotDirection);
	GetWorldTimerManager().SetTimer(
		LifetimeTimerHandle,
		this,
		&ThisClass::ReturnToPool,
		FMath::Max(0.1f, LifeSeconds),
		false);
	FakeTarget.Reset();
}

void ARLProjectile::SpawnParrySplitFragments(
	AActor* OriginalOwner,
	APawn* OriginalInstigator,
	AActor* PlayerActor)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(PlayerActor))
	{
		return;
	}

	URLProjectilePoolSubsystem* PoolSubsystem =
		World->GetSubsystem<URLProjectilePoolSubsystem>();
	if (!PoolSubsystem)
	{
		return;
	}

	const int32 FragmentCount = FMath::Clamp(ParrySplitFragmentCount, 1, 6);
	const FVector DirectionToPlayer =
		(PlayerActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (DirectionToPlayer.IsNearlyZero())
	{
		return;
	}

	for (int32 FragmentIndex = 0; FragmentIndex < FragmentCount; ++FragmentIndex)
	{
		const float FragmentAlpha = FragmentCount > 1
			? static_cast<float>(FragmentIndex) / static_cast<float>(FragmentCount - 1)
			: 0.5f;
		const float FragmentAngle = FMath::Lerp(
			-ParrySplitSpreadAngle * 0.5f,
			ParrySplitSpreadAngle * 0.5f,
			FragmentAlpha);
		const FVector FragmentDirection = DirectionToPlayer.RotateAngleAxis(
			FragmentAngle,
			FVector::UpVector);
		const FTransform FragmentTransform(
			FragmentDirection.Rotation(),
			GetActorLocation() + FragmentDirection * (DefaultCollisionRadius + 4.0f));
		if (ARLProjectile* Fragment = PoolSubsystem->AcquireProjectile(
			GetClass(),
			FragmentTransform,
			OriginalOwner,
			OriginalInstigator))
		{
			Fragment->InitializeFromDefinition(ActiveDefinition, nullptr, false);
			Fragment->ConfigureAsSplitFragment(
				ParrySplitFragmentSpeedMultiplier,
				ParrySplitFragmentScale);
		}
	}
}

void ARLProjectile::ConfigureAsSplitFragment(float SpeedMultiplier, float VisualScale)
{
	const float SafeScale = FMath::Clamp(VisualScale, 0.1f, 1.0f);
	ProjectileMesh->SetRelativeScale3D(DefaultProjectileMeshScale * SafeScale);
	CollisionComponent->SetSphereRadius(DefaultCollisionRadius * SafeScale, true);
	SetProjectileSpeed(
		ProjectileSpeed * FMath::Max(0.1f, SpeedMultiplier),
		GetActorForwardVector());
}

void ARLProjectile::Explode()
{
	TriggerExplosion(true, false);
}

void ARLProjectile::TriggerExplosion(bool bDamagePlayer, bool bDamageEnemies)
{
	if (!bIsActive || bIsFadingOut)
	{
		return;
	}

	const FVector ExplosionLocation = GetActorLocation();
	const float BlastRadius = FMath::Max(1.0f, ExplosionRadius);
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ExplosionSound,
			ExplosionLocation,
			FRotator::ZeroRotator,
			FMath::Max(0.0f, ExplosionSoundVolume),
			1.0f,
			0.0f,
			nullptr,
			ExplosionSoundConcurrency,
			this);
	}
	if (ExplosionVFX)
	{
		if (URLVFXPoolSubsystem* VFXPool = GetWorld()->GetSubsystem<URLVFXPoolSubsystem>())
		{
			VFXPool->PlaySystemAtLocation(
				ExplosionVFX,
				ExplosionLocation,
				FRotator::ZeroRotator,
				FVector(ExplosionVFXScale));
		}
	}
	SpawnExplosionVisual(ExplosionLocation, BlastRadius);
	OnExploded(ExplosionLocation, BlastRadius);

	if (bDamagePlayer)
	{
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			if (FVector::DistSquared(PlayerPawn->GetActorLocation(), ExplosionLocation) <=
				FMath::Square(BlastRadius))
			{
				UGameplayStatics::ApplyDamage(
					PlayerPawn,
					FMath::Max(0.0f, ExplosionDamage),
					GetInstigatorController(),
					this,
					UDamageType::StaticClass());
			}
		}
	}

	if (bDamageEnemies)
	{
		for (TActorIterator<ARLEnemyCharacter> Iterator(GetWorld()); Iterator; ++Iterator)
		{
			ARLEnemyCharacter* Enemy = *Iterator;
			if (!IsValid(Enemy) || !Enemy->IsPoolActive() ||
				FVector::DistSquared(Enemy->GetActorLocation(), ExplosionLocation) >
				FMath::Square(BlastRadius))
			{
				continue;
			}

			UGameplayStatics::ApplyDamage(
				Enemy,
				FMath::Max(0.0f, ExplosionDamage),
				GetInstigatorController(),
				this,
				UDamageType::StaticClass());
		}
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Projectile explosion at %s with radius %.1f. PlayerDamage=%s EnemyDamage=%s"),
		*ExplosionLocation.ToCompactString(),
		BlastRadius,
		bDamagePlayer ? TEXT("true") : TEXT("false"),
		bDamageEnemies ? TEXT("true") : TEXT("false"));
	ReturnToPool();
}

void ARLProjectile::SpawnExplosionVisual(
	const FVector& ExplosionLocation,
	float BlastRadius)
{
	UWorld* World = GetWorld();
	if (!World || !ProjectileMesh || !ProjectileMesh->GetStaticMesh())
	{
		return;
	}

	FVector GroundLocation = ExplosionLocation;
	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ExplosionVisualGroundTrace), false, this);
	const FVector TraceStart = ExplosionLocation + FVector(0.0f, 0.0f, 200.0f);
	const FVector TraceEnd = ExplosionLocation - FVector(0.0f, 0.0f, 600.0f);
	if (World->LineTraceSingleByChannel(
		GroundHit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams))
	{
		GroundLocation = GroundHit.ImpactPoint;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARLExplosionVisual* ExplosionVisual = World->SpawnActor<ARLExplosionVisual>(
		ARLExplosionVisual::StaticClass(),
		GroundLocation,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!ExplosionVisual)
	{
		return;
	}

	UMaterialInterface* ShardMaterial = ReflectedMaterial
		? ReflectedMaterial.Get()
		: ProjectileMesh->GetMaterial(0);
	ExplosionVisual->Initialize(
		ProjectileMesh->GetStaticMesh(),
		ShardMaterial,
		DefaultProjectileMeshScale,
		BlastRadius,
		ExplosiveDecalColor);
}

void ARLProjectile::UpdateRally(float DeltaTime)
{
	(void)DeltaTime;

	AActor* Target = RallyTarget.Get();
	if (!IsValid(Target))
	{
		SetRallyTarget(RallyFinalTarget.Get());
		return;
	}

	if (ARLEnemyCharacter* TargetEnemy = Cast<ARLEnemyCharacter>(Target))
	{
		if (!TargetEnemy->IsPoolActive())
		{
			AActor* ReplacementTarget = FindNextRallyTarget(GetOwner());
			SetRallyTarget(ReplacementTarget ? ReplacementTarget : RallyFinalTarget.Get());
			return;
		}

		if (FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) <=
			FMath::Square(FMath::Max(1.0f, RallyArrivalRadius)))
		{
			AdvanceRally();
			return;
		}
	}

	const FVector TargetDirection = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	if (!TargetDirection.IsNearlyZero())
	{
		SetProjectileSpeed(FMath::Max(1.0f, RallyCurrentSpeed), TargetDirection);
	}
}

void ARLProjectile::AdvanceRally()
{
	ARLEnemyCharacter* RelayEnemy = Cast<ARLEnemyCharacter>(RallyTarget.Get());
	if (!RelayEnemy || !RelayEnemy->IsPoolActive())
	{
		SetRallyTarget(RallyFinalTarget.Get());
		return;
	}

	++RallyCount;
	RallyCurrentSpeed = FMath::Min(
		ProjectileSpeed * FMath::Max(1.0f, RallyMaxSpeedMultiplier),
		FMath::Max(ProjectileSpeed, RallyCurrentSpeed) * RallySpeedMultiplierPerRally);
	SetProjectileOwnerAndIgnore(RelayEnemy);

	AActor* NextTarget = nullptr;
	if (RallyCount < MaxRallies)
	{
		NextTarget = FindNextRallyTarget(RelayEnemy);
	}
	SetRallyTarget(NextTarget ? NextTarget : RallyFinalTarget.Get());

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Rally projectile relay %d/%d, speed %.1f."),
		RallyCount,
		MaxRallies,
		RallyCurrentSpeed);
}

AActor* ARLProjectile::FindNextRallyTarget(AActor* RelaySource) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ARLEnemyCharacter* BestTarget = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<ARLEnemyCharacter> Iterator(World); Iterator; ++Iterator)
	{
		ARLEnemyCharacter* Candidate = *Iterator;
		if (!IsValid(Candidate) || !Candidate->IsPoolActive() || Candidate == RelaySource)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(
			GetActorLocation(),
			Candidate->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

void ARLProjectile::SetRallyTarget(AActor* NewTarget)
{
	if (!IsValid(NewTarget))
	{
		ReturnToPool();
		return;
	}

	RallyTarget = NewTarget;
	bRallyFinalShot = NewTarget == RallyFinalTarget.Get();
	const FVector TargetDirection = (NewTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	SetProjectileSpeed(FMath::Max(1.0f, RallyCurrentSpeed), TargetDirection);
}

void ARLProjectile::SetProjectileOwnerAndIgnore(AActor* NewOwner)
{
	if (AActor* PreviousOwner = GetOwner())
	{
		if (UPrimitiveComponent* PreviousOwnerRoot =
			Cast<UPrimitiveComponent>(PreviousOwner->GetRootComponent()))
		{
			PreviousOwnerRoot->IgnoreActorWhenMoving(this, false);
		}
	}

	SetOwner(NewOwner);
	CollisionComponent->ClearMoveIgnoreActors();
	if (NewOwner)
	{
		CollisionComponent->IgnoreActorWhenMoving(NewOwner, true);
		if (UPrimitiveComponent* NewOwnerRoot =
			Cast<UPrimitiveComponent>(NewOwner->GetRootComponent()))
		{
			NewOwnerRoot->IgnoreActorWhenMoving(this, true);
		}
	}
}

void ARLProjectile::SetProjectileSpeed(float NewSpeed, const FVector& Direction)
{
	const FVector SafeDirection = Direction.GetSafeNormal();
	if (SafeDirection.IsNearlyZero())
	{
		return;
	}

	const float SafeSpeed = FMath::Max(1.0f, NewSpeed);
	SetActorRotation(SafeDirection.Rotation());
	ProjectileMovement->InitialSpeed = SafeSpeed;
	ProjectileMovement->MaxSpeed = SafeSpeed;
	ProjectileMovement->Velocity = SafeDirection * SafeSpeed;
	if (!ProjectileMovement->IsActive())
	{
		ProjectileMovement->Activate(true);
	}
	ProjectileMovement->UpdateComponentVelocity();
}

bool ARLProjectile::IsOutsidePlayArea() const
{
	const FVector Location = GetActorLocation();
	return FMath::Abs(Location.X - PlayAreaCenter.X) >
			FMath::Max(1.0f, PlayAreaHalfExtent.X) ||
		FMath::Abs(Location.Y - PlayAreaCenter.Y) >
			FMath::Max(1.0f, PlayAreaHalfExtent.Y);
}

void ARLProjectile::HandleProjectileHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!bIsActive)
	{
		return;
	}

	if (TryDetonateOnPlayerContact(OtherActor))
	{
		return;
	}

	if (bIsRallyProjectile && OtherActor &&
		OtherActor == RallyTarget.Get() && OtherActor->IsA<ARLEnemyCharacter>())
	{
		AdvanceRally();
		return;
	}

	if (ShouldIgnoreActor(OtherActor))
	{
		return;
	}

	if (TryExplodeOnEnemyContact(OtherActor))
	{
		return;
	}

	if (bIsExplosive)
	{
		Explode();
		return;
	}

	ApplyDamageAndReturn(OtherActor);
}

void ARLProjectile::HandleProjectileOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bIsActive)
	{
		return;
	}

	if (TryDetonateOnPlayerContact(OtherActor))
	{
		return;
	}

	if (bIsRallyProjectile && OtherActor &&
		OtherActor == RallyTarget.Get() && OtherActor->IsA<ARLEnemyCharacter>())
	{
		AdvanceRally();
		return;
	}

	if (ShouldIgnoreActor(OtherActor))
	{
		return;
	}

	if (TryExplodeOnEnemyContact(OtherActor))
	{
		return;
	}

	if (bIsExplosive)
	{
		Explode();
		return;
	}

	ApplyDamageAndReturn(OtherActor);
}

bool ARLProjectile::TryDetonateOnPlayerContact(AActor* OtherActor)
{
	if (!bIsExplosive || !IsValid(OtherActor))
	{
		return false;
	}

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (OtherActor != PlayerPawn)
	{
		return false;
	}

	return Detonate();
}

bool ARLProjectile::TryExplodeOnEnemyContact(AActor* OtherActor)
{
	if (!bExplodesOnEnemyImpact || !bIsReflected ||
		!IsValid(OtherActor) || !OtherActor->IsA<ARLEnemyCharacter>())
	{
		return false;
	}

	TriggerExplosion(false, true);
	return true;
}

bool ARLProjectile::ShouldIgnoreActor(const AActor* OtherActor) const
{
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner())
	{
		return true;
	}

	const APawn* ProjectileInstigator = GetInstigator();
	return ProjectileInstigator &&
		ProjectileInstigator->IsA<ARLEnemyCharacter>() &&
		OtherActor->IsA<ARLEnemyCharacter>();
}

void ARLProjectile::ApplyDamageAndReturn(AActor* OtherActor)
{
	ARLEnemyCharacter* HitEnemy = Cast<ARLEnemyCharacter>(OtherActor);
	if (bIsReflected && HitEnemy && HitEnemy->TryAbsorbReflectedProjectile())
	{
		ReturnToPool();
		return;
	}

	if (bIsGuardProjectile && bIsReflected && HitEnemy)
	{
		ReturnToPool();
		return;
	}

	const bool bPiercesEnemy = bIsReflected &&
		OtherActor &&
		OtherActor->IsA<ARLEnemyCharacter>() &&
		RemainingPierces > 0;

	const float AppliedDamage = UGameplayStatics::ApplyDamage(
		OtherActor,
		DamageAmount,
		GetInstigatorController(),
		this,
		UDamageType::StaticClass());

	if (bPiercesEnemy && AppliedDamage > 0.0f)
	{
		--RemainingPierces;
		return;
	}

	ReturnToPool();
}

