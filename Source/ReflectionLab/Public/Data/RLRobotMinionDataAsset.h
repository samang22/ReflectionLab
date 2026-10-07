#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLRobotMinionDataAsset.generated.h"

class URLProjectileDefinitionDataAsset;
class URLBossLaserDataAsset;

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLRobotMinionDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="1.0")) float MaxHealth = 2.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.1")) float AttackInterval = 2.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<URLProjectileDefinitionDataAsset> ProjectileDefinition;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<URLBossLaserDataAsset> LaserSettings;
};
