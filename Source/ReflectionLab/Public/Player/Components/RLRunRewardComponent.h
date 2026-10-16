#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/RLRunRewardTypes.h"
#include "Player/RLPlayerCombatTypes.h"
#include "RLRunRewardComponent.generated.h"

class URLHealthComponent;
class URLRunRewardDataAsset;

USTRUCT()
struct FRLAcquiredReward
{
	GENERATED_BODY()
	UPROPERTY() FName Id;
	UPROPERTY() FText Title;
	UPROPERTY() FText Description;
	UPROPERTY() int32 Count = 0;
};

DECLARE_MULTICAST_DELEGATE(FRLRunRewardsChangedSignature);

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLRunRewardComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLRunRewardComponent();
	virtual void InitializeComponent() override;
	// Explicit dependency injection for standalone components/previews; runtime
	// initialization resolves the owner's health component automatically.
	void Initialize(URLHealthComponent* Health);
	// Compatibility API for existing Blueprint-facing character wrappers.
	void ApplyRunReward(ERLRunRewardType RewardType);
	UFUNCTION(BlueprintCallable, Category = "Run|Rewards")
	bool TryApplyReward(const URLRunRewardDataAsset* Definition);
	void ResetRunRewards();
	void ApplyModifiers(FRLParryStats& Stats) const;
	void ApplyPerfectRecovery(const FRLParryResult& Result);
	float GetMaxHealthBonus() const { return RunRewardMaxHealthBonus; }
	float GetReflectedSpeedBonus() const { return RunRewardReflectedSpeedBonus; }
	bool HasPerfectRecoveryReward() const { return RunRewardPerfectRecoveryAmount > 0; }
	const TArray<FRLAcquiredReward>& GetAcquiredRewards() const { return AcquiredRewards; }
	FRLRunRewardsChangedSignature OnRewardsChanged;

private:
	void RecordReward(FName Id, const FText& Title, const FText& Description);
	UPROPERTY(Transient) TArray<FRLAcquiredReward> AcquiredRewards;
	bool ApplyRewardEffect(ERLRunRewardType RewardType, float Amount);
	UPROPERTY(Transient)
	TObjectPtr<URLHealthComponent> HealthComponent;
	float RunRewardRangeMultiplier = 1.0f;
	float RunRewardArcBonusDegrees = 0.0f;
	int32 RunRewardPierceBonus = 0;
	int32 RunRewardPerfectSplitBonus = 0;
	float RunRewardReflectedSpeedBonus = 0.0f;
	float RunRewardCloseRangeBonus = 0.0f;
	float RunRewardMaxHealthBonus = 0.0f;
	int32 RunRewardPerfectRecoveryAmount = 0;
};

