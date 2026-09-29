#pragma once

#include "CoreMinimal.h"
#include "Data/RLDifficultyScheduleDataAsset.h"
#include "GameFramework/Actor.h"
#include "RLEnemySpawner.generated.h"

class ARLEnemyCharacter;
class USceneComponent;

UCLASS()
class REFLECTIONLAB_API ARLEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	ARLEnemySpawner();

	UFUNCTION(BlueprintCallable, Category = "Enemy Spawner")
	void StartSpawning();

	UFUNCTION(BlueprintCallable, Category = "Enemy Spawner")
	void StopSpawning();

	UFUNCTION(BlueprintCallable, Category = "Enemy Spawner")
	ARLEnemyCharacter* SpawnEnemy();

	UFUNCTION(BlueprintCallable, Category = "Enemy Spawner")
	int32 SpawnBatch(int32 RequestedCount);

	UFUNCTION(BlueprintCallable, Category = "Enemy Spawner|Difficulty")
	void ApplyDifficultyPhase(const FRLDifficultyPhase& DifficultyPhase);

	UFUNCTION(BlueprintCallable, Category = "Enemy Spawner|Wave")
	void ConfigureWave(const FRLWaveDefinition& WaveDefinition);

	UFUNCTION(BlueprintCallable, Category = "Enemy Spawner|Difficulty")
	void SetMaxAliveEnemies(int32 NewMaxAliveEnemies);

	UFUNCTION(BlueprintCallable, Category = "Enemy Spawner|Difficulty")
	void SetSpawnInterval(float NewSpawnInterval);

	UFUNCTION(BlueprintCallable, Category = "Enemy Spawner|Difficulty")
	void SetSpawnDistanceRange(float NewMinimumDistance, float NewMaximumDistance);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy Spawner")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner")
	TSubclassOf<ARLEnemyCharacter> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner")
	bool bAutoStartSpawning = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner", meta = (ClampMin = "0"))
	int32 PrewarmCount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner", meta = (ClampMin = "1"))
	int32 MaxAliveEnemies = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner", meta = (ClampMin = "0"))
	int32 InitialEnemyCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner", meta = (ClampMin = "0.1"))
	float SpawnInterval = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner", meta = (ClampMin = "0.0"))
	float InitialSpawnDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner", meta = (ClampMin = "0.0"))
	float SpawnRadius = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner")
	float SpawnHeightOffset = 96.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner", meta = (ClampMin = "0.0"))
	float MinimumPlayerDistance = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Spawner", meta = (ClampMin = "1"))
	int32 SpawnLocationAttempts = 12;

private:
	void HandleSpawnTimer();
	void CleanupInactiveEnemies();
	bool FindSpawnTransform(FTransform& OutSpawnTransform) const;

	FTimerHandle SpawnTimerHandle;
	TSet<TWeakObjectPtr<ARLEnemyCharacter>> ActiveEnemies;
	FRLDifficultyPhase ActiveDifficultyPhase;
	FRLWaveDefinition ActiveWaveDefinition;
	int32 CurrentSpawnBatchSize = 1;
	int32 SpawnSequenceCount = 0;
	bool bHasActiveDifficultyPhase = false;
	bool bHasActiveWaveDefinition = false;
};
