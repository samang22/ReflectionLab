// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/PlayerController/RLPlayerController.h"

#include "Camera/PlayerCameraManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Player/RLPlayerCharacter.h"
#include "UObject/ConstructorHelpers.h"

ARLPlayerController::ARLPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;

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
	static ConstructorHelpers::FObjectFinder<UInputAction> ReflectActionFinder(
		TEXT("/Game/ReflectionLab/Input/Actions/IA_Reflect.IA_Reflect"));

	DefaultMappingContext = MappingContextFinder.Object;
	MoveForwardAction = MoveForwardActionFinder.Object;
	MoveBackwardAction = MoveBackwardActionFinder.Object;
	MoveLeftAction = MoveLeftActionFinder.Object;
	MoveRightAction = MoveRightActionFinder.Object;
	ReflectAction = ReflectActionFinder.Object;
}

void ARLPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	UpdateAimRotation();
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

	if (ReflectAction)
	{
		EnhancedInputComponent->BindAction(
			ReflectAction,
			ETriggerEvent::Started,
			this,
			&ThisClass::ActivateParry);
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

void ARLPlayerController::ActivateParry()
{
	if (ARLPlayerCharacter* PlayerCharacter = Cast<ARLPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->StartParry();
	}
}

void ARLPlayerController::UpdateAimRotation()
{
	APawn* ControlledPawn = GetPawn();
	if (!IsLocalController() || !ControlledPawn)
	{
		return;
	}

	FVector MouseWorldOrigin;
	FVector MouseWorldDirection;
	if (!DeprojectMousePositionToWorld(MouseWorldOrigin, MouseWorldDirection))
	{
		return;
	}

	if (FMath::IsNearlyZero(FVector::DotProduct(MouseWorldDirection, FVector::UpVector)))
	{
		return;
	}

	const FPlane PlayerPlane(ControlledPawn->GetActorLocation(), FVector::UpVector);
	const FVector AimLocation = FMath::LinePlaneIntersection(
		MouseWorldOrigin,
		MouseWorldOrigin + MouseWorldDirection * 100000.0f,
		PlayerPlane);

	FVector AimDirection = AimLocation - ControlledPawn->GetActorLocation();
	AimDirection.Z = 0.0f;
	if (!AimDirection.IsNearlyZero())
	{
		SetControlRotation(AimDirection.Rotation());
	}
}

