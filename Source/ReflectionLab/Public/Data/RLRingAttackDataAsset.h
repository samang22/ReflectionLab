#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLRingAttackDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FRLRingAttackSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring", meta = (ClampMin = "0.0"))
	float StartRadius = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring", meta = (ClampMin = "1.0"))
	float ExpansionSpeed = 450.0f;
	// Full radial width, not half width.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring", meta = (ClampMin = "1.0"))
	float RingThickness = 40.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring", meta = (ClampMin = "1.0"))
	float AttackHeight = 70.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring", meta = (ClampMin = "0.0"))
	float Damage = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring|Lifetime", meta = (ClampMin = "0.1"))
	float ActiveDuration = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring|Lifetime", meta = (ClampMin = "0.01"))
	float FadeOutDuration = 0.5f;
};

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLRingAttackDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ring")
	FRLRingAttackSettings Settings;
};

USTRUCT(BlueprintType)
struct FRLRingAttackSpawnRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring Attack", meta = (ClampMin = "0"))
	int32 Weight = 1;
	// Per-enemy minimum interval, unaffected by round attack-speed scaling.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring Attack", meta = (ClampMin = "0.0"))
	float MinimumIntervalSeconds = 20.0f;

	// Legacy interval selection only. Zero disables the legacy attack.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring Attack", meta = (ClampMin = "0", DisplayName = "Legacy Shot Interval"))
	int32 ShotInterval = 12;
	// Null uses the ring actor's built-in settings.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring Attack")
	TObjectPtr<URLRingAttackDataAsset> AttackData;

	bool MatchesShot(int32 ShotNumber) const
	{
		return ShotInterval > 0 && ShotNumber > 0 && ShotNumber % ShotInterval == 0;
	}
};
