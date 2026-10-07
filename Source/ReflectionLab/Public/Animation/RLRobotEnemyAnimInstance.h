#pragma once

#include "Animation/RLEnemyAnimInstance.h"
#include "RLRobotEnemyAnimInstance.generated.h"

// Reuses enemy shot events and pool reset behavior, with robot-authored poses.
UCLASS()
class REFLECTIONLAB_API URLRobotEnemyAnimInstance : public URLEnemyAnimInstance
{
	GENERATED_BODY()
public:
	URLRobotEnemyAnimInstance();
};
