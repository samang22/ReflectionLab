#include "Animation/RLPlayerAnimInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"
#include "Player/RLPlayerCharacter.h"

void URLPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CacheOwnerReferences();
}

void URLPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!IsValid(PlayerCharacter) || !IsValid(MovementComponent))
	{
		CacheOwnerReferences();
	}

	if (!IsValid(PlayerCharacter) || !IsValid(MovementComponent))
	{
		GroundSpeed = 0.0f;
		Direction = 0.0f;
		bShouldMove = false;
		bIsFalling = false;
		return;
	}

	const FVector Velocity = PlayerCharacter->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	Direction = UKismetAnimationLibrary::CalculateDirection(
		Velocity,
		PlayerCharacter->GetActorRotation());
	bShouldMove = GroundSpeed > 3.0f;
	bIsFalling = MovementComponent->IsFalling();
}

void URLPlayerAnimInstance::CacheOwnerReferences()
{
	PlayerCharacter = Cast<ARLPlayerCharacter>(TryGetPawnOwner());
	MovementComponent = PlayerCharacter
		? PlayerCharacter->GetCharacterMovement()
		: nullptr;
}
