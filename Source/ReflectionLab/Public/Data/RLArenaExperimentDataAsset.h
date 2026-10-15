#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLArenaExperimentDataAsset.generated.h"
class UMaterialInterface;
class URLProjectileDefinitionDataAsset;
class ARLEnemyCharacter;

UENUM(BlueprintType)
enum class ERLArenaExperimentType : uint8 { Crossfire, SupportNetwork, AlternatingZones };

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLArenaExperimentDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment")
	ERLArenaExperimentType Type = ERLArenaExperimentType::Crossfire;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Combat", meta=(ClampMin="0.1", ClampMax="3"))
	float AttackFrequencyMultiplier = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Support")
	TSubclassOf<ARLEnemyCharacter> SupportEnemyClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Support", meta=(ClampMin="100"))
	float SupportRadius = 1000;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Support", meta=(ClampMin="1", ClampMax="3"))
	float SupportAttackRate = 1.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Zones", meta=(ClampMin="100"))
	float ArenaHalfExtent = 1850;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Zones", meta=(ClampMin="0"))
	float ZoneDamage = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Zones", meta=(ClampMin="0.1"))
	float ZoneDamageInterval = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment")
	FText DisplayName = NSLOCTEXT("ReflectionLab", "CrossfireExperiment", "CROSSFIRE / ALTERNATING SHUTTERS");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment", meta=(ClampMin="3.0"))
	float PhaseDuration = 6.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment", meta=(ClampMin="0.5"))
	float WarningDuration = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment", meta=(ClampMin="0.1"))
	float ShutterMoveDuration = 0.4f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Layout", meta=(ClampMin="600.0"))
	float TeamDistance = 1200.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Layout", meta=(ClampMin="100.0"))
	float TeamSpread = 900.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Layout", meta=(ClampMin="100.0"))
	float ShutterDistance = 500.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Layout")
	FVector ShutterSize = FVector(70, 700, 180);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Visual")
	TObjectPtr<UMaterialInterface> ShutterMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Combat")
	TObjectPtr<URLProjectileDefinitionDataAsset> ProjectileDefinition;
	bool IsLeftPhase(float Elapsed) const
	{
		return static_cast<int32>(FMath::Max(0.0f, Elapsed) / FMath::Max(3.0f, PhaseDuration)) % 2 == 0;
	}
};
