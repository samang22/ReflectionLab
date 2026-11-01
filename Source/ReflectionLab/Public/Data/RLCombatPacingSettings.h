#pragma once
#include "CoreMinimal.h"
#include "RLCombatPacingSettings.generated.h"

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLCombatPacingSettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10")) int32 RemainingEnemyThreshold = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="4")) float ThresholdAttackRate = 1.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="4")) float LastEnemyAttackRate = 2.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.1", ClampMax="3")) float NoThreatMaxWait = 0.7f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.05", ClampMax="0.5")) float UpdateInterval = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.5", ClampMax="10")) float ThreatArrivalHorizon = 4;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="50", ClampMax="600")) float ThreatPassDistance = 250;
	float GetAttackRate(int32 RemainingEnemies) const;
	bool IsApproaching(const FVector& ShotLocation, const FVector& Velocity, const FVector& PlayerLocation) const;
};
