#pragma once

#include "CoreMinimal.h"
#include "RLRunRewardTypes.generated.h"

UENUM(BlueprintType)
enum class ERLRunRewardType : uint8
{
	WiderArc,
	ExtendedRange,
	PiercingReturn,
	PerfectFocus,
	VelocityDrive,
	CloseCall,
	Vitality,
	PerfectRecovery,
};
