#pragma once

#include "CoreMinimal.h"
#include "RLArenaExperimentSettings.generated.h"

class ARLEnemyCharacter;
class USoundBase;

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLCrossfireMazeWall
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Center = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Size = FVector(90, 650, 180);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Yaw = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) int32 PhaseGroup = 0;
};

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLCrossfireExperimentSettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="3")) float PhaseDuration = 6;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.5")) float WarningDuration = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.1")) float ShutterMoveDuration = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="600")) float TeamDistance = 1200;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100")) float TeamSpread = 900;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100")) float ShutterDistance = 500;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector ShutterSize = FVector(70, 700, 180);
	// Empty preserves the legacy two-gate arrangement. Group 0/1 alternate each phase.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FRLCrossfireMazeWall> MazeWalls;
};

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLFieldLayoutSettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FVector2D> FieldCenters = {FVector2D(-650, 0), FVector2D(650, 0)};
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D FieldHalfExtent = FVector2D(180, 900);
};

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLAccelerationExperimentSettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="3")) float FieldSpeedMultiplier = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.1", ClampMax="1")) float BaselineHostileSpeedMultiplier = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> AccelerationSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0", ClampMax="1")) float AccelerationSoundVolume = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barrier") bool bEnableReturnBarriers = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barrier", meta=(EditCondition="bEnableReturnBarriers"))
	TArray<FVector2D> ReturnBarrierCenters = {FVector2D(-950, 0), FVector2D(950, 0)};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barrier", meta=(EditCondition="bEnableReturnBarriers"))
	FVector ReturnBarrierSize = FVector(60, 4000, 220);
};

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLRefractionExperimentSettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="-80", ClampMax="80")) float RefractionAngleDegrees = 35;
};

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLSupportExperimentSettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<ARLEnemyCharacter> SupportEnemyClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100")) float SupportRadius = 1000;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="3")) float SupportAttackRate = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSupportGrantsInvulnerability = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor SupportShieldColor = FLinearColor(0.65f, 0.15f, 1);
};

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLZoneExperimentSettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="3")) float PhaseDuration = 6;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.5")) float WarningDuration = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100")) float ArenaHalfExtent = 1850;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D ZoneHalfExtent = FVector2D(875, 1850);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float ZoneCenterDistance = 975;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float ZoneDamage = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.1")) float ZoneDamageInterval = 1;
};

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLMirrorExperimentSettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="400", ClampMax="1200")) float DiagonalMirrorHalfLength = 650;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D DiagonalMirrorCenter = FVector2D(950, 800);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera", meta=(ClampMin="2000", ClampMax="6000")) float MirrorCameraArmLength = 4300;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="300", ClampMax="600")) float LaneHalfWidth = 550;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="150", ClampMax="350")) float LaneHalfLength = 300;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="10", ClampMax="100")) float LaneBarrierVisualHeight = 45;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="20", ClampMax="200")) float ShotBarrierVisualHeight = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100", ClampMax="1200")) float ShotBarrierLength = 270;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="8")) int32 MaxMirrorBounces = 4;
};
