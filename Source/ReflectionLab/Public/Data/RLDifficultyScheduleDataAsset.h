#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLDifficultyScheduleDataAsset.generated.h"

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLDifficultyPhase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	FName PhaseName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float StartTimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Spawning", meta = (ClampMin = "0"))
	int32 MaxAliveEnemies = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Spawning", meta = (ClampMin = "1"))
	int32 SpawnBatchSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Spawning", meta = (ClampMin = "0.1"))
	float SpawnInterval = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Spawning", meta = (ClampMin = "0.0"))
	float MinimumSpawnDistance = 750.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Spawning", meta = (ClampMin = "0.0"))
	float MaximumSpawnDistance = 1150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Combat", meta = (ClampMin = "0.1"))
	float AttackIntervalMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Combat", meta = (ClampMin = "0"))
	int32 ShotsPerBurstOverride = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Combat", meta = (ClampMin = "0.1"))
	float TimeBetweenShotsMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Special Projectiles", meta = (ClampMin = "0"))
	int32 ExplosiveShotInterval = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Special Projectiles", meta = (ClampMin = "0"))
	int32 RallyShotInterval = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Special Projectiles", meta = (ClampMin = "1"))
	int32 RallyRelayCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Special Projectiles", meta = (ClampMin = "1.0"))
	float RallySpeedMultiplierPerRelay = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	bool bBreatherPhase = false;
};

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLDifficultyScheduleDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Difficulty", meta = (ClampMin = "1.0"))
	float DurationSeconds = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Difficulty")
	TArray<FRLDifficultyPhase> Phases;

	const FRLDifficultyPhase* FindPhaseAtTime(float ElapsedSeconds, int32& OutPhaseIndex) const;
};
