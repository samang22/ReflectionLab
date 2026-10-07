#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLBossSummonDataAsset.generated.h"

class ARLRobotMinionCharacter;
class UMaterialInterface;
class USoundBase;

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLBossSummonDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TSubclassOf<ARLRobotMinionCharacter> MinionClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.1")) float InitialDelay = 5.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.1")) float Interval = 12.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.1")) float EnragedInterval = 8.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.0", ClampMax="1.0")) float EnragedHealthRatio = 0.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="1", ClampMax="10")) int32 MinionsPerSummon = 2;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="1", ClampMax="10")) int32 MaxAliveMinions = 3;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="1.0")) float SpawnRadius = 650.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.0")) float MinPlayerDistance = 250.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation", meta=(ClampMin="0.1")) float TelegraphDuration = 0.6f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation", meta=(ClampMin="0.0")) float SpawnStagger = 0.2f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation", meta=(ClampMin="1.0")) float MarkerRadius = 90.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") TObjectPtr<UMaterialInterface> MarkerMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") TObjectPtr<UMaterialInterface> PulseMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") TObjectPtr<USoundBase> SummonSound;
};
