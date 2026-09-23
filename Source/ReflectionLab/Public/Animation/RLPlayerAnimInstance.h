// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "RLPlayerAnimInstance.generated.h"

class ARLPlayerCharacter;
class UCharacterMovementComponent;

UCLASS()
class REFLECTIONLAB_API URLPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float GroundSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Direction = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsFalling = false;

private:
	void CacheOwnerReferences();

	UPROPERTY(Transient)
	TObjectPtr<ARLPlayerCharacter> PlayerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> MovementComponent;
};
