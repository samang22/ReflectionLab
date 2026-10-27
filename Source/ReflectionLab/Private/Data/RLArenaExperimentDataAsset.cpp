#include "Data/RLArenaExperimentDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"

void URLArenaExperimentDataAsset::PostInitProperties()
{
	Super::PostInitProperties();
	// Newly created assets use grouped settings. Old serialized assets default to the legacy version.
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad)) { bGroupedSettings = true; }
}

void URLArenaExperimentDataAsset::PostLoad()
{
	Super::PostLoad();
	MigrateLegacySettings();
}

void URLArenaExperimentDataAsset::MigrateLegacySettings()
{
	if (bGroupedSettings) { return; }
	Crossfire.PhaseDuration = PhaseDuration;
	Crossfire.WarningDuration = WarningDuration;
	Crossfire.ShutterMoveDuration = ShutterMoveDuration;
	Crossfire.TeamDistance = TeamDistance;
	Crossfire.TeamSpread = TeamSpread;
	Crossfire.ShutterDistance = ShutterDistance;
	Crossfire.ShutterSize = ShutterSize;
	FieldLayout.FieldCenters = FieldCenters;
	FieldLayout.FieldHalfExtent = FieldHalfExtent;
	Acceleration.FieldSpeedMultiplier = FieldSpeedMultiplier;
	Acceleration.BaselineHostileSpeedMultiplier = BaselineHostileSpeedMultiplier;
	Acceleration.AccelerationSound = AccelerationSound;
	Acceleration.AccelerationSoundVolume = AccelerationSoundVolume;
	Refraction.RefractionAngleDegrees = RefractionAngleDegrees;
	Support.SupportEnemyClass = SupportEnemyClass;
	Support.SupportRadius = SupportRadius;
	Support.SupportAttackRate = SupportAttackRate;
	Support.bSupportGrantsInvulnerability = bSupportGrantsInvulnerability;
	Support.SupportShieldColor = SupportShieldColor;
	Zones.PhaseDuration = PhaseDuration;
	Zones.WarningDuration = WarningDuration;
	Zones.ArenaHalfExtent = ArenaHalfExtent;
	Zones.ZoneHalfExtent = ZoneHalfExtent;
	Zones.ZoneCenterDistance = ZoneCenterDistance;
	Zones.ZoneDamage = ZoneDamage;
	Zones.ZoneDamageInterval = ZoneDamageInterval;
	Mirror.DiagonalMirrorHalfLength = DiagonalMirrorHalfLength;
	Mirror.DiagonalMirrorCenter = DiagonalMirrorCenter;
	Mirror.MirrorCameraArmLength = MirrorCameraArmLength;
	Mirror.LaneHalfWidth = LaneHalfWidth;
	Mirror.LaneHalfLength = LaneHalfLength;
	Mirror.LaneBarrierVisualHeight = LaneBarrierVisualHeight;
	Mirror.ShotBarrierVisualHeight = ShotBarrierVisualHeight;
	Mirror.MaxMirrorBounces = MaxMirrorBounces;
	SurfaceMaterial = ShutterMaterial;
	bGroupedSettings = true;
}
