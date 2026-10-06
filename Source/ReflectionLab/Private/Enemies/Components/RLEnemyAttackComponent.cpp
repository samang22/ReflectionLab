#include "Enemies/Components/RLEnemyAttackComponent.h"

#include "Combat/RLExpandingRingAttack.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Data/RLEnemyCombatRow.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/Components/RLEnemyMovementComponent.h"
#include "Engine/World.h"
#include "Framework/GameMode/RLGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

URLEnemyAttackComponent::URLEnemyAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URLEnemyAttackComponent::Configure(const FRLEnemyCombatRow& Config,
	TSubclassOf<ARLProjectile> NewProjectileClass, USceneComponent* NewMuzzlePoint, bool bAutoStart)
{
	StopFiring();
	ProjectileClass = NewProjectileClass;
	MuzzlePoint = NewMuzzlePoint;
	bAutoStartFiring = bAutoStart;
	AttackInterval = FMath::Max(0.1f, Config.AttackInterval);
	ShotsPerBurst = FMath::Max(1, Config.ShotsPerBurst);
	TimeBetweenShots = FMath::Max(0.01f, Config.TimeBetweenShots);
	InitialFireDelay = FMath::Max(0.0f, Config.InitialFireDelay);
	CacheBaseCombatValues();
	if (ProjectileClass && GetWorld())
	{
		if (URLProjectilePoolSubsystem* Pool = GetWorld()->GetSubsystem<URLProjectilePoolSubsystem>())
		{
			Pool->PrewarmPool(ProjectileClass, FMath::Max(0, Config.ProjectilePoolPrewarmCount));
		}
	}
}

void URLEnemyAttackComponent::ActivateForPool()
{
	StopFiring();
	bTutorialCombatControlled = false;
	ShotsFiredSinceActivation = 0;
	RandomizePatternStart();
	NextRingAttackTime = 0.0;
	if (bAutoStartFiring) { StartFiring(); }
}

void URLEnemyAttackComponent::DeactivateForPool()
{
	StopFiring();
	bTutorialCombatControlled = false;
	ShotsFiredSinceActivation = 0;
	PatternShotOffset = 0;
	RingAttackRule = FRLRingAttackSpawnRule();
	NextRingAttackTime = 0.0;
}

void URLEnemyAttackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopFiring();
	OnFireShot.Unbind();
	OnShotSpawned.Unbind();
	Super::EndPlay(EndPlayReason);
}

void URLEnemyAttackComponent::CacheBaseCombatValues()
{
	BaseAttackInterval = FMath::Max(0.1f, AttackInterval);
	BaseShotsPerBurst = FMath::Max(1, ShotsPerBurst);
	BaseTimeBetweenShots = FMath::Max(0.01f, TimeBetweenShots);
}

void URLEnemyAttackComponent::ApplyDifficultyPhase(const FRLDifficultyPhase& DifficultyPhase)
{
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy) { return; }

	AttackInterval = BaseAttackInterval *
		FMath::Max(0.1f, DifficultyPhase.AttackIntervalMultiplier);
	ShotsPerBurst = DifficultyPhase.ShotsPerBurstOverride > 0
		? DifficultyPhase.ShotsPerBurstOverride
		: BaseShotsPerBurst;
	TimeBetweenShots = BaseTimeBetweenShots *
		FMath::Max(0.1f, DifficultyPhase.TimeBetweenShotsMultiplier);
	DefaultProjectileDefinition = DifficultyPhase.DefaultProjectileDefinition;
	ProjectileRules = DifficultyPhase.ProjectileRules;
	bUseWeightedPatterns = DifficultyPhase.bUseWeightedPatterns;
	DefaultProjectileWeight = DifficultyPhase.DefaultProjectileWeight;
	RingAttackRule = DifficultyPhase.RingAttack;
	if (DifficultyPhase.bBreatherPhase)
	{
		RingAttackRule.ShotInterval = 0;
		RingAttackRule.Weight = 0;
	}
	AttackVariation = DifficultyPhase.AttackVariation;
	RandomizePatternStart();

	if (Enemy->IsPoolActive() && bAutoStartFiring)
	{
		StopFiring();
		StartFiring();
	}
}

void URLEnemyAttackComponent::ApplyWaveDefinition(const FRLWaveDefinition& WaveDefinition)
{
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy) { return; }

	AttackInterval = BaseAttackInterval *
		FMath::Max(0.1f, WaveDefinition.AttackIntervalMultiplier);
	ShotsPerBurst = WaveDefinition.ShotsPerBurstOverride > 0
		? WaveDefinition.ShotsPerBurstOverride
		: BaseShotsPerBurst;
	TimeBetweenShots = BaseTimeBetweenShots *
		FMath::Max(0.1f, WaveDefinition.TimeBetweenShotsMultiplier);
	DefaultProjectileDefinition = WaveDefinition.DefaultProjectileDefinition;
	ProjectileRules = WaveDefinition.ProjectileRules;
	bUseWeightedPatterns = WaveDefinition.bUseWeightedPatterns;
	DefaultProjectileWeight = WaveDefinition.DefaultProjectileWeight;
	RingAttackRule = WaveDefinition.RingAttack;
	AttackVariation = WaveDefinition.AttackVariation;
	RandomizePatternStart();

	if (Enemy->IsPoolActive() && bAutoStartFiring)
	{
		StopFiring();
		StartFiring();
	}
}

void URLEnemyAttackComponent::StartFiring()
{
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy) { return; }

	if (!GetWorld() || bFiring || !Enemy->IsPoolActive() || bTutorialCombatControlled)
	{
		return;
	}

	bFiring = true;
	const float Jitter = UsesAttackVariation() && FMath::IsFinite(AttackVariation.InitialDelayJitterSeconds)
		? FMath::FRandRange(0.0f, FMath::Max(0.0f, AttackVariation.InitialDelayJitterSeconds)) : 0.0f;
	const float Delay = FMath::Max(0.01f, InitialFireDelay + Jitter);
	GetWorld()->GetTimerManager().SetTimer(
		AttackTimerHandle,
		this,
		&ThisClass::BeginBurst,
		Delay,
		false);
}

void URLEnemyAttackComponent::StopFiring()
{
	if (URLEnemyMovementComponent* Movement = GetOwner()->FindComponentByClass<URLEnemyMovementComponent>())
	{
		Movement->CancelReposition();
	}
	bFiring = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackTimerHandle);
		World->GetTimerManager().ClearTimer(BurstTimerHandle);
	}
	RemainingShotsInBurst = 0;
}

void URLEnemyAttackComponent::BeginBurst()
{
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy || !GetWorld()) { return; }

	if (!bFiring || !Enemy->IsPoolActive() || bTutorialCombatControlled || RemainingShotsInBurst > 0)
	{
		return;
	}

	BurstStartTime = GetWorld()->GetTimeSeconds();
	if (URLEnemyMovementComponent* Movement = Enemy->FindComponentByClass<URLEnemyMovementComponent>())
	{
		if (Movement->IsRepositioning())
		{
			GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, this, &ThisClass::BeginBurst, 0.1f, false);
			return;
		}
	}
	CurrentBurstInterval = FRLEnemyAttackVariation::SampleInterval(AttackInterval,
		UsesAttackVariation() ? AttackVariation.AttackIntervalJitterRatio : 0.0f, 0.1f);
	RemainingShotsInBurst = FMath::Max(1, ShotsPerBurst);
	FireNextShot();
}

void URLEnemyAttackComponent::FireNextShot()
{
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy || !GetWorld()) { return; }

	if (!bFiring || !Enemy->IsPoolActive() || bTutorialCombatControlled || RemainingShotsInBurst <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(BurstTimerHandle);
		return;
	}

	--RemainingShotsInBurst;
	OnFireShot.ExecuteIfBound();
	// Fire can synchronously end a round and stop this enemy's timers.
	if (!bFiring || !IsValid(Enemy) || !GetWorld() ||
		!Enemy->IsPoolActive() || bTutorialCombatControlled) { return; }

	if (RemainingShotsInBurst > 0)
	{
		const float Delay = FRLEnemyAttackVariation::SampleInterval(TimeBetweenShots,
			UsesAttackVariation() ? AttackVariation.BurstIntervalJitterRatio : 0.0f, 0.01f);
		GetWorld()->GetTimerManager().SetTimer(BurstTimerHandle, this, &ThisClass::FireNextShot, Delay, false);
	}
	else
	{
		// Account for actual randomized shot gaps so jitter applies to the whole
		// start-to-start interval, not just the pause. Long bursts never overlap.
		const float Elapsed = static_cast<float>(GetWorld()->GetTimeSeconds() - BurstStartTime);
		const float Delay = FMath::Max(0.1f, CurrentBurstInterval - Elapsed);
		if (URLEnemyMovementComponent* Movement = Enemy->FindComponentByClass<URLEnemyMovementComponent>())
		{
			Movement->BeginReposition();
		}
		GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, this, &ThisClass::BeginBurst, Delay, false);
	}
}

bool URLEnemyAttackComponent::UsesAttackVariation() const
{
	const ARLGameModeBase* GameMode = Cast<ARLGameModeBase>(UGameplayStatics::GetGameMode(this));
	return !bTutorialCombatControlled && (!GameMode || !GameMode->IsCurrentRoundTutorial());
}

void URLEnemyAttackComponent::RandomizePatternStart()
{
	PatternShotOffset = 0;
	if (bUseWeightedPatterns) { return; }
	if (!AttackVariation.bRandomizePatternStart || !UsesAttackVariation()) { return; }
	TArray<int32> Intervals;
	Intervals.Add(RingAttackRule.ShotInterval);
	for (const FRLProjectileSpawnRule& Rule : ProjectileRules)
	{
		if (Rule.ProjectileDefinition) { Intervals.Add(Rule.ShotInterval); }
	}
	PatternShotOffset = FMath::RandHelper(FRLEnemyAttackVariation::GetPatternCycleLength(Intervals));
}

void URLEnemyAttackComponent::Fire()
{
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy) { return; }

	if (!Enemy->IsPoolActive() || bTutorialCombatControlled || !GetWorld())
	{
		return;
	}

	const ARLGameModeBase* GameMode = Cast<ARLGameModeBase>(UGameplayStatics::GetGameMode(this));
	if (GameMode && GameMode->GetRunState() != ERLRunState::PlayingRound) { return; }
	if (bUseWeightedPatterns && (!GameMode || !GameMode->IsCurrentRoundTutorial()))
	{
		FireWeightedPattern();
		return;
	}
	// Bound legacy counters so unusually long sessions cannot overflow.
	ShotsFiredSinceActivation = ShotsFiredSinceActivation >= MAX_int32 - 4096
		? 1 : ShotsFiredSinceActivation + 1;
	const int32 PatternShotNumber = ShotsFiredSinceActivation + (UsesAttackVariation() ? PatternShotOffset : 0);
	// Emit alongside the selected projectile so matching ring intervals cannot
	// permanently suppress existing special-projectile rules.
	if ((!GameMode || !GameMode->IsCurrentRoundTutorial()) &&
		RingAttackRule.MatchesShot(PatternShotNumber))
	{
		SpawnRingAttack();
	}
	const FRLProjectileSpawnRule* SelectedRule = nullptr;
	for (const FRLProjectileSpawnRule& Rule : ProjectileRules)
	{
		if (Rule.ProjectileDefinition && Rule.ShotInterval > 0 &&
			PatternShotNumber % Rule.ShotInterval == 0)
		{
			SelectedRule = &Rule;
			break;
		}
	}

	SpawnProjectile(
		SelectedRule ? SelectedRule->ProjectileDefinition.Get() : DefaultProjectileDefinition.Get(),
		SelectedRule ? SelectedRule->ShotPattern : ERLShotPattern::Single,
		SelectedRule ? SelectedRule->CrossLateralOffset : 90.0f,
		SelectedRule ? SelectedRule->CrossTargetOffset : 110.0f);
}

void URLEnemyAttackComponent::FireWeightedPattern()
{
	UWorld* World = GetWorld();
	if (!World) { return; }
	const double Now = World->GetTimeSeconds();
	const int64 DefaultWeight = IsValid(DefaultProjectileDefinition)
		? FMath::Max(0, DefaultProjectileWeight) : 0;
	const int64 RingWeight = Now >= NextRingAttackTime ? FMath::Max(0, RingAttackRule.Weight) : 0;
	int64 TotalWeight = DefaultWeight + RingWeight;
	for (const FRLProjectileSpawnRule& Rule : ProjectileRules)
	{
		if (IsValid(Rule.ProjectileDefinition)) { TotalWeight += FMath::Max(0, Rule.Weight); }
	}
	if (TotalWeight <= 0) { return; }
	int64 Ticket = FMath::RandRange(static_cast<int64>(0), TotalWeight - 1);
	if (Ticket < DefaultWeight)
	{
		SpawnProjectile(DefaultProjectileDefinition.Get());
		return;
	}
	Ticket -= DefaultWeight;
	for (const FRLProjectileSpawnRule& Rule : ProjectileRules)
	{
		const int64 Weight = IsValid(Rule.ProjectileDefinition) ? FMath::Max(0, Rule.Weight) : 0;
		if (Ticket < Weight)
		{
			SpawnProjectile(Rule.ProjectileDefinition.Get(), Rule.ShotPattern,
				Rule.CrossLateralOffset, Rule.CrossTargetOffset);
			return;
		}
		Ticket -= Weight;
	}
	if (RingWeight > 0 && Ticket < RingWeight)
	{
		// A ring replaces this shot; it is never emitted alongside a projectile.
		const float Cooldown = FMath::IsFinite(RingAttackRule.MinimumIntervalSeconds)
			? FMath::Max(0.0f, RingAttackRule.MinimumIntervalSeconds) : 20.0f;
		if (SpawnRingAttack()) { NextRingAttackTime = Now + Cooldown; }
	}
}

ARLExpandingRingAttack* URLEnemyAttackComponent::SpawnRingAttack(bool bAutoStart)
{
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy) { return nullptr; }

	UWorld* World = GetWorld();
	if (!World || !UGameplayStatics::GetPlayerPawn(this, 0)) { return nullptr; }
	// Start at the feet, not the muzzle. The visibility trace aligns to the
	// arena floor; capsule bottom is the fallback if the trace misses.
	FVector GroundLocation = Enemy->GetActorLocation() - FVector(0.0f, 0.0f,
		Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RingAttackGroundTrace), false, Enemy);
	QueryParams.AddIgnoredActor(UGameplayStatics::GetPlayerPawn(this, 0));
	if (World->LineTraceSingleByChannel(GroundHit, Enemy->GetActorLocation(),
		GroundLocation - FVector(0.0f, 0.0f, 200.0f), ECC_Visibility, QueryParams))
	{
		GroundLocation = GroundHit.ImpactPoint;
	}
	const FTransform SpawnTransform(FRotator::ZeroRotator, GroundLocation);
	ARLExpandingRingAttack* Ring = World->SpawnActorDeferred<ARLExpandingRingAttack>(
		ARLExpandingRingAttack::StaticClass(), SpawnTransform, Enemy, Enemy,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Ring) { return nullptr; }
	Ring->AttackData = RingAttackRule.AttackData;
	Ring->bAutoStart = bAutoStart;
	UGameplayStatics::FinishSpawningActor(Ring, SpawnTransform);
	if (!IsValid(Ring) || Ring->IsActorBeingDestroyed()) { return nullptr; }
	OnShotSpawned.ExecuteIfBound();
	return Ring;
}

bool URLEnemyAttackComponent::SpawnProjectile(
	URLProjectileDefinitionDataAsset* ProjectileDefinition,
	ERLShotPattern ShotPattern,
	float CrossLateralOffset,
	float CrossTargetOffset)
{
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy) { return false; }

	if (!ProjectileClass || !MuzzlePoint || !GetWorld() || !ProjectileDefinition)
	{
		return false;
	}

	APawn* TargetPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	URLProjectilePoolSubsystem* PoolSubsystem =
		GetWorld()->GetSubsystem<URLProjectilePoolSubsystem>();
	if (!TargetPawn || !PoolSubsystem)
	{
		return false;
	}

	const FVector SpawnLocation = MuzzlePoint->GetComponentLocation();
	const FRotator SpawnRotation =
		(TargetPawn->GetActorLocation() - SpawnLocation).Rotation();
	auto SpawnConfiguredProjectile = [
		this,
		Enemy,
		PoolSubsystem,
		ProjectileDefinition,
		TargetPawn](const FTransform& SpawnTransform)
	{
		ARLProjectile* Projectile = PoolSubsystem->AcquireProjectile(
			ProjectileClass, SpawnTransform, Enemy, Enemy);
		if (Projectile)
		{
			Projectile->InitializeFromDefinition(ProjectileDefinition, TargetPawn);
		}
		return Projectile != nullptr;
	};

	if (ShotPattern == ERLShotPattern::Cross)
	{
		bool bSpawnedAny = false;
		const FVector RightDirection = SpawnRotation.RotateVector(FVector::RightVector);
		for (const float Side : {-1.0f, 1.0f})
		{
			const FVector CrossSpawnLocation = SpawnLocation +
				RightDirection * Side * FMath::Max(0.0f, CrossLateralOffset);
			const FVector CrossTargetLocation = TargetPawn->GetActorLocation() -
				RightDirection * Side * FMath::Max(0.0f, CrossTargetOffset);
			const FVector CrossDirection =
				(CrossTargetLocation - CrossSpawnLocation).GetSafeNormal();
			bSpawnedAny |= SpawnConfiguredProjectile(
				FTransform(CrossDirection.Rotation(), CrossSpawnLocation));
		}
		if (bSpawnedAny)
		{
			OnShotSpawned.ExecuteIfBound();
		}
		return bSpawnedAny;
	}

	const bool bSpawned = SpawnConfiguredProjectile(FTransform(SpawnRotation, SpawnLocation));
	if (bSpawned)
	{
		OnShotSpawned.ExecuteIfBound();
	}
	return bSpawned;
}

void URLEnemyAttackComponent::SetTutorialCombatControlled(bool bControlled)
{
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy) { return; }

	bTutorialCombatControlled = bControlled;
	StopFiring();
	if (!bTutorialCombatControlled && Enemy->IsPoolActive() && bAutoStartFiring)
	{
		StartFiring();
	}
}

bool URLEnemyAttackComponent::FireTutorialProjectile(
	URLProjectileDefinitionDataAsset* ProjectileDefinition)
{
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy) { return false; }

	return Enemy->IsPoolActive() && bTutorialCombatControlled &&
		SpawnProjectile(ProjectileDefinition);
}

ARLExpandingRingAttack* URLEnemyAttackComponent::FireTutorialRingAttack()
{
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy) { return nullptr; }

	return Enemy->IsPoolActive() && bTutorialCombatControlled ? SpawnRingAttack(false) : nullptr;
}

