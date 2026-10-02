#include "Player/Components/RLRunRewardComponent.h"
#include "Player/Components/RLHealthComponent.h"

URLRunRewardComponent::URLRunRewardComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URLRunRewardComponent::Initialize(URLHealthComponent* Health)
{
	HealthComponent = Health;
}

void URLRunRewardComponent::ApplyRunReward(ERLRunRewardType RewardType)
{
	if (!HealthComponent) { return; }
	switch (RewardType)
	{
	case ERLRunRewardType::WiderArc:
		RunRewardArcBonusDegrees += 12.0f;
		break;
	case ERLRunRewardType::ExtendedRange:
		RunRewardRangeMultiplier += 0.18f;
		break;
	case ERLRunRewardType::PiercingReturn:
		++RunRewardPierceBonus;
		break;
	case ERLRunRewardType::PerfectFocus:
		++RunRewardPerfectSplitBonus;
		break;
	case ERLRunRewardType::VelocityDrive:
		RunRewardReflectedSpeedBonus += 0.2f;
		break;
	case ERLRunRewardType::CloseCall:
		RunRewardCloseRangeBonus += 12.0f;
		break;
	case ERLRunRewardType::Vitality:
		RunRewardMaxHealthBonus += 2.0f;
		HealthComponent->SetMaxHealth(HealthComponent->GetMaxHealth() + 2.0f, true);
		break;
	case ERLRunRewardType::PerfectRecovery:
		++RunRewardPerfectRecoveryAmount;
		break;
	default:
		return;
	}

	OnRewardsChanged.Broadcast();
}

void URLRunRewardComponent::ResetRunRewards()
{
	if (!HealthComponent) { return; }
	RunRewardRangeMultiplier = 1.0f;
	RunRewardArcBonusDegrees = 0.0f;
	RunRewardPierceBonus = 0;
	RunRewardPerfectSplitBonus = 0;
	RunRewardReflectedSpeedBonus = 0.0f;
	RunRewardCloseRangeBonus = 0.0f;
	HealthComponent->SetMaxHealth(HealthComponent->GetMaxHealth() - RunRewardMaxHealthBonus);
	RunRewardMaxHealthBonus = 0.0f;
	RunRewardPerfectRecoveryAmount = 0;
	OnRewardsChanged.Broadcast();
}

void URLRunRewardComponent::ApplyModifiers(FRLParryStats& Stats) const
{
	Stats.ReflectionRange = FMath::Max(1.0f, Stats.ReflectionRange * RunRewardRangeMultiplier);
	Stats.ReflectionHalfAngleDegrees = FMath::Clamp(
		Stats.ReflectionHalfAngleDegrees + RunRewardArcBonusDegrees,
		0.0f,
		180.0f);
	Stats.BasePierceCount = FMath::Max(0, Stats.BasePierceCount + RunRewardPierceBonus);
	Stats.PerfectSplitProjectileCount = FMath::Clamp(
		Stats.PerfectSplitProjectileCount + RunRewardPerfectSplitBonus,
		1,
		8);
	Stats.MaxReflectedSpeedMultiplier = FMath::Max(
		1.0f,
		Stats.MaxReflectedSpeedMultiplier + RunRewardReflectedSpeedBonus);
	Stats.CloseRangeThreshold = FMath::Max(0.0f, Stats.CloseRangeThreshold + RunRewardCloseRangeBonus);
}

void URLRunRewardComponent::ApplyPerfectRecovery(const FRLParryResult& Result)
{
	if (HealthComponent && Result.bPerfectParry && RunRewardPerfectRecoveryAmount > 0)
	{
		HealthComponent->RestoreHealth(RunRewardPerfectRecoveryAmount);
	}
}

