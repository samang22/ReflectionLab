#include "Player/Components/RLParryProgressionComponent.h"
#include "Data/RLPlayerStatsDataAsset.h"

URLParryProgressionComponent::URLParryProgressionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URLParryProgressionComponent::Configure(const URLPlayerStatsDataAsset* PlayerStatsData)
{
	if (!PlayerStatsData) { return; }
	EnhancementStage2Combo = FMath::Max(1, PlayerStatsData->EnhancementStage2Combo);
	EnhancementStage3Combo = FMath::Max(
		EnhancementStage2Combo,
		PlayerStatsData->EnhancementStage3Combo);
	EnhancementStage4Combo = FMath::Max(
		EnhancementStage3Combo,
		PlayerStatsData->EnhancementStage4Combo);
}

int32 URLParryProgressionComponent::PreviewNextSuccess() const
{
	return ParryEnhancementLevel < 4 && EnhancementComboProgress + 1 >= GetEnhancementComboRequirement()
		? ParryEnhancementLevel + 1 : ParryEnhancementLevel;
}

void URLParryProgressionComponent::RegisterSuccess(const FRLParryResult& Result)
{
	const int32 MultiParryCount = Result.MultiParryCount;
	const bool bPerfectParry = Result.bPerfectParry;
	const bool bCloseRangeParry = Result.bCloseRangeParry;
	const bool bOverdrive = Result.bOverdrive;
	++ParryChainCount;
	if (!bOverdrive)
	{
		++EnhancementComboProgress;
		if (ParryEnhancementLevel < 4 &&
			EnhancementComboProgress >= GetEnhancementComboRequirement())
		{
			SetParryEnhancementLevel(ParryEnhancementLevel + 1);
			EnhancementComboProgress = 0;
		}
	}
	OnParryChainChanged.Broadcast(ParryChainCount);
	OnParryComboChanged.Broadcast(
		ParryChainCount,
		FMath::Max(1, MultiParryCount),
		ParryEnhancementLevel,
		bPerfectParry,
		bCloseRangeParry);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Parry combo: %d, Multi: %d, Perfect: %s, Close: %s, Overdrive: %s"),
		ParryChainCount,
		MultiParryCount,
		bPerfectParry ? TEXT("true") : TEXT("false"),
		bCloseRangeParry ? TEXT("true") : TEXT("false"),
		bOverdrive ? TEXT("true") : TEXT("false"));
}

void URLParryProgressionComponent::ResetParryChain()
{
	if (ParryChainCount == 0)
	{
		return;
	}

	ParryChainCount = 0;
	OnParryChainChanged.Broadcast(ParryChainCount);
	OnParryComboChanged.Broadcast(0, 0, ParryEnhancementLevel, false, false);
	UE_LOG(LogTemp, Display, TEXT("Parry chain reset."));
}

void URLParryProgressionComponent::ConsumeOverdriveEnhancement()
{
	SetParryEnhancementLevel(3);
	EnhancementComboProgress = 0;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Overdrive enhancement consumed; stage reduced to 3 while combo remains %d."),
		ParryChainCount);
}

void URLParryProgressionComponent::DowngradeParryEnhancement()
{
	SetParryEnhancementLevel(ParryEnhancementLevel - 1);
	EnhancementComboProgress = 0;
}

void URLParryProgressionComponent::SetParryEnhancementLevel(int32 NewLevel)
{
	const int32 ClampedLevel = FMath::Clamp(NewLevel, 1, 4);
	if (ParryEnhancementLevel == ClampedLevel)
	{
		return;
	}

	ParryEnhancementLevel = ClampedLevel;
	OnParryComboChanged.Broadcast(
		ParryChainCount,
		0,
		ParryEnhancementLevel,
		false,
		false);
	UE_LOG(LogTemp, Display, TEXT("Parry enhancement stage: %d"), ParryEnhancementLevel);
}

int32 URLParryProgressionComponent::GetEnhancementComboRequirement() const
{
	switch (ParryEnhancementLevel)
	{
	case 1:
		return FMath::Max(1, EnhancementStage2Combo);
	case 2:
		return FMath::Max(1, EnhancementStage3Combo - EnhancementStage2Combo);
	case 3:
		return FMath::Max(1, EnhancementStage4Combo - EnhancementStage3Combo);
	default:
		return MAX_int32;
	}
}

