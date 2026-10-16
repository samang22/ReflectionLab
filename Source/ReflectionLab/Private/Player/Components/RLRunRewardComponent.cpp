#include "Player/Components/RLRunRewardComponent.h"
#include "Player/Components/RLHealthComponent.h"
#include "Data/RLRunRewardDataAsset.h"
#include "GameFramework/Actor.h"

URLRunRewardComponent::URLRunRewardComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void URLRunRewardComponent::InitializeComponent()
{
	Super::InitializeComponent();
	if (AActor* Owner = GetOwner()) { Initialize(Owner->FindComponentByClass<URLHealthComponent>()); }
}

void URLRunRewardComponent::Initialize(URLHealthComponent* Health)
{
	if (!IsValid(Health) || HealthComponent == Health) { return; }
	if (IsValid(HealthComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("Run reward health dependency is already initialized."));
		return;
	}
	HealthComponent = Health;
}

void URLRunRewardComponent::ApplyRunReward(ERLRunRewardType RewardType)
{
	// Compatibility entry point; new gameplay uses data assets via TryApplyReward.
	float Amount = 1.0f;
	switch (RewardType)
	{
	case ERLRunRewardType::WiderArc:
	case ERLRunRewardType::CloseCall: Amount = 12.0f; break;
	case ERLRunRewardType::ExtendedRange: Amount = 0.18f; break;
	case ERLRunRewardType::VelocityDrive: Amount = 0.2f; break;
	case ERLRunRewardType::Vitality: Amount = 2.0f; break;
	default: break;
	}
	if (ApplyRewardEffect(RewardType, Amount))
	{
		RecordReward(FName(*FString::Printf(TEXT("Legacy_%d"), static_cast<int32>(RewardType))),
			StaticEnum<ERLRunRewardType>()->GetDisplayNameTextByValue(static_cast<int64>(RewardType)), FText::GetEmpty());
		OnRewardsChanged.Broadcast();
	}
}

bool URLRunRewardComponent::TryApplyReward(const URLRunRewardDataAsset* Definition)
{
	if (!IsValid(Definition) || !Definition->bEnabled || !Definition->HasValidEffect()) { return false; }
	if (!ApplyRewardEffect(Definition->RewardType, Definition->Amount)) { return false; }
	RecordReward(FName(*Definition->GetPathName()), Definition->Title, Definition->Description);
	OnRewardsChanged.Broadcast();
	return true;
}

bool URLRunRewardComponent::ApplyRewardEffect(ERLRunRewardType RewardType, float Amount)
{
	if (!IsValid(HealthComponent) || !FMath::IsFinite(Amount) || Amount <= 0.0f) { return false; }
	switch (RewardType)
	{
	case ERLRunRewardType::WiderArc:
		RunRewardArcBonusDegrees += Amount;
		break;
	case ERLRunRewardType::ExtendedRange:
		RunRewardRangeMultiplier += Amount;
		break;
	case ERLRunRewardType::PiercingReturn:
		RunRewardPierceBonus = static_cast<int32>(FMath::Min<int64>(MAX_int32,
			static_cast<int64>(RunRewardPierceBonus) + FMath::RoundToInt(Amount)));
		break;
	case ERLRunRewardType::PerfectFocus:
		RunRewardPerfectSplitBonus = FMath::Min(8, RunRewardPerfectSplitBonus + FMath::RoundToInt(Amount));
		break;
	case ERLRunRewardType::VelocityDrive:
		RunRewardReflectedSpeedBonus += Amount;
		break;
	case ERLRunRewardType::CloseCall:
		RunRewardCloseRangeBonus += Amount;
		break;
	case ERLRunRewardType::Vitality:
		if (!FMath::IsFinite(HealthComponent->GetMaxHealth() + Amount) ||
			!FMath::IsFinite(RunRewardMaxHealthBonus + Amount)) { return false; }
		RunRewardMaxHealthBonus += Amount;
		HealthComponent->SetMaxHealth(HealthComponent->GetMaxHealth() + Amount, true);
		break;
	case ERLRunRewardType::PerfectRecovery:
		RunRewardPerfectRecoveryAmount = static_cast<int32>(FMath::Min<int64>(MAX_int32,
			static_cast<int64>(RunRewardPerfectRecoveryAmount) + FMath::RoundToInt(Amount)));
		break;
	default:
		return false;
	}
	return true;
}

void URLRunRewardComponent::RecordReward(FName Id, const FText& Title, const FText& Description)
{
	FRLAcquiredReward* Entry = AcquiredRewards.FindByPredicate([Id](const auto& Item) { return Item.Id == Id; });
	if (!Entry) { Entry = &AcquiredRewards.AddDefaulted_GetRef(); Entry->Id = Id; Entry->Title = Title; Entry->Description = Description; }
	Entry->Count = FMath::Min(Entry->Count, MAX_int32 - 1) + 1;
}

void URLRunRewardComponent::ResetRunRewards()
{
	AcquiredRewards.Reset();
	if (!IsValid(HealthComponent)) { return; }
	const float PreviousMaxHealthBonus = RunRewardMaxHealthBonus;
	RunRewardRangeMultiplier = 1.0f;
	RunRewardArcBonusDegrees = 0.0f;
	RunRewardPierceBonus = 0;
	RunRewardPerfectSplitBonus = 0;
	RunRewardReflectedSpeedBonus = 0.0f;
	RunRewardCloseRangeBonus = 0.0f;
	RunRewardMaxHealthBonus = 0.0f;
	RunRewardPerfectRecoveryAmount = 0;
	// Clear all modifiers before health-change listeners observe the reset.
	HealthComponent->SetMaxHealth(HealthComponent->GetMaxHealth() - PreviousMaxHealthBonus);
	OnRewardsChanged.Broadcast();
}

void URLRunRewardComponent::ApplyModifiers(FRLParryStats& Stats) const
{
	Stats.ReflectionRange = FMath::Max(1.0f, Stats.ReflectionRange * RunRewardRangeMultiplier);
	Stats.ReflectionHalfAngleDegrees = FMath::Clamp(
		Stats.ReflectionHalfAngleDegrees + RunRewardArcBonusDegrees,
		0.0f,
		180.0f);
	Stats.BasePierceCount = static_cast<int32>(FMath::Clamp<int64>(
		static_cast<int64>(Stats.BasePierceCount) + RunRewardPierceBonus, 0, MAX_int32));
	Stats.PerfectSplitProjectileCount = FMath::Clamp(
		Stats.PerfectSplitProjectileCount + RunRewardPerfectSplitBonus,
		1,
		8);
	Stats.PerfectSplitRewardBonus = RunRewardPerfectSplitBonus;
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

