#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLParryTuningDataAsset.generated.h"

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLParryTuningDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Timing", meta = (ClampMin = "0.0", DisplayName = "Failed Parry Cooldown"))
	float FailedParryCooldown = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Timing", meta = (ClampMin = "0.0"))
	float SuccessfulParryCooldown = 0.03f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Chain", meta = (ClampMin = "0.0"))
	float ParryChainGracePeriod = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Detection", meta = (ClampMin = "1.0", DisplayName = "Parry Range"))
	float ParryRange = 140.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Detection", meta = (ClampMin = "0.0", ClampMax = "180.0", DisplayName = "Parry Half Angle"))
	float ParryHalfAngleDegrees = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IndicatorIdleOpacity = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IndicatorActiveOpacity = 0.38f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IndicatorSuccessOpacity = 0.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IndicatorUnavailableOpacity = 0.22f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ImpactSoundVolume = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback", meta = (ClampMin = "0.0", ClampMax = "0.2"))
	float HitStopDuration = 0.04f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float HitStopTimeDilation = 0.1f;
};
