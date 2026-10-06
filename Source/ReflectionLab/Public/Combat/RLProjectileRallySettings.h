#pragma once

#include "CoreMinimal.h"
#include "RLProjectileRallySettings.generated.h"

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLProjectileRallySettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Rally", meta = (ClampMin = "1.0"))
	float RallyVisualScale = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Rally", meta = (ClampMin = "1.0"))
	float RallyMaxSpeedMultiplier = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Rally", meta = (ClampMin = "1.0"))
	float RallyArrivalRadius = 48.0f;
};
