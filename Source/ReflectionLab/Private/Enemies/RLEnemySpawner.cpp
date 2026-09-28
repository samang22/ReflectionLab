#include "Enemies/RLEnemySpawner.h"

#include "Components/SceneComponent.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/RLEnemyPoolSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ARLEnemySpawner::ARLEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ARLEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!EnemyClass || !GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyClass is not assigned on %s."), *GetName());
		return;
	}

	if (URLEnemyPoolSubsystem* PoolSubsystem =
		GetWorld()->GetSubsystem<URLEnemyPoolSubsystem>())
	{
		PoolSubsystem->PrewarmPool(
			EnemyClass,
			FMath::Max(PrewarmCount, MaxAliveEnemies));
	}

	if (bAutoStartSpawning)
	{
		const int32 EnemiesToSpawn = FMath::Min(InitialEnemyCount, MaxAliveEnemies);
		for (int32 Index = 0; Index < EnemiesToSpawn; ++Index)
		{
			SpawnEnemy();
		}

		StartSpawning();
	}
}

void ARLEnemySpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopSpawning();
	ActiveEnemies.Empty();
	Super::EndPlay(EndPlayReason);
}

void ARLEnemySpawner::StartSpawning()
{
	if (!EnemyClass || !GetWorld() || GetWorldTimerManager().IsTimerActive(SpawnTimerHandle))
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&ThisClass::HandleSpawnTimer,
		FMath::Max(0.1f, SpawnInterval),
		true,
		FMath::Max(0.0f, InitialSpawnDelay));
}

void ARLEnemySpawner::HandleSpawnTimer()
{
	SpawnBatch(CurrentSpawnBatchSize);
}

void ARLEnemySpawner::StopSpawning()
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
}

ARLEnemyCharacter* ARLEnemySpawner::SpawnEnemy()
{
	CleanupInactiveEnemies();
	if (!EnemyClass || !GetWorld() || ActiveEnemies.Num() >= MaxAliveEnemies)
	{
		return nullptr;
	}

	FTransform SpawnTransform;
	if (!FindSpawnTransform(SpawnTransform))
	{
		return nullptr;
	}

	if (URLEnemyPoolSubsystem* PoolSubsystem =
		GetWorld()->GetSubsystem<URLEnemyPoolSubsystem>())
	{
		if (ARLEnemyCharacter* Enemy =
			PoolSubsystem->AcquireEnemy(EnemyClass, SpawnTransform))
		{
			if (bHasActiveDifficultyPhase)
			{
				Enemy->ApplyDifficultyPhase(ActiveDifficultyPhase);
			}
			ActiveEnemies.Add(Enemy);
			return Enemy;
		}
	}

	return nullptr;
}

int32 ARLEnemySpawner::SpawnBatch(int32 RequestedCount)
{
	const int32 SafeRequestedCount = FMath::Max(0, RequestedCount);
	int32 SpawnedCount = 0;
	for (int32 SpawnIndex = 0; SpawnIndex < SafeRequestedCount; ++SpawnIndex)
	{
		if (!SpawnEnemy())
		{
			break;
		}
		++SpawnedCount;
	}

	return SpawnedCount;
}

void ARLEnemySpawner::ApplyDifficultyPhase(const FRLDifficultyPhase& DifficultyPhase)
{
	ActiveDifficultyPhase = DifficultyPhase;
	bHasActiveDifficultyPhase = true;
	CurrentSpawnBatchSize = FMath::Max(1, DifficultyPhase.SpawnBatchSize);
	MaxAliveEnemies = FMath::Max(0, DifficultyPhase.MaxAliveEnemies);
	MinimumPlayerDistance = FMath::Max(0.0f, DifficultyPhase.MinimumSpawnDistance);
	SpawnRadius = FMath::Max(
		MinimumPlayerDistance,
		DifficultyPhase.MaximumSpawnDistance);

	CleanupInactiveEnemies();
	for (const TWeakObjectPtr<ARLEnemyCharacter>& EnemyPtr : ActiveEnemies)
	{
		if (ARLEnemyCharacter* Enemy = EnemyPtr.Get())
		{
			Enemy->ApplyDifficultyPhase(DifficultyPhase);
		}
	}

	SetSpawnInterval(DifficultyPhase.SpawnInterval);
	if (DifficultyPhase.bBreatherPhase || MaxAliveEnemies <= 0)
	{
		StopSpawning();
	}
	else
	{
		StartSpawning();
	}
}

void ARLEnemySpawner::SetMaxAliveEnemies(int32 NewMaxAliveEnemies)
{
	MaxAliveEnemies = FMath::Max(0, NewMaxAliveEnemies);
}

void ARLEnemySpawner::SetSpawnInterval(float NewSpawnInterval)
{
	const bool bWasSpawning = GetWorld() &&
		GetWorldTimerManager().IsTimerActive(SpawnTimerHandle);
	SpawnInterval = FMath::Max(0.1f, NewSpawnInterval);
	if (bWasSpawning)
	{
		StopSpawning();
		StartSpawning();
	}
}

void ARLEnemySpawner::SetSpawnDistanceRange(
	float NewMinimumDistance,
	float NewMaximumDistance)
{
	MinimumPlayerDistance = FMath::Max(0.0f, NewMinimumDistance);
	SpawnRadius = FMath::Max(MinimumPlayerDistance, NewMaximumDistance);
}

void ARLEnemySpawner::CleanupInactiveEnemies()
{
	for (auto Iterator = ActiveEnemies.CreateIterator(); Iterator; ++Iterator)
	{
		ARLEnemyCharacter* Enemy = Iterator->Get();
		if (!IsValid(Enemy) || !Enemy->IsPoolActive())
		{
			Iterator.RemoveCurrent();
		}
	}
}

bool ARLEnemySpawner::FindSpawnTransform(FTransform& OutSpawnTransform) const
{
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Origin = GetActorLocation();
	const int32 Attempts = FMath::Max(1, SpawnLocationAttempts);

	for (int32 Attempt = 0; Attempt < Attempts; ++Attempt)
	{
		const FVector2D Offset = FMath::RandPointInCircle(FMath::Max(0.0f, SpawnRadius));
		const FVector Candidate = Origin + FVector(Offset.X, Offset.Y, SpawnHeightOffset);
		if (PlayerPawn && FVector::DistSquared2D(Candidate, PlayerPawn->GetActorLocation()) <
			FMath::Square(MinimumPlayerDistance))
		{
			continue;
		}

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemySpawner), false, this);
		const bool bBlocked = GetWorld()->OverlapBlockingTestByChannel(
			Candidate,
			FQuat::Identity,
			ECC_Pawn,
			FCollisionShape::MakeCapsule(42.0f, 96.0f),
			QueryParams);
		if (bBlocked)
		{
			continue;
		}

		const FRotator FacingRotation = PlayerPawn
			? (PlayerPawn->GetActorLocation() - Candidate).Rotation()
			: GetActorRotation();
		OutSpawnTransform = FTransform(
			FRotator(0.0f, FacingRotation.Yaw, 0.0f),
			Candidate);
		return true;
	}

	return false;
}
