#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RLEnemyCombatRow.generated.h"

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLEnemyCombatRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "0"))
	int32 ProjectilePoolPrewarmCount = 16;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "0.1"))
	float AttackInterval = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "1"))
	int32 ShotsPerBurst = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "0.01"))
	float TimeBetweenShots = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "0.0"))
	float InitialFireDelay = 1.0f;
};
