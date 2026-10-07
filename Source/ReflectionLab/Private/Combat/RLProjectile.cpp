#include "Combat/RLProjectile.h"

#include "Combat/RLProjectilePoolSubsystem.h"
#include "Combat/RLProjectileFeedbackComponent.h"
#include "Combat/RLProjectileVisualComponent.h"
#include "Combat/RLProjectileSpecialComponent.h"
#include "Combat/RLProjectileRallyComponent.h"
#include "Combat/RLProjectileContactComponent.h"
#include "Enemies/Components/RLBossOverloadComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"

namespace
{
	constexpr float GlobalProjectileLifeSeconds = 999.0f;
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

	VisualComponent = CreateDefaultSubobject<URLProjectileVisualComponent>(TEXT("VisualComponent"));
	SpecialComponent = CreateDefaultSubobject<URLProjectileSpecialComponent>(TEXT("SpecialComponent"));
	RallyComponent = CreateDefaultSubobject<URLProjectileRallyComponent>(TEXT("RallyComponent"));
	ContactComponent = CreateDefaultSubobject<URLProjectileContactComponent>(TEXT("ContactComponent"));
	FeedbackComponent = CreateDefaultSubobject<URLProjectileFeedbackComponent>(TEXT("FeedbackComponent"));
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetCanEverAffectNavigation(false);
	// Projectile materials use translucency for blinking and pool fade-out.
	ProjectileMesh->bDisallowNanite = true;

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

}

void ARLProjectile::BeginPlay()
{
	Super::BeginPlay();
	// Enforce this for existing blueprint component defaults as well.
	if (!ProjectileMesh->bDisallowNanite)
	{
		ProjectileMesh->bDisallowNanite = true;
		ProjectileMesh->MarkRenderStateDirty();
	}

	SpecialComponent->InitializeFeedback();

	DefaultProjectileMeshScale = ProjectileMesh->GetRelativeScale3D();
	DefaultCollisionRadius = CollisionComponent->GetUnscaledSphereRadius();
	VisualComponent->CreateReflectedAfterimages();

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
		if (VisualComponent->UpdateFadeOut(DeltaTime))
		{
			CompleteReturnToPool();
		}
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

	if (SpecialComponent->IsExplosive())
	{
		SpecialComponent->UpdateExplosive(DeltaTime);
	}

	if (!bIsActive || bIsFadingOut)
	{
		return;
	}

	if (SpecialComponent->IsDelayedExplosive())
	{
		SpecialComponent->UpdateDelayedExplosive(DeltaTime);
	}

	if (SpecialComponent->IsFakeProjectile())
	{
		SpecialComponent->UpdateFake(DeltaTime);
	}

	if (RallyComponent->IsRallyProjectile())
	{
		RallyComponent->UpdateRally(DeltaTime);
	}
	if (!bIsActive || bIsFadingOut) { return; }

	if (bIsReflected || RallyComponent->IsRallyProjectile())
	{
		VisualComponent->UpdateReflectedTrailLocation();
		VisualComponent->UpdateReflectedAfterimages(DeltaTime);
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
	bIsReflected = false;
	bIsFadingOut = false;
	bCanBeReflected = true;
	SpecialComponent->ResetRuntimeState();
	RallyComponent->ResetRuntimeState(ProjectileSpeed);
	VisualComponent->ResetRuntimeState();
	ReflectionState.RemainingPierces = 0;
	ReflectionState.ReflectionChain = 0;
	ReflectionState.bWasPerfectParried = false;
	ReflectionState.bWasCloseRangeParried = false;
	ReflectionState.bWasOverdriveReflected = false;
	ActiveDefinition = nullptr;
	ProjectileMesh->SetRelativeScale3D(DefaultProjectileMeshScale);
	ProjectileMesh->SetVisibility(true, true);
	CollisionComponent->SetSphereRadius(DefaultCollisionRadius, false);
	VisualComponent->UpdateProjectileMaterial();
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
	ProjectileMovement->SetUpdatedComponent(CollisionComponent);
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->Activate(true);
	ProjectileMovement->Velocity =
		GetActorForwardVector() * ProjectileSpeed;
	ProjectileMovement->UpdateComponentVelocity();

	bIsActive = true;
	FeedbackComponent->ResetForActivation();
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
		SpecialComponent->ConfigureAsExplosive();
		break;
	case ERLProjectileBehavior::DelayedExplosive:
		SpecialComponent->ConfigureAsDelayedExplosive(TargetActor);
		break;
	case ERLProjectileBehavior::Fake:
		SpecialComponent->ConfigureAsFake(TargetActor);
		break;
	case ERLProjectileBehavior::Guard:
		SpecialComponent->ConfigureAsGuard();
		break;
	case ERLProjectileBehavior::ParrySplit:
		SpecialComponent->ConfigureAsParrySplit();
		break;
	case ERLProjectileBehavior::Rally:
		RallyComponent->ConfigureAsRally(
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
	VisualComponent->ApplyDefinitionSettings(Definition);
	SpecialComponent->ApplyDefinitionSettings(Definition);
	RallyComponent->ApplyDefinitionSettings(Definition);

	const float VisualScale = FMath::Max(0.1f, Definition.VisualScale);
	ProjectileMesh->SetRelativeScale3D(DefaultProjectileMeshScale * VisualScale);
	CollisionComponent->SetSphereRadius(DefaultCollisionRadius * VisualScale, false);
	VisualComponent->UpdateProjectileMaterial();
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
	if (!bIsActive || bIsFadingOut || !bCanBeReflected || !NewOwner || !NewInstigator || NewDirection.IsNearlyZero())
	{
		return false;
	}
	AActor* OriginalOwner = GetOwner();
	APawn* OriginalInstigator = GetInstigator();
	const bool bShouldSpawnParryFragments = SpecialComponent->SplitsOnParry();

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
	const float ReflectedSpeed = SpecialComponent->CalculateReturnSpeed(
		ReflectionParams.SpeedMultiplier, ReflectedSpeedMultiplier);
	SetActorRotation(ReflectedDirection.Rotation());
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->InitialSpeed = ReflectedSpeed;
	ProjectileMovement->MaxSpeed = ReflectedSpeed;
	ProjectileMovement->Velocity = ReflectedDirection * ReflectedSpeed;
	ProjectileMovement->Activate(true);
	ProjectileMovement->UpdateComponentVelocity();
	bIsReflected = true;
	SpecialComponent->PrepareForReflection();
	RallyComponent->ResetTargetsForReflection();
	// Rally returns may hit one additional enemy, but cannot form a long relay chain.
	ReflectionState.RemainingPierces = RallyComponent->IsRallyProjectile()
		? FMath::Clamp(ReflectionParams.PierceCount, 0, 1)
		: FMath::Max(0, ReflectionParams.PierceCount);
	ReflectionState.ReflectionChain = FMath::Max(0, ReflectionParams.ReflectionChain);
	ReflectionState.bWasPerfectParried = ReflectionParams.bPerfectParry;
	ReflectionState.bWasCloseRangeParried = ReflectionParams.bCloseRangeParry;
	ReflectionState.bWasOverdriveReflected = ReflectionParams.bOverdrive;
	const float RequestedVisualScale = FMath::Max(
		0.1f,
		ReflectionParams.VisualScaleMultiplier);
	float ReflectedScale;
	float CollisionScale;
	SpecialComponent->CalculateReturnScales(RequestedVisualScale, ReflectedScale, CollisionScale);
	ProjectileMesh->SetRelativeScale3D(
		DefaultProjectileMeshScale * ReflectedScale);
	CollisionComponent->SetSphereRadius(
		DefaultCollisionRadius * CollisionScale,
		false);
	VisualComponent->UpdateProjectileMaterial();
	SpecialComponent->RestartReflectedFuse();
	RallyComponent->BeginReflectedReturn(ReflectedSpeed);
	VisualComponent->ActivateReflectedTrail(ReflectedScale);
	VisualComponent->ResetReflectedAfterimages();
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
		SpecialComponent->SpawnParrySplitFragments(OriginalOwner, OriginalInstigator, NewOwner);
	}
	// Reflection inside an existing overlap does not emit another BeginOverlap.
	CollisionComponent->UpdateOverlaps();
	TArray<AActor*> OverlappingActors;
	CollisionComponent->GetOverlappingActors(OverlappingActors);
	for (AActor* Actor : OverlappingActors)
	{
		if (auto* Overload = Actor->FindComponentByClass<URLBossOverloadComponent>())
		{
			if (Overload->TryAbsorb(this)) { break; }
		}
	}

	return true;
}

void ARLProjectile::ReturnToPool()
{
	if (!bIsActive || bIsFadingOut)
	{
		return;
	}

	bIsFadingOut = true;
	bCanBeReflected = false;
	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Deactivate();
	VisualComponent->ResetReflectedAfterimages();
	VisualComponent->BeginFadeOut();
	SetActorTickEnabled(true);

	if (VisualComponent->HasImmediateFadeOut())
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
	const bool bWasActive = bIsActive;
	bIsActive = false;
	if (bWasActive) { FeedbackComponent->PlayPoolReturnEffect(ProjectileMesh); }
	bIsFadingOut = false;
	bIsReflected = false;
	bCanBeReflected = true;
	SpecialComponent->ResetRuntimeState();
	RallyComponent->ResetRuntimeState();
	VisualComponent->ResetRuntimeState();
	ReflectionState.RemainingPierces = 0;
	ReflectionState.ReflectionChain = 0;
	ReflectionState.bWasPerfectParried = false;
	ReflectionState.bWasCloseRangeParried = false;
	ReflectionState.bWasOverdriveReflected = false;
	ActiveDefinition = nullptr;
	ProjectileMesh->SetRelativeScale3D(DefaultProjectileMeshScale);
	ProjectileMesh->SetVisibility(true, true);
	CollisionComponent->SetSphereRadius(DefaultCollisionRadius, false);
	SetActorTickEnabled(false);
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
	ContactComponent->ResolveProjectileContact(OtherActor);
}

void ARLProjectile::HandleProjectileOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (IsValid(OtherActor))
	{
		if (auto* Overload = OtherActor->FindComponentByClass<URLBossOverloadComponent>())
		{
			// The aura is a sensor, never a normal enemy hit (including hostile bullets).
			if (Overload->OwnsVolume(OtherComponent))
			{
				Overload->TryAbsorb(this);
				return;
			}
		}
	}
	ContactComponent->ResolveProjectileContact(OtherActor);
}

void ARLProjectile::ConfigureAsExplosive()
{
	SpecialComponent->ConfigureAsExplosive();
}

bool ARLProjectile::IsGuardProjectile() const
{
	return SpecialComponent->IsGuardProjectile();
}

bool ARLProjectile::IsExplosive() const
{
	return SpecialComponent->IsExplosive();
}

bool ARLProjectile::IsRallyProjectile() const
{
	return RallyComponent->IsRallyProjectile();
}

float ARLProjectile::GetExplosionRadius() const
{
	return SpecialComponent->GetExplosionRadius();
}

float ARLProjectile::GetExplosiveFuseRemainingSeconds() const
{
	return SpecialComponent->GetFuseRemainingSeconds();
}

void ARLProjectile::ConfigureAsDelayedExplosive(AActor* TargetActor)
{
	SpecialComponent->ConfigureAsDelayedExplosive(TargetActor);
}

void ARLProjectile::ConfigureAsFake(AActor* TargetActor)
{
	SpecialComponent->ConfigureAsFake(TargetActor);
}

void ARLProjectile::ConfigureAsGuard()
{
	SpecialComponent->ConfigureAsGuard();
}

void ARLProjectile::ConfigureAsParrySplit()
{
	SpecialComponent->ConfigureAsParrySplit();
}

bool ARLProjectile::Detonate()
{
	return SpecialComponent->Detonate();
}

void ARLProjectile::ConfigureAsRally(
	AActor* FinalTarget,
	int32 InMaxRallies,
	float InSpeedMultiplierPerRally)
{
	RallyComponent->ConfigureAsRally(FinalTarget, InMaxRallies, InSpeedMultiplierPerRally);
}

void ARLProjectile::PauseMotion()
{
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Deactivate();
}

void ARLProjectile::RestartLifetime(float Duration)
{
	GetWorldTimerManager().SetTimer(
		LifetimeTimerHandle, this, &ThisClass::ReturnToPool, Duration, false);
}

void ARLProjectile::NotifyExplosion(const FVector& Location, float Radius)
{
	FeedbackComponent->MarkExplosionPlayed();
	OnExploded(Location, Radius);
}

bool ARLProjectile::TryConsumePierce(float AppliedDamage)
{
	if (!bIsReflected || ReflectionState.RemainingPierces <= 0 || AppliedDamage <= 0.0f)
	{
		return false;
	}
	--ReflectionState.RemainingPierces;
	return true;
}

