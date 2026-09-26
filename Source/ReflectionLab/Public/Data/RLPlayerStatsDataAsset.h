#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLPlayerStatsDataAsset.generated.h"

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLPlayerStatsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Movement", meta = (ClampMin = "0.0"))
	float MaxWalkSpeed = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Hit Recovery", meta = (ClampMin = "0.0"))
	float HitRecoveryDuration = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Hit Recovery", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitRecoveryMovementSpeedMultiplier = 0.45f;
};
