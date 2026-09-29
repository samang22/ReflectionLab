#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLParryTuningDataAsset.generated.h"

class USoundBase;

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLParryTuningDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Timing", meta = (ClampMin = "0.0", DisplayName = "Failed Parry Cooldown"))
	float FailedParryCooldown = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Timing", meta = (ClampMin = "0.0"))
	float SuccessfulParryCooldown = 0.03f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Perfect", meta = (ClampMin = "0.0", DisplayName = "Perfect Parry Outer Band Width"))
	float PerfectParryOuterBandWidth = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Perfect", meta = (ClampMin = "1", ClampMax = "8"))
	int32 PerfectSplitProjectileCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Perfect", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float PerfectSplitAngleDegrees = 40.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Perfect", meta = (ClampMin = "1.0", ClampMax = "4.0"))
	float PerfectHitStopDurationMultiplier = 1.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Reflection", meta = (ClampMin = "0"))
	int32 BasePierceCount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Reflection", meta = (ClampMin = "1.0"))
	float MaxReflectedSpeedMultiplier = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Reflection", meta = (ClampMin = "1.0", ClampMax = "4.0"))
	float BaseReflectedProjectileScale = 1.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Close Range", meta = (ClampMin = "0.0"))
	float CloseRangeThreshold = 55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Close Range", meta = (ClampMin = "0"))
	int32 CloseRangePierceCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Close Range", meta = (ClampMin = "1.0", ClampMax = "4.0"))
	float CloseRangeProjectileScale = 1.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Combo", meta = (ClampMin = "1"))
	int32 ComboSpeedMilestone = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Combo", meta = (ClampMin = "1"))
	int32 ComboExtraProjectileMilestone = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Enhancement", meta = (ClampMin = "1"))
	int32 EnhancementStage2Combo = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Enhancement", meta = (ClampMin = "1"))
	int32 EnhancementStage3Combo = 6;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Enhancement", meta = (ClampMin = "1"))
	int32 EnhancementStage4Combo = 9;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Combo", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float ComboExtraProjectileSpreadAngle = 18.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Overdrive", meta = (ClampMin = "1"))
	int32 OverdriveComboThreshold = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Overdrive", meta = (ClampMin = "1", ClampMax = "5"))
	int32 OverdriveProjectileCount = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Overdrive", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float OverdriveSpreadAngleDegrees = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Overdrive", meta = (ClampMin = "1.0", ClampMax = "4.0"))
	float OverdriveProjectileScale = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Overdrive", meta = (ClampMin = "0"))
	int32 OverdrivePierceCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Overdrive", meta = (ClampMin = "1.0", ClampMax = "5.0"))
	float OverdriveHitStopDurationMultiplier = 2.5f;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback")
	TObjectPtr<USoundBase> SwingSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SwingSoundVolume = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback|Combo")
	TArray<TObjectPtr<USoundBase>> ComboImpactSounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback|Combo", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	TArray<float> ComboImpactSoundVolumes = {
		0.55f,
		0.65f,
		0.8f,
		0.7f,
		0.9f,
		0.75f,
		0.82f,
		1.0f,
	};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback", meta = (ClampMin = "0.0", ClampMax = "0.2"))
	float HitStopDuration = 0.04f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float HitStopTimeDilation = 0.1f;
};
