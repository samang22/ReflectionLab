#pragma once

#include "CoreMinimal.h"
#include "RLProjectileReflectionTypes.generated.h"

USTRUCT(BlueprintType)
struct FRLProjectileReflectionParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Reflection")
	float SpeedMultiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Reflection", meta = (ClampMin = "0.1"))
	float VisualScaleMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Reflection", meta = (ClampMin = "0"))
	int32 PierceCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Reflection", meta = (ClampMin = "0"))
	int32 ReflectionChain = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Reflection")
	bool bPerfectParry = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Reflection")
	bool bCloseRangeParry = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Reflection")
	bool bOverdrive = false;
};

// Per-activation reflection state; not asset configuration.
struct FRLProjectileReflectionState
{
	int32 RemainingPierces = 0;
	int32 ReflectionChain = 0;
	bool bWasPerfectParried = false;
	bool bWasCloseRangeParried = false;
	bool bWasOverdriveReflected = false;
};
