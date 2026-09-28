#include "Combat/RLProjectile.h"

#include "Combat/RLExplosionVisual.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Enemies/RLEnemyCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
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

	if (bIsExplosive)
	{
		UpdateExplosive(DeltaTime);
	}

	if (!bIsActive || bIsFadingOut)
	{
		return;
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
	SetActorTickEnabled(false);
	ResetReflectedAfterimages();
	bIsReflected = false;
	bIsFadingOut = false;
	bCanBeReflected = true;
	bIsExplosive = false;
	bExplosiveBlinkWarning = false;
	bIsRallyProjectile = false;
	bRallyFinalShot = false;
	RemainingPierces = 0;
	ReflectionChain = 0;
	bWasPerfectParried = false;
	bWasCloseRangeParried = false;
	bWasOverdriveReflected = false;
	ExplosiveElapsedTime = 0.0f;
	ExplosiveNextBlinkTime = 0.0f;
	RallyCurrentSpeed = ProjectileSpeed;
	RallySpeedMultiplierPerRally = 1.15f;
	RallyCount = 0;
	MaxRallies = 0;
	RallyTarget.Reset();
	RallyFinalTarget.Reset();
	ExplosiveMaterialInstance = nullptr;
	FadeMaterialInstance = nullptr;
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
	GetWorldTimerManager().SetTimer(
		LifetimeTimerHandle,
		this,
		&ThisClass::ReturnToPool,
		FMath::Max(0.1f, LifeSeconds),
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
	const float ReflectedScale = FMath::Max(
		0.1f,
		ReflectionParams.VisualScaleMultiplier);
	ProjectileMesh->SetRelativeScale3D(
		DefaultProjectileMeshScale * ReflectedScale);
	CollisionComponent->SetSphereRadius(
		DefaultCollisionRadius * ReflectedScale,
		true);
	UpdateProjectileMaterial();
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
	bIsRallyProjectile = false;
	bRallyFinalShot = false;
	RemainingPierces = 0;
	ReflectionChain = 0;
	bWasPerfectParried = false;
	bWasCloseRangeParried = false;
	bWasOverdriveReflected = false;
	ExplosiveElapsedTime = 0.0f;
	ExplosiveNextBlinkTime = 0.0f;
	RallyCurrentSpeed = 0.0f;
	RallyCount = 0;
	MaxRallies = 0;
	RallyTarget.Reset();
	RallyFinalTarget.Reset();
	ExplosiveMaterialInstance = nullptr;
	FadeMaterialInstance = nullptr;
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

	for (UStaticMeshComponent* AfterimageMesh : ReflectedAfterimageMeshes)
	{
		if (AfterimageMesh && ReflectedMaterial)
		{
			AfterimageMesh->SetMaterial(0, ReflectedMaterial);
		}
	}
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

void ARLProjectile::Explode()
{
	if (!bIsActive)
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
			FMath::Max(0.0f, ExplosionSoundVolume));
	}
	SpawnExplosionVisual(ExplosionLocation, BlastRadius);
	OnExploded(ExplosionLocation, BlastRadius);

	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (FVector::Dist(PlayerPawn->GetActorLocation(), ExplosionLocation) <= BlastRadius)
		{
			UGameplayStatics::ApplyDamage(
				PlayerPawn,
				FMath::Max(0.0f, ExplosionDamage),
				GetInstigatorController(),
				this,
				UDamageType::StaticClass());
		}
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Explosive projectile detonated at %s with radius %.1f."),
		*ExplosionLocation.ToCompactString(),
		BlastRadius);
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
	const bool bPiercesEnemy = bIsReflected &&
		OtherActor &&
		OtherActor->IsA<ARLEnemyCharacter>() &&
		RemainingPierces > 0;

	UGameplayStatics::ApplyDamage(
		OtherActor,
		DamageAmount,
		GetInstigatorController(),
		this,
		UDamageType::StaticClass());

	if (bPiercesEnemy)
	{
		--RemainingPierces;
		return;
	}

	ReturnToPool();
}

