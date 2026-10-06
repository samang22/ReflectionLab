#include "Combat/RLProjectileRallyComponent.h"

#include "Combat/RLProjectile.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Combat/RLProjectileVisualComponent.h"
#include "Combat/RLProjectileSpecialComponent.h"
#include "Combat/RLProjectileContactComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Enemies/RLEnemyCharacter.h"

URLProjectileRallyComponent::URLProjectileRallyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URLProjectileRallyComponent::ApplyDefinitionSettings(const URLProjectileDefinitionDataAsset& Definition)
{
	Settings.RallyVisualScale = FMath::Max(1.0f, Definition.RallyVisualScale);
	Settings.RallyMaxSpeedMultiplier = FMath::Max(1.0f, Definition.RallyMaxSpeedMultiplier);
	Settings.RallyArrivalRadius = FMath::Max(1.0f, Definition.RallyArrivalRadius);
}

void URLProjectileRallyComponent::ResetRuntimeState(float InitialSpeed)
{
	bIsRallyProjectile = false;
	bRallyFinalShot = false;
	RallyCurrentSpeed = InitialSpeed;
	RallySpeedMultiplierPerRally = 1.15f;
	RallyCount = 0;
	MaxRallies = 0;
	RallyTarget.Reset();
	RallyDamagedEnemies.Reset();
	RallyFinalTarget.Reset();
}

void URLProjectileRallyComponent::ConfigureAsRally(
	AActor* FinalTarget,
	int32 InMaxRallies,
	float InSpeedMultiplierPerRally)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->IsPoolActive() || !IsValid(FinalTarget))
	{
		return;
	}

	Projectile->SetParryEnabled(true);
	Projectile->GetSpecialComponent()->DisableBehavior();
	bIsRallyProjectile = true;
	Projectile->GetVisualComponent()->ResetReflectedAfterimages();
	Projectile->GetVisualComponent()->UpdateProjectileMaterial();
	bRallyFinalShot = false;
	RallyCount = 0;
	MaxRallies = FMath::Max(1, InMaxRallies);
	RallySpeedMultiplierPerRally = FMath::Max(1.0f, InSpeedMultiplierPerRally);
	RallyCurrentSpeed = Projectile->GetBaseSpeed();
	RallyFinalTarget = FinalTarget;
	Projectile->GetVisualMesh()->SetRelativeScale3D(
		Projectile->GetDefaultVisualScale() * FMath::Max(1.0f, Settings.RallyVisualScale));

	AActor* FirstRelayTarget = FindNextRallyTarget(Projectile->GetOwner());
	SetRallyTarget(FirstRelayTarget ? FirstRelayTarget : FinalTarget);
	Projectile->SetActorTickEnabled(true);
}

void URLProjectileRallyComponent::UpdateRally(float DeltaTime)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	(void)DeltaTime;

	AActor* Target = RallyTarget.Get();
	if (Projectile->IsReflected())
	{
		// Player returns home toward an enemy, never relay harmlessly through it.
		const ARLEnemyCharacter* TargetEnemy = Cast<ARLEnemyCharacter>(Target);
		if (!IsValid(TargetEnemy) || !TargetEnemy->IsPoolActive() || RallyDamagedEnemies.Contains(TargetEnemy))
		{
			SetRallyTarget(FindNextRallyTarget(nullptr));
			return;
		}
		// Homing can arrive already overlapping a target without a new overlap
		// event. Resolve arrival explicitly, just as hostile rally relays do.
		if (FVector::DistSquared(Projectile->GetActorLocation(), Target->GetActorLocation()) <=
			FMath::Square(FMath::Max(1.0f, Settings.RallyArrivalRadius)))
		{
			Projectile->GetContactComponent()->ResolveProjectileContact(Target);
			return;
		}
		Projectile->SetProjectileSpeed(RallyCurrentSpeed,
			(Target->GetActorLocation() - Projectile->GetActorLocation()).GetSafeNormal());
		return;
	}
	if (!IsValid(Target))
	{
		SetRallyTarget(RallyFinalTarget.Get());
		return;
	}

	if (ARLEnemyCharacter* TargetEnemy = Cast<ARLEnemyCharacter>(Target))
	{
		if (!TargetEnemy->IsPoolActive())
		{
			AActor* ReplacementTarget = FindNextRallyTarget(Projectile->GetOwner());
			SetRallyTarget(ReplacementTarget ? ReplacementTarget : RallyFinalTarget.Get());
			return;
		}

		if (FVector::DistSquared(Projectile->GetActorLocation(), Target->GetActorLocation()) <=
			FMath::Square(FMath::Max(1.0f, Settings.RallyArrivalRadius)))
		{
			AdvanceRally();
			return;
		}
	}

	const FVector TargetDirection = (Target->GetActorLocation() - Projectile->GetActorLocation()).GetSafeNormal();
	if (!TargetDirection.IsNearlyZero())
	{
		Projectile->SetProjectileSpeed(FMath::Max(1.0f, RallyCurrentSpeed), TargetDirection);
	}
}

void URLProjectileRallyComponent::AdvanceRally()
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	ARLEnemyCharacter* RelayEnemy = Cast<ARLEnemyCharacter>(RallyTarget.Get());
	if (!RelayEnemy || !RelayEnemy->IsPoolActive())
	{
		SetRallyTarget(RallyFinalTarget.Get());
		return;
	}

	++RallyCount;
	RallyCurrentSpeed = FMath::Min(
		Projectile->GetBaseSpeed() * FMath::Max(1.0f, Settings.RallyMaxSpeedMultiplier),
		FMath::Max(Projectile->GetBaseSpeed(), RallyCurrentSpeed) * RallySpeedMultiplierPerRally);
	Projectile->SetProjectileOwnerAndIgnore(RelayEnemy);

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

AActor* URLProjectileRallyComponent::FindNextRallyTarget(AActor* RelaySource) const
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	UWorld* World = Projectile->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ARLEnemyCharacter* BestTarget = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<ARLEnemyCharacter> Iterator(World); Iterator; ++Iterator)
	{
		ARLEnemyCharacter* Candidate = *Iterator;
		if (!IsValid(Candidate) || !Candidate->IsPoolActive() || Candidate == RelaySource ||
			(Projectile->IsReflected() && RallyDamagedEnemies.Contains(Candidate)))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(
			Projectile->GetActorLocation(),
			Candidate->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

void URLProjectileRallyComponent::SetRallyTarget(AActor* NewTarget)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!IsValid(NewTarget))
	{
		RallyTarget.Reset();
		if (!Projectile->IsReflected() || !RallyDamagedEnemies.IsEmpty()) { Projectile->ReturnToPool(); }
		return;
	}

	RallyTarget = NewTarget;
	bRallyFinalShot = NewTarget == RallyFinalTarget.Get();
	const FVector TargetDirection = (NewTarget->GetActorLocation() - Projectile->GetActorLocation()).GetSafeNormal();
	Projectile->SetProjectileSpeed(FMath::Max(1.0f, RallyCurrentSpeed), TargetDirection);
}

bool URLProjectileRallyComponent::HasDamagedEnemy(const ARLEnemyCharacter* Enemy) const
{
	return RallyDamagedEnemies.Contains(Enemy);
}

void URLProjectileRallyComponent::RecordEnemyHit(ARLEnemyCharacter* Enemy)
{
	RallyDamagedEnemies.AddUnique(Enemy);
}

void URLProjectileRallyComponent::DisableRally(bool bClearTargets)
{
	bIsRallyProjectile = false;
	if (bClearTargets)
	{
		bRallyFinalShot = false;
		RallyTarget.Reset();
		RallyFinalTarget.Reset();
	}
}

void URLProjectileRallyComponent::ResetTargetsForReflection()
{
	bRallyFinalShot = false;
	RallyTarget.Reset();
	RallyDamagedEnemies.Reset();
	RallyFinalTarget.Reset();
}

void URLProjectileRallyComponent::BeginReflectedReturn(float Speed)
{
	if (bIsRallyProjectile)
	{
		RallyCurrentSpeed = Speed;
		SetRallyTarget(FindNextRallyTarget(nullptr));
	}
}

void URLProjectileRallyComponent::AdvanceAfterEnemyHit()
{
	SetRallyTarget(FindNextRallyTarget(nullptr));
}
