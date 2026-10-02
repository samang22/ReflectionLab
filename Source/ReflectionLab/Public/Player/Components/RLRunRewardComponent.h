#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/RLRunRewardTypes.h"
#include "Player/RLPlayerCombatTypes.h"
#include "RLRunRewardComponent.generated.h"

class URLHealthComponent;

DECLARE_MULTICAST_DELEGATE(FRLRunRewardsChangedSignature);

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLRunRewardComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLRunRewardComponent();
	void Initialize(URLHealthComponent* Health);
	void ApplyRunReward(ERLRunRewardType RewardType);
	void ResetRunRewards();
	void ApplyModifiers(FRLParryStats& Stats) const;
	void ApplyPerfectRecovery(const FRLParryResult& Result);
	float GetMaxHealthBonus() const { return RunRewardMaxHealthBonus; }
	bool HasPerfectRecoveryReward() const { return RunRewardPerfectRecoveryAmount > 0; }
	FRLRunRewardsChangedSignature OnRewardsChanged;

private:
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

