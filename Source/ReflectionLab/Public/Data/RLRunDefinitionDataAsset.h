#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLRunDefinitionDataAsset.generated.h"

class URLDifficultyScheduleDataAsset;
class URLRunRewardDataAsset;
class ARLEnemyCharacter;
class URLArenaExperimentDataAsset;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
	TObjectPtr<URLDifficultyScheduleDataAsset> DifficultySchedule;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Cleanup")
	bool bClearEnemiesOnComplete = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Cleanup")
	bool bClearProjectilesOnComplete = true;
};

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLEnemyHealthMix
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "1.0"))
	float NormalHealth = 2.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "1.0"))
	float ReinforcedHealth = 4.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "1.0"))
	float HeavyHealth = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReinforcedRatio = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HeavyRatio = 0.0f;

	// Evenly spread authored proportions without random all-heavy waves.
	float GetHealthForSpawn(int32 Index, int32 Count) const
	{
		const float Alpha = (FMath::Max(0, Index) + 0.5f) / FMath::Max(1, Count);
		const float Heavy = FMath::IsFinite(HeavyRatio) ? FMath::Clamp(HeavyRatio, 0.0f, 1.0f) : 0.0f;
		const float Reinforced = FMath::IsFinite(ReinforcedRatio) ? FMath::Clamp(ReinforcedRatio, 0.0f, 1.0f - Heavy) : 0.0f;
		const float Health = Alpha < Heavy ? HeavyHealth : (Alpha < Heavy + Reinforced ? ReinforcedHealth : NormalHealth);
		return FMath::IsFinite(Health) ? FMath::Max(1.0f, Health) : 2.0f;
	}
};

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLRunDefinitionDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Experiments")
	TMap<int32, TObjectPtr<URLArenaExperimentDataAsset>> RoundExperiments;
	// Displayed rounds. Custom enemy classes (boss encounters) retain their own health.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Enemies")
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Enemies")
	TMap<int32, TSubclassOf<ARLEnemyCharacter>> RoundEnemyClasses;
	// Explicit encounter sizes bypass the ordinary doubled-enemy balance rule.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Enemies", meta = (ClampMin = "1"))
	TMap<int32, int32> RoundEnemyCounts;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Enemies")
	TSet<int32> SingleWaveRounds;
	// Uses the displayed main-game round number after short-round preparation.
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
