#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLRunDefinitionDataAsset.generated.h"

class URLDifficultyScheduleDataAsset;

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLRoundDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
	FName RoundName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round", meta = (ClampMin = "1.0"))
	float DurationSeconds = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round", meta = (ClampMin = "0.0"))
	float IntermissionDurationSeconds = 25.0f;

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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run")
	TArray<FRLRoundDefinition> Rounds;
};
