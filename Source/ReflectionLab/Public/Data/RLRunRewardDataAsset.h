#pragma once

#include "CoreMinimal.h"
#include "Data/RLRunRewardTypes.h"
#include "Engine/DataAsset.h"
#include "RLRunRewardDataAsset.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLRunRewardDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward")
	bool bEnabled = true;
	// Relative draw weight, not the number of times this reward can be acquired.
	// Zero excludes this reward from the pool.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "0"))
	int32 Count = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward|Effect")
	ERLRunRewardType RewardType = ERLRunRewardType::WiderArc;
	// Degrees, additive range ratio (0.18 = 18%), count, speed ratio, cm, or HP.
	// Count-based effects require a positive whole number.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward|Effect", meta = (ClampMin = "0.0", ClampMax = "10000.0"))
	float Amount = 12.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward|Presentation")
	FText Title;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward|Presentation", meta = (MultiLine = "true"))
	FText Description;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward|Presentation")
	TObjectPtr<UTexture2D> Illustration;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward|Presentation")
	FLinearColor AccentColor = FLinearColor(0.2f, 0.9f, 1.0f);

	bool HasValidEffect() const
	{
		if (static_cast<uint8>(RewardType) > static_cast<uint8>(ERLRunRewardType::PerfectRecovery) ||
			!FMath::IsFinite(Amount) || Amount <= 0.0f || Amount > 10000.0f) { return false; }
		const bool bCountEffect = RewardType == ERLRunRewardType::PiercingReturn ||
			RewardType == ERLRunRewardType::PerfectFocus || RewardType == ERLRunRewardType::PerfectRecovery;
		return !bCountEffect || Amount == FMath::FloorToFloat(Amount);
	}
};
