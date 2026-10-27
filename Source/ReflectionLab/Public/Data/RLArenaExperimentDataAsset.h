#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/RLArenaExperimentSettings.h"
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
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	UFUNCTION(BlueprintCallable, Category="Experiment|Migration")
	void MigrateLegacySettings();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, AdvancedDisplay, Category="Experiment|Migration")
	bool bGroupedSettings = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Settings", meta=(EditCondition="Type == ERLArenaExperimentType::Crossfire", EditConditionHides))
	FRLCrossfireExperimentSettings Crossfire;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Settings", meta=(EditCondition="Type == ERLArenaExperimentType::AccelerationField || Type == ERLArenaExperimentType::RefractionGate", EditConditionHides))
	FRLFieldLayoutSettings FieldLayout;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Settings", meta=(EditCondition="Type == ERLArenaExperimentType::AccelerationField", EditConditionHides))
	FRLAccelerationExperimentSettings Acceleration;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Settings", meta=(EditCondition="Type == ERLArenaExperimentType::RefractionGate", EditConditionHides))
	FRLRefractionExperimentSettings Refraction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Settings", meta=(EditCondition="Type == ERLArenaExperimentType::SupportNetwork", EditConditionHides))
	FRLSupportExperimentSettings Support;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Settings", meta=(EditCondition="Type == ERLArenaExperimentType::AlternatingZones", EditConditionHides))
	FRLZoneExperimentSettings Zones;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Settings", meta=(EditCondition="Type == ERLArenaExperimentType::MirrorArena", EditConditionHides))
	FRLMirrorExperimentSettings Mirror;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Visual")
	TObjectPtr<UMaterialInterface> SurfaceMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment")
	ERLArenaExperimentType Type = ERLArenaExperimentType::Crossfire;
	// Serialization compatibility only. Editor tuning and gameplay use the groups above.
	// These remain readable for migration comparisons, but are not editable in Details.
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	TArray<FVector2D> FieldCenters = {FVector2D(-650, 0), FVector2D(650, 0)};
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	FVector2D FieldHalfExtent = FVector2D(180, 900);
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float FieldSpeedMultiplier = 1.5f;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float BaselineHostileSpeedMultiplier = 1;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	TObjectPtr<USoundBase> AccelerationSound;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float AccelerationSoundVolume = 0.25f;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float RefractionAngleDegrees = 35;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Combat", meta=(ClampMin="0.1", ClampMax="3"))
	float AttackFrequencyMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	TSubclassOf<ARLEnemyCharacter> SupportEnemyClass;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float SupportRadius = 1000;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float SupportAttackRate = 1.5f;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	bool bSupportGrantsInvulnerability = false;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	FLinearColor SupportShieldColor = FLinearColor(0.65f,0.15f,1);
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float MirrorDistance = 1600;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float MirrorHalfWidth = 1800;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float DiagonalMirrorHalfLength = 650;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	FVector2D DiagonalMirrorCenter = FVector2D(950, 800);
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float MirrorCameraArmLength = 4300;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float LaneHalfWidth = 550;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float LaneHalfLength = 300;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float LaneBarrierVisualHeight = 45;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float ShotBarrierVisualHeight = 100;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	int32 MaxMirrorBounces = 4;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float ArenaHalfExtent = 1850;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	FVector2D ZoneHalfExtent = FVector2D(875,1850);
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float ZoneCenterDistance = 975;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float ZoneDamage = 1;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float ZoneDamageInterval = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment")
	FText DisplayName = NSLOCTEXT("ReflectionLab", "CrossfireExperiment", "CROSSFIRE / ALTERNATING SHUTTERS");
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float PhaseDuration = 6.0f;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float WarningDuration = 1.0f;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float ShutterMoveDuration = 0.4f;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float TeamDistance = 1200.0f;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float TeamSpread = 900.0f;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	float ShutterDistance = 500.0f;
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	FVector ShutterSize = FVector(70, 700, 180);
	UPROPERTY(BlueprintReadOnly, Category="Legacy")
	TObjectPtr<UMaterialInterface> ShutterMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Experiment|Combat")
	TObjectPtr<URLProjectileDefinitionDataAsset> ProjectileDefinition;
	bool IsLeftPhase(float Elapsed) const
	{
		const float Duration = Type == ERLArenaExperimentType::AlternatingZones ? Zones.PhaseDuration : Crossfire.PhaseDuration;
		return static_cast<int32>(FMath::Max(0.0f, Elapsed) / FMath::Max(3.0f, Duration)) % 2 == 0;
	}
};
