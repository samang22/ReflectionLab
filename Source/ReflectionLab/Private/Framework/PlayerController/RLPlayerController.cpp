// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/PlayerController/RLPlayerController.h"

#include "Camera/PlayerCameraManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

ARLPlayerController::ARLPlayerController()
{
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MappingContextFinder(
		TEXT("/Game/ReflectionLab/Input/IMC_Player.IMC_Player"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveForwardActionFinder(
		TEXT("/Game/ReflectionLab/Input/Actions/IA_MoveForward.IA_MoveForward"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveBackwardActionFinder(
		TEXT("/Game/ReflectionLab/Input/Actions/IA_MoveBackward.IA_MoveBackward"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveLeftActionFinder(
		TEXT("/Game/ReflectionLab/Input/Actions/IA_MoveLeft.IA_MoveLeft"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveRightActionFinder(
		TEXT("/Game/ReflectionLab/Input/Actions/IA_MoveRight.IA_MoveRight"));

	DefaultMappingContext = MappingContextFinder.Object;
	MoveForwardAction = MoveForwardActionFinder.Object;
	MoveBackwardAction = MoveBackwardActionFinder.Object;
	MoveLeftAction = MoveLeftActionFinder.Object;
	MoveRightAction = MoveRightActionFinder.Object;
}

void ARLPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController() || !DefaultMappingContext)
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ARLPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("RLPlayerController requires an EnhancedInputComponent."));
		return;
	}

	if (MoveForwardAction)
	{
		EnhancedInputComponent->BindAction(
			MoveForwardAction, ETriggerEvent::Triggered, this, &ARLPlayerController::MoveForward);
	}

	if (MoveBackwardAction)
	{
		EnhancedInputComponent->BindAction(
			MoveBackwardAction, ETriggerEvent::Triggered, this, &ARLPlayerController::MoveBackward);
	}

	if (MoveLeftAction)
	{
		EnhancedInputComponent->BindAction(
			MoveLeftAction, ETriggerEvent::Triggered, this, &ARLPlayerController::MoveLeft);
	}

	if (MoveRightAction)
	{
		EnhancedInputComponent->BindAction(
			MoveRightAction, ETriggerEvent::Triggered, this, &ARLPlayerController::MoveRight);
	}
}

void ARLPlayerController::MoveForward()
{
	Move(FVector2D(0.0, 1.0));
}

void ARLPlayerController::MoveBackward()
{
	Move(FVector2D(0.0, -1.0));
}

void ARLPlayerController::MoveLeft()
{
	Move(FVector2D(-1.0, 0.0));
}

void ARLPlayerController::MoveRight()
{
	Move(FVector2D(1.0, 0.0));
}

void ARLPlayerController::Move(const FVector2D& Direction)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const FRotator ViewRotation = PlayerCameraManager
		? PlayerCameraManager->GetCameraRotation()
		: GetControlRotation();
	const FRotator YawRotation(0.0, ViewRotation.Yaw, 0.0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	ControlledPawn->AddMovementInput(ForwardDirection, Direction.Y);
	ControlledPawn->AddMovementInput(RightDirection, Direction.X);
}

