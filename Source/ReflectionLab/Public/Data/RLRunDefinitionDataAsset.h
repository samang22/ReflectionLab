#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLRunDefinitionDataAsset.generated.h"

class URLDifficultyScheduleDataAsset;
class URLRunRewardDataAsset;

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

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLRunDefinitionDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
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
