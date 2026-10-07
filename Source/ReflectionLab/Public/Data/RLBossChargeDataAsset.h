#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLBossChargeDataAsset.generated.h"

class UMaterialInterface;
class URLProjectileDefinitionDataAsset;

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLBossChargeDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.1")) float PreparationDuration = 1.2f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="1.0")) float Distance = 900.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="1.0")) float Speed = 1600.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.1")) float BrakingDuration = 0.35f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.1")) float RecoveryDuration = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.1")) float Cooldown = 8.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.0")) float ContactDamage = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="1", ClampMax="31")) int32 CounterShotCount = 7;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.0", ClampMax="180.0")) float CounterSpreadDegrees = 80.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<URLProjectileDefinitionDataAsset> ProjectileDefinition;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UMaterialInterface> DecalMaterial;
};
