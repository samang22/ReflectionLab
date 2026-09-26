#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RLEnemyPoolSubsystem.generated.h"

class ARLEnemyCharacter;

USTRUCT()
struct FRLEnemyPoolBucket
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<ARLEnemyCharacter>> InactiveEnemies;
};

UCLASS()
class REFLECTIONLAB_API URLEnemyPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Enemy Pool")
	void PrewarmPool(TSubclassOf<ARLEnemyCharacter> EnemyClass, int32 DesiredInactiveCount);

	UFUNCTION(BlueprintCallable, Category = "Enemy Pool")
	ARLEnemyCharacter* AcquireEnemy(
		TSubclassOf<ARLEnemyCharacter> EnemyClass,
		const FTransform& SpawnTransform);

	UFUNCTION(BlueprintCallable, Category = "Enemy Pool")
	void ReleaseEnemy(ARLEnemyCharacter* Enemy);

private:
	ARLEnemyCharacter* SpawnEnemy(TSubclassOf<ARLEnemyCharacter> EnemyClass);

	UPROPERTY()
	TMap<TSubclassOf<ARLEnemyCharacter>, FRLEnemyPoolBucket> EnemyPools;
};
