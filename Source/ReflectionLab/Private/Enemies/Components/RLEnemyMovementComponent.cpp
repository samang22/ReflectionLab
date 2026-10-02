#include "Enemies/Components/RLEnemyMovementComponent.h"

#include "Enemies/RLEnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

URLEnemyMovementComponent::URLEnemyMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URLEnemyMovementComponent::InitializeMovement()
{
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy) { return; }
	// Enforce facing ownership even when a blueprint saved other defaults.
	Enemy->bUseControllerRotationPitch = false;
	Enemy->bUseControllerRotationYaw = false;
	Enemy->bUseControllerRotationRoll = false;
	if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = false;
	}
	SetComponentTickEnabled(Enemy->IsPoolActive());
	UpdateFacingPlayer();
}

void URLEnemyMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateFacingPlayer();
}

void URLEnemyMovementComponent::UpdateFacingPlayer()
{
	const ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy || !Enemy->IsPoolActive()) { return; }
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!IsValid(PlayerPawn)) { return; }
	const FVector ToPlayer = (PlayerPawn->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
	if (!ToPlayer.IsNearlyZero())
	{
		GetOwner()->SetActorRotation(FRotator(0.0f, ToPlayer.Rotation().Yaw, 0.0f));
	}
}

void URLEnemyMovementComponent::SetTutorialMovementLocked(bool bLocked)
{
	bTutorialMovementLocked = bLocked;
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy || !Enemy->IsPoolActive()) { return; }
	// Lock translation, not facing: tutorial enemies still face the player.
	SetComponentTickEnabled(true);
	if (AController* Controller = Enemy->GetController()) { Controller->StopMovement(); }
	if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		if (bTutorialMovementLocked) { Movement->DisableMovement(); }
		else { Movement->SetDefaultMovementMode(); }
	}
}

void URLEnemyMovementComponent::ActivateForPool()
{
	bTutorialMovementLocked = false;
	InitializeMovement();
	SetTutorialMovementLocked(false);
}

void URLEnemyMovementComponent::DeactivateForPool()
{
	bTutorialMovementLocked = false;
	SetComponentTickEnabled(false);
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy) { return; }
	if (AController* Controller = Enemy->GetController()) { Controller->StopMovement(); }
	if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
}

