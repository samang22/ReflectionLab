#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLEnemyMovementDataAsset.generated.h"

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLEnemyMovementDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "1"))
	float MovingEnemyRatio = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	float MoveSpeed = 180.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float PostAttackDelay = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	float MinPlayerDistance = 550.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	float MaxPlayerDistance = 1000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	float MaxRepositionDistance = 400.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	float EnemySeparation = 180.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.1"))
	float MoveTimeout = 3.0f;
};
