#pragma once

#include "CoreMinimal.h"

// Effective values are rebuilt from DA_PlayerStats, then reward modifiers are applied once.
struct REFLECTIONLAB_API FRLParryStats
{
	float ReflectionCooldown = 0.5f;
	float SuccessfulParryCooldown = 0.03f;
	float PerfectParryOuterBandWidth = 30.0f;
	int32 PerfectSplitProjectileCount = 3;
	float PerfectSplitAngleDegrees = 40.0f;
	int32 BasePierceCount = 0;
	float MaxReflectedSpeedMultiplier = 2.0f;
	float BaseReflectedProjectileScale = 1.35f;
	float CloseRangeThreshold = 55.0f;
	int32 CloseRangePierceCount = 3;
	float CloseRangeProjectileScale = 1.7f;
	float ComboExtraProjectileSpreadAngle = 18.0f;
	int32 OverdriveProjectileCount = 5;
	float OverdriveSpreadAngleDegrees = 100.0f;
	float OverdriveProjectileScale = 2.0f;
	int32 OverdrivePierceCount = 3;
	float ReflectionRange = 140.0f;
	float ReflectionHalfAngleDegrees = 50.0f;
};

// One successful swing, not one projectile. Multi-reflections still advance/heal once.
struct REFLECTIONLAB_API FRLParryResult
{
	int32 MultiParryCount = 1;
	bool bPerfectParry = false;
	bool bCloseRangeParry = false;
	bool bOverdrive = false;
};

// Feedback consumes a value snapshot instead of querying the character's private state.
struct REFLECTIONLAB_API FRLParryViewState
{
	FRLParryStats Stats;
	int32 EnhancementLevel = 1;
	bool bAttemptInProgress = false;
	bool bActive = false;
	bool bOnCooldown = false;
	bool bInHitRecovery = false;
};

