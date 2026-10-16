#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLArenaExperimentDataAsset.generated.h"
class UMaterialInterface;
class URLProjectileDefinitionDataAsset;
class ARLEnemyCharacter;
class USoundBase;

UENUM(BlueprintType)
enum class ERLArenaExperimentType : uint8 { Crossfire, SupportNetwork, AlternatingZones, AccelerationField, RefractionGate, MirrorArena };

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLArenaExperimentDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment")
	ERLArenaExperimentType Type = ERLArenaExperimentType::Crossfire;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Ballistics")
	TArray<FVector2D> FieldCenters = {FVector2D(-650, 0), FVector2D(650, 0)};
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Ballistics")
	FVector2D FieldHalfExtent = FVector2D(180, 900);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Ballistics", meta=(ClampMin="1", ClampMax="3"))
	float FieldSpeedMultiplier = 1.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Ballistics", meta=(ClampMin="0.1", ClampMax="1"))
	float BaselineHostileSpeedMultiplier = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Ballistics|Audio")
	TObjectPtr<USoundBase> AccelerationSound;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Ballistics|Audio", meta=(ClampMin="0", ClampMax="1"))
	float AccelerationSoundVolume = 0.25f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Ballistics", meta=(ClampMin="-80", ClampMax="80"))
	float RefractionAngleDegrees = 35;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Combat", meta=(ClampMin="0.1", ClampMax="3"))
	float AttackFrequencyMultiplier = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Support")
	TSubclassOf<ARLEnemyCharacter> SupportEnemyClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Support", meta=(ClampMin="100"))
	float SupportRadius = 1000;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Support", meta=(ClampMin="1", ClampMax="3"))
	float SupportAttackRate = 1.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Support")
	bool bSupportGrantsInvulnerability = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Support")
	FLinearColor SupportShieldColor = FLinearColor(0.65f,0.15f,1);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Mirror", meta=(ClampMin="100"))
	float MirrorDistance = 1600;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Mirror", meta=(ClampMin="100"))
	float MirrorHalfWidth = 1800;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Mirror", meta=(ClampMin="100"))
	float LaneHalfWidth = 550;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Mirror", meta=(ClampMin="100"))
	float LaneHalfLength = 300;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Mirror", meta=(ClampMin="10",ClampMax="100"))
	float LaneBarrierVisualHeight = 45;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Mirror", meta=(ClampMin="1",ClampMax="8"))
	int32 MaxMirrorBounces = 4;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Zones", meta=(ClampMin="100"))
	float ArenaHalfExtent = 1850;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Zones")
	FVector2D ZoneHalfExtent = FVector2D(875,1850);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Zones", meta=(ClampMin="0"))
	float ZoneCenterDistance = 975;
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
