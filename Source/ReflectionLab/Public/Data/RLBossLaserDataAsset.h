#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLBossLaserDataAsset.generated.h"
class UMaterialInterface;
class USoundBase;

UCLASS(BlueprintType, EditInlineNew)
class REFLECTIONLAB_API URLBossLaserDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio") TObjectPtr<USoundBase> ChargeSound;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio") TObjectPtr<USoundBase> FireSound;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0", ClampMax="1")) float ChargeVolume = 0.45f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0", ClampMax="1")) float FireVolume = 0.6f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.1")) float PreparationDuration = 2.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.1")) float FiringDuration = 0.2f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.0")) float FadeOutDuration = 0.2f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.1")) float Cooldown = 10.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="1.0")) float Length = 6600.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="1.0")) float Width = 50.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.0")) float Damage = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.1")) float DamageInterval = 0.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UMaterialInterface> BeamMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UMaterialInterface> DecalMaterial;
};
