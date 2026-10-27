#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLRunDefinitionDataAsset.generated.h"

class URLDifficultyScheduleDataAsset;
class URLRunRewardDataAsset;
class ARLEnemyCharacter;
class URLArenaExperimentDataAsset;

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLEnemyHealthMix
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) float NormalHealth = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) float ReinforcedHealth = 4;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) float HeavyHealth = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) float ReinforcedRatio = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) float HeavyRatio = 0;
	float GetHealthForSpawn(int32 Index, int32 Count) const;
};

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLRoundDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
	FName RoundName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
	bool bIsTutorial = false;

	// Retained for existing asset compatibility. Wave completion now ends a round.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deprecated", meta = (DeprecatedProperty, DeprecationMessage = "Rounds now end after the final wave."))
	float DurationSeconds = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round", meta = (ClampMin = "0.0"))
	float IntermissionDurationSeconds = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Round")
	TObjectPtr<URLDifficultyScheduleDataAsset> DifficultySchedule;

	// These belong to this displayed round, never to an index in a second map.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Round|Experiment")
	TObjectPtr<URLArenaExperimentDataAsset> Experiment;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Round|Enemies")
	TSubclassOf<ARLEnemyCharacter> EnemyClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Round|Enemies", meta=(ClampMin="0", ToolTip="0 uses the wave enemy count multiplied by EnemyCountMultiplier."))
	int32 EnemyCountOverride = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Round|Enemies", meta=(ClampMin="1"))
	int32 EnemyCountMultiplier = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Round|Enemies")
	bool bSingleWave = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Round|Enemies")
	bool bOverrideEnemyHealth = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Round|Enemies", meta=(EditCondition="bOverrideEnemyHealth", EditConditionHides))
	FRLEnemyHealthMix EnemyHealth;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Round|Combat", meta=(ClampMin="0.1"))
	float BaseAttackTimingMultiplier = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Cleanup")
	bool bClearEnemiesOnComplete = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Cleanup")
	bool bClearProjectilesOnComplete = true;
};

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLRunDefinitionDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// Migration-only source fields. Runtime uses FRLRoundDefinition exclusively.
	UPROPERTY()
	TMap<int32, TObjectPtr<URLArenaExperimentDataAsset>> RoundExperiments;
	// Displayed rounds. Custom enemy classes (boss encounters) retain their own health.
	UPROPERTY()
	TMap<int32, FRLEnemyHealthMix> RoundEnemyHealthMixes;

	const FRLEnemyHealthMix* GetEnemyHealthMix(int32 RoundNumber) const
	{
		const FRLEnemyHealthMix* Result = nullptr;
		int32 LatestRound = 0;
		for (const auto& Entry : RoundEnemyHealthMixes)
		{
			if (Entry.Key > LatestRound && Entry.Key <= RoundNumber)
			{
				LatestRound = Entry.Key;
				Result = &Entry.Value;
			}
		}
		return Result;
	}
	// Uses displayed main-game round numbers, after wave splitting; tutorial is excluded.
	UPROPERTY()
	TMap<int32, TSubclassOf<ARLEnemyCharacter>> RoundEnemyClasses;
	// Explicit encounter sizes bypass the ordinary doubled-enemy balance rule.
	UPROPERTY()
	TMap<int32, int32> RoundEnemyCounts;
	UPROPERTY()
	TSet<int32> SingleWaveRounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AdvancedDisplay, Category="Run|Migration", meta=(ToolTip="Enabled after migration. For a newly authored run, enable this before adding displayed rounds; legacy assets must use MigrateLegacyRoundLayout first."))
	bool bExplicitRoundLayout = false;
	// One-time conversion, also available to editor Python. Never saves implicitly.
	UFUNCTION(BlueprintCallable, Category="Run|Migration")
	bool MigrateLegacyRoundLayout();
	// Uses the displayed main-game round number in Rounds; tutorial is excluded.
	// Zero preserves normal entry; tutorial-only mode ignores this override.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Testing", meta = (ClampMin = "0", ToolTip = "0: normal start. Positive values: start at this main-game round (excluding tutorial). Skipped rewards are not granted. Ignored in tutorial-only mode."))
	int32 StartingRoundNumber = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Rewards")
	TArray<TObjectPtr<URLRunRewardDataAsset>> RewardPool;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Progression")
	bool bRepeatFinalRound = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Progression", meta = (ClampMin = "1.0", ClampMax = "2.0"))
	float AttackFrequencyGrowthPerRound = 1.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Progression", meta = (ClampMin = "1.0", ClampMax = "100.0"))
	float MaxAttackFrequencyMultiplier = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run")
	TArray<FRLRoundDefinition> Rounds;
};
