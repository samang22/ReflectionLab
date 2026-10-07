#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLBossOverloadDataAsset.generated.h"

class UMaterialInterface;
class URLProjectileDefinitionDataAsset;

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLBossOverloadDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1.0"))
	float MaxHealth = 20.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.1"))
	float DamageThreshold = 5.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.1"))
	// Absorption phase; retained property name preserves existing asset compatibility.
	float Duration = 5.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.1"))
	float VulnerableDuration = 5.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1.0"))
	float Radius = 350.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float HealPerProjectile = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spin", meta = (ClampMin = "1.0", Units = "deg/s"))
	float SpinSpeed = 720.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spin", meta = (ClampMin = "0.01", Units = "s"))
	float SpinShotInterval = 0.06f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deprecated", meta = (DeprecatedProperty, DeprecationMessage = "Use SpinShotInterval instead."))
	float ShotInterval = 0.8f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deprecated", meta = (DeprecatedProperty, DeprecationMessage = "Spin pattern emits one shot from muzzle at a time."))
	int32 ProjectilesPerVolley = 12;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float PulseAmplitude = 0.12f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.1"))
	float PulseFrequency = 2.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<URLProjectileDefinitionDataAsset> ProjectileDefinition;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> DecalMaterial;
};
