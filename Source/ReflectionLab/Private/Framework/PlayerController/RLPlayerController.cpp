// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/PlayerController/RLPlayerController.h"
#include "Framework/RLTutorialFlowComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLExpandingRingAttack.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/GameMode/RLGameModeBase.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Player/RLPlayerCharacter.h"
#include "UI/RLParryComboWidget.h"
#include "UI/RLPlayerHealthWidget.h"
#include "UI/RLRunStatusWidget.h"
#include "UI/RLBossHealthWidget.h"
#include "UI/RLOffscreenEnemyWidget.h"
#include "UI/RLTutorialPromptWidget.h"
#include "UI/RLPauseMenuWidget.h"
#include "UObject/ConstructorHelpers.h"

ARLPlayerController::ARLPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	TutorialFlow = CreateDefaultSubobject<URLTutorialFlowComponent>(TEXT("TutorialFlow"));
	OffscreenEnemyWidgetClass = URLOffscreenEnemyWidget::StaticClass();

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

	if (bPauseMenuVisible) { return; }
	UpdateAimRotation();
	TutorialFlow->UpdateTutorial();
}

void ARLPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}
	RestoreGameplayInputMode();

	if (DefaultMappingContext)
	{
		if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	CreateOrBindParryComboWidget();
	CreateOrBindPlayerHealthWidget();
	CreateOrBindRunStatusWidget();
	TutorialFlow->Initialize(bTutorialPromptsEnabled, TutorialNormalProjectileTriggerDistance, TutorialSpecialProjectileTriggerDistance);
	CreateOffscreenEnemyWidget();
}

void ARLPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	RestoreGameplayInputMode();
	CreateOrBindParryComboWidget();
	CreateOrBindPlayerHealthWidget();
	CreateOrBindRunStatusWidget();
	TutorialFlow->Initialize(bTutorialPromptsEnabled, TutorialNormalProjectileTriggerDistance, TutorialSpecialProjectileTriggerDistance);
	CreateOffscreenEnemyWidget();
}

void ARLPlayerController::CreateOffscreenEnemyWidget()
{
	if (IsLocalController() && !OffscreenEnemyWidget)
	{
		OffscreenEnemyWidget = CreateWidget<URLOffscreenEnemyWidget>(
			this, OffscreenEnemyWidgetClass
				? OffscreenEnemyWidgetClass.Get() : URLOffscreenEnemyWidget::StaticClass());
		if (OffscreenEnemyWidget)
		{
			OffscreenEnemyWidget->AddToViewport(5);
		}
	}
}

void ARLPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PauseMenuWidget) { PauseMenuWidget->RemoveFromParent(); }
	bPauseMenuVisible = false;
	Super::EndPlay(EndPlayReason);
}

void ARLPlayerController::TogglePauseMenu()
{
	if (!IsLocalController()) { return; }
	if (bPauseMenuVisible)
	{
		if (PauseMenuWidget && PauseMenuWidget->IsConfirming())
		{
			PauseMenuWidget->CancelConfirmation();
		}
		else { ResumeFromPauseMenu(); }
	}
	else { OpenPauseMenu(); }
}

void ARLPlayerController::OpenPauseMenu()
{
	ARLGameModeBase* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ARLGameModeBase>() : nullptr;
	if (!GameMode || bPauseMenuVisible) { return; }
	// Do not take ownership of pauses from another system. Tutorial prompts are
	// our own modal UI and remain paused when this overlay is dismissed.
	if (UGameplayStatics::IsGamePaused(this) && !TutorialFlow->IsPromptVisible()) { return; }
	if (!PauseMenuWidget)
	{
		PauseMenuWidget = CreateWidget<URLPauseMenuWidget>(this, URLPauseMenuWidget::StaticClass());
	}
	if (!PauseMenuWidget) { return; }
	if (!UGameplayStatics::IsGamePaused(this) && !UGameplayStatics::SetGamePaused(this, true))
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not pause the game."));
		return;
	}
	bPauseMenuVisible = true;
	if (!PauseMenuWidget->IsInViewport()) { PauseMenuWidget->AddToViewport(200); }
	PauseMenuWidget->ShowMenu(GameMode->CanReturnToMainMenu());
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	DefaultMouseCursor = EMouseCursor::Default;
	bShowMouseCursor = true;
}

void ARLPlayerController::ResumeFromPauseMenu()
{
	if (!bPauseMenuVisible) { return; }
	if (!TutorialFlow->IsPromptVisible() && !UGameplayStatics::SetGamePaused(this, false))
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not resume the game."));
		return;
	}
	bPauseMenuVisible = false;
	if (PauseMenuWidget) { PauseMenuWidget->RemoveFromParent(); }
	RestoreModalInputMode();
}

void ARLPlayerController::RestoreModalInputMode()
{
	if (TutorialFlow->IsPromptVisible()) { TutorialFlow->RestorePromptInputMode(); }
	else { RestoreGameplayInputMode(); }
}

bool ARLPlayerController::PreparePauseMenuTravel()
{
	if (!bPauseMenuVisible || !UGameplayStatics::SetGamePaused(this, false)) { return false; }
	bPauseMenuVisible = false;
	if (PauseMenuWidget) { PauseMenuWidget->RemoveFromParent(); }
	// Travel abandons the lesson; do not advance its stage or fire completion.
	TutorialFlow->AbandonPrompt();
	RestoreGameplayInputMode();
	return true;
}

void ARLPlayerController::RestartFromPauseMenu()
{
	ARLGameModeBase* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ARLGameModeBase>() : nullptr;
	if (GameMode && PreparePauseMenuTravel()) { GameMode->RestartRun(); }
}

void ARLPlayerController::ReturnToMainMenuFromPauseMenu()
{
	ARLGameModeBase* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ARLGameModeBase>() : nullptr;
	if (GameMode && GameMode->CanReturnToMainMenu() && PreparePauseMenuTravel())
	{
		GameMode->ReturnToMainMenu();
	}
}

void ARLPlayerController::RestoreGameplayInputMode()
{
	if (!IsLocalController())
	{
		return;
	}

	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	bShowMouseCursor = true;
	UE_LOG(LogTemp, Display, TEXT("Gameplay input mode restored for RLPlayerController."));
}

void ARLPlayerController::CreateOrBindRunStatusWidget()
{
	if (!BossHealthWidget)
	{
		BossHealthWidget = CreateWidget<URLBossHealthWidget>(this, URLBossHealthWidget::StaticClass());
		if (BossHealthWidget) { BossHealthWidget->AddToViewport(11); }
	}
	if (!IsLocalController())
	{
		return;
	}

	if (!RunStatusWidget)
	{
		RunStatusWidget = CreateWidget<URLRunStatusWidget>(
			this,
			URLRunStatusWidget::StaticClass());
		if (RunStatusWidget)
		{
			RunStatusWidget->AddToViewport(10);
		}
	}

	if (RunStatusWidget)
	{
		ARLGameModeBase* GameMode = GetWorld()
			? GetWorld()->GetAuthGameMode<ARLGameModeBase>()
			: nullptr;
		RunStatusWidget->BindToGameMode(GameMode);
	}
}

void ARLPlayerController::OnUnPossess()
{
	if (bPauseMenuVisible) { ResumeFromPauseMenu(); }
	TutorialFlow->UnbindPlayer();
	if (ParryComboWidget)
	{
		ParryComboWidget->BindToPlayer(nullptr);
	}
	if (PlayerHealthWidget)
	{
		PlayerHealthWidget->BindToPlayer(nullptr);
	}
	Super::OnUnPossess();
}

void ARLPlayerController::SetGameplayHUDVisible(bool bVisible)
{
	const ESlateVisibility Visibility = bVisible
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed;
	if (ParryComboWidget)
	{
		const ARLGameModeBase* GameMode = GetWorld()
			? GetWorld()->GetAuthGameMode<ARLGameModeBase>() : nullptr;
		const bool bShowCombo = bVisible && GameMode &&
			GameMode->GetRunState() == ERLRunState::PlayingRound && ParryComboWidget->IsFeedbackActive();
		ParryComboWidget->SetVisibility(bShowCombo
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (PlayerHealthWidget)
	{
		PlayerHealthWidget->SetVisibility(Visibility);
	}
	if (OffscreenEnemyWidget)
	{
		OffscreenEnemyWidget->SetVisibility(Visibility);
	}
}

void ARLPlayerController::CreateOrBindPlayerHealthWidget()
{
	if (!IsLocalController())
	{
		return;
	}

	if (!PlayerHealthWidget)
	{
		PlayerHealthWidget = CreateWidget<URLPlayerHealthWidget>(
			this,
			URLPlayerHealthWidget::StaticClass());
		if (PlayerHealthWidget)
		{
			PlayerHealthWidget->AddToViewport(15);
		}
	}

	if (PlayerHealthWidget)
	{
		PlayerHealthWidget->BindToPlayer(Cast<ARLPlayerCharacter>(GetPawn()));
	}
}

void ARLPlayerController::CreateOrBindParryComboWidget()
{
	if (!IsLocalController())
	{
		return;
	}

	if (!ParryComboWidget)
	{
		ParryComboWidget = CreateWidget<URLParryComboWidget>(
			this,
			URLParryComboWidget::StaticClass());
		if (ParryComboWidget)
		{
			ParryComboWidget->AddToViewport(20);
		}
	}

	if (ParryComboWidget)
	{
		ParryComboWidget->BindToPlayer(Cast<ARLPlayerCharacter>(GetPawn()));
	}
}

void ARLPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent) { return; }
	FInputKeyBinding& PauseBinding = InputComponent->BindKey(
		EKeys::Escape, IE_Pressed, this, &ThisClass::TogglePauseMenu);
	PauseBinding.bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ThisClass::ActivateRoll);
	InputComponent->BindKey(EKeys::P, IE_Pressed, this, &ThisClass::TogglePlayerInvincibility);

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

void ARLPlayerController::TogglePlayerInvincibility()
{
	if (!IsGameplayInputAllowed()) { return; }
	if (ARLPlayerCharacter* PlayerCharacter = Cast<ARLPlayerCharacter>(GetPawn()); PlayerCharacter && !PlayerCharacter->IsDead())
	{
		PlayerCharacter->ToggleInvincibility();
		UE_LOG(LogTemp, Display, TEXT("Player invincibility: %s"),
			PlayerCharacter->IsInvincibilityEnabled() ? TEXT("ON") : TEXT("OFF"));
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
	if (!IsGameplayInputAllowed())
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (const ARLPlayerCharacter* RollingCharacter = Cast<ARLPlayerCharacter>(ControlledPawn); RollingCharacter && RollingCharacter->IsRolling())
	{
		return;
	}
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
	TutorialFlow->ObserveMovement();
}

void ARLPlayerController::ActivateRoll()
{
	if (!IsGameplayInputAllowed())
	{
		return;
	}
	if (ARLPlayerCharacter* RollingCharacter = Cast<ARLPlayerCharacter>(GetPawn()))
	{
		// Read held keys directly: movement input may already have been consumed
		// this frame, and the last input vector can remain after releasing a key.
		const float ForwardInput = (IsInputKeyDown(EKeys::W) ? 1.0f : 0.0f) -
			(IsInputKeyDown(EKeys::S) ? 1.0f : 0.0f);
		const float RightInput = (IsInputKeyDown(EKeys::D) ? 1.0f : 0.0f) -
			(IsInputKeyDown(EKeys::A) ? 1.0f : 0.0f);
		const FRotator ViewRotation = PlayerCameraManager
			? PlayerCameraManager->GetCameraRotation() : GetControlRotation();
		const FRotator YawRotation(0.0, ViewRotation.Yaw, 0.0);
		const FVector RollDirection = (FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X) * ForwardInput +
			FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y) * RightInput).GetSafeNormal2D();
		const bool bWasRolling = RollingCharacter->IsRolling();
		RollingCharacter->StartRoll(RollDirection.IsNearlyZero()
			? RollingCharacter->GetActorForwardVector() : RollDirection);
		TutorialFlow->ObserveRoll(!bWasRolling && RollingCharacter->IsRolling());
	}
}

void ARLPlayerController::ActivateParry()
{
	if (!IsGameplayInputAllowed())
	{
		return;
	}

	if (ARLPlayerCharacter* PlayerCharacter = Cast<ARLPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->StartParry();
	}
}

void ARLPlayerController::UpdateAimRotation()
{
	if (const ARLPlayerCharacter* RollingCharacter = Cast<ARLPlayerCharacter>(GetPawn()); RollingCharacter && RollingCharacter->IsRolling())
	{
		return;
	}
	APawn* ControlledPawn = GetPawn();
	if (!IsLocalController() || !ControlledPawn || !IsGameplayInputAllowed())
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
		TutorialFlow->ObserveAim();
	}
}

bool ARLPlayerController::IsGameplayInputAllowed() const
{
	const ARLGameModeBase* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ARLGameModeBase>()
		: nullptr;
	return !bPauseMenuVisible && !TutorialFlow->IsPromptVisible() && !UGameplayStatics::IsGamePaused(this) &&
		(!GameMode || GameMode->GetRunState() == ERLRunState::PlayingRound);
}

void ARLPlayerController::DismissTutorialPrompt() { TutorialFlow->DismissTutorialPrompt(); }
bool ARLPlayerController::IsTutorialRewardChoiceAllowed() const
{
	return !bPauseMenuVisible && TutorialFlow->IsRewardChoiceAllowed();
}
