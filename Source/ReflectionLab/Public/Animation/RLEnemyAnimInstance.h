#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "RLEnemyAnimInstance.generated.h"

class UAnimSequence;
class UBlendSpace;
struct FRLEnemyAnimInstanceProxy;

UCLASS()
class REFLECTIONLAB_API URLEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	URLEnemyAnimInstance();
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	void PlayShootAnimation();
	void ResetCombatAnimation();
	bool IsShooting() const { return bIsShooting; }
	bool IsMoving() const { return GroundSpeed > 3.0f; }

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Animation")
	TObjectPtr<UAnimSequence> IdleAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Animation")
	TObjectPtr<UAnimSequence> ShootAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Animation")
	TObjectPtr<UBlendSpace> MovementBlendSpace;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy Animation")
	float GroundSpeed = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Enemy Animation")
	float Direction = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Animation", meta = (ClampMin = "0.1"))
	float ShootPlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Animation", meta = (ClampMin = "0.0"))
	float BlendTime = 0.12f;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy Animation")
	bool bIsShooting = false;

private:
	friend struct FRLEnemyAnimInstanceProxy;
	float ShootTimeRemaining = 0.0f;
	uint32 ShotSerial = 0;
	uint32 ResetSerial = 0;
};
