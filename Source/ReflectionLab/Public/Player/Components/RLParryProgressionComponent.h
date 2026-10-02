#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/RLPlayerCombatTypes.h"
#include "RLParryProgressionComponent.generated.h"

class URLPlayerStatsDataAsset;
DECLARE_MULTICAST_DELEGATE_OneParam(FRLProgressionChainSignature, int32);
DECLARE_MULTICAST_DELEGATE_FiveParams(FRLProgressionComboSignature, int32, int32, int32, bool, bool);

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLParryProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLParryProgressionComponent();
	void Configure(const URLPlayerStatsDataAsset* Stats);
	void RegisterSuccess(const FRLParryResult& Result);
	void ResetParryChain();
	void ConsumeOverdriveEnhancement();
	void DowngradeParryEnhancement();
	int32 PreviewNextSuccess() const;
	int32 GetChainCount() const { return ParryChainCount; }
	int32 GetEnhancementLevel() const { return ParryEnhancementLevel; }
	FRLProgressionChainSignature OnParryChainChanged;
	FRLProgressionComboSignature OnParryComboChanged;

private:
	void SetParryEnhancementLevel(int32 NewLevel);
	int32 GetEnhancementComboRequirement() const;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Parry|Chain", meta = (AllowPrivateAccess = "true"))
	int32 ParryChainCount = 0;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Parry|Enhancement", meta = (AllowPrivateAccess = "true"))
	int32 ParryEnhancementLevel = 1;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Parry|Enhancement", meta = (AllowPrivateAccess = "true"))
	int32 EnhancementComboProgress = 0;
	int32 EnhancementStage2Combo = 3;
	int32 EnhancementStage3Combo = 6;
	int32 EnhancementStage4Combo = 9;
};

