#include "Enemies/RLEnemyPoolSubsystem.h"

#include "Enemies/RLEnemyCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void URLEnemyPoolSubsystem::PrewarmPool(
	TSubclassOf<ARLEnemyCharacter> EnemyClass,
	int32 DesiredInactiveCount)
{
	if (!EnemyClass || DesiredInactiveCount <= 0)
	{
		return;
	}

	FRLEnemyPoolBucket& Pool = EnemyPools.FindOrAdd(EnemyClass);
	Pool.InactiveEnemies.RemoveAll(
		[](const ARLEnemyCharacter* Enemy)
		{
			return !IsValid(Enemy);
		});

	while (Pool.InactiveEnemies.Num() < DesiredInactiveCount)
	{
		ARLEnemyCharacter* Enemy = SpawnEnemy(EnemyClass);
		if (!Enemy)
		{
			break;
		}

		Pool.InactiveEnemies.Add(Enemy);
	}
}

ARLEnemyCharacter* URLEnemyPoolSubsystem::AcquireEnemy(
	TSubclassOf<ARLEnemyCharacter> EnemyClass,
	const FTransform& SpawnTransform)
{
	if (!EnemyClass)
	{
		return nullptr;
	}

	FRLEnemyPoolBucket& Pool = EnemyPools.FindOrAdd(EnemyClass);
	ARLEnemyCharacter* Enemy = nullptr;
	while (Pool.InactiveEnemies.Num() > 0 && !IsValid(Enemy))
	{
		Enemy = Pool.InactiveEnemies.Pop(EAllowShrinking::No);
	}

	if (!Enemy)
	{
		Enemy = SpawnEnemy(EnemyClass);
	}

	if (Enemy)
	{
		Enemy->ActivateFromPool(SpawnTransform);
	}

	return Enemy;
}

void URLEnemyPoolSubsystem::ReleaseEnemy(ARLEnemyCharacter* Enemy)
{
	if (!IsValid(Enemy))
	{
		return;
	}

	Enemy->DeactivateForPool();
	EnemyPools.FindOrAdd(Enemy->GetClass()).InactiveEnemies.AddUnique(Enemy);
}

ARLEnemyCharacter* URLEnemyPoolSubsystem::SpawnEnemy(
	TSubclassOf<ARLEnemyCharacter> EnemyClass)
{
	UWorld* World = GetWorld();
	if (!World || !EnemyClass)
	{
		return nullptr;
	}

	const FTransform InactiveTransform(FRotator::ZeroRotator, FVector::ZeroVector);
	ARLEnemyCharacter* Enemy = World->SpawnActorDeferred<ARLEnemyCharacter>(
		EnemyClass,
		InactiveTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Enemy)
	{
		return nullptr;
	}

	Enemy->bIsPoolActive = false;
	UGameplayStatics::FinishSpawningActor(Enemy, InactiveTransform);
	Enemy->DeactivateForPool();
	return Enemy;
}
