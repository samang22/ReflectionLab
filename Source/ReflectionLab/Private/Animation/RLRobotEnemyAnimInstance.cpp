#include "Animation/RLRobotEnemyAnimInstance.h"
#include "Animation/AnimSequence.h"
#include "UObject/ConstructorHelpers.h"

URLRobotEnemyAnimInstance::URLRobotEnemyAnimInstance()
{
	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleFinder(
		TEXT("/Game/ReflectionLab/Art/Characters/Robot/robot/SkeletalMeshes/robotiddle.robotiddle"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> MoveFinder(
		TEXT("/Game/ReflectionLab/Art/Characters/Robot/robot/SkeletalMeshes/robotwalking.robotwalking"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> ShootFinder(
		TEXT("/Game/ReflectionLab/Art/Characters/Robot/robot/SkeletalMeshes/robotattackminiguns.robotattackminiguns"));
	IdleAnimation = IdleFinder.Object;
	MovementAnimation = MoveFinder.Object;
	ShootAnimation = ShootFinder.Object;
	MovementBlendSpace = nullptr;
}
