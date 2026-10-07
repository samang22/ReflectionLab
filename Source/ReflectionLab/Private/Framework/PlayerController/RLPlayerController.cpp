// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/PlayerController/RLPlayerController.h"

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

#define LOCTEXT_NAMESPACE "ReflectionLabTutorial"

ARLPlayerController::ARLPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
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
	UpdateTutorial();
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
	CreateOrBindTutorialPromptWidget();
	CreateOffscreenEnemyWidget();
}

void ARLPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	RestoreGameplayInputMode();
	CreateOrBindParryComboWidget();
	CreateOrBindPlayerHealthWidget();
	CreateOrBindRunStatusWidget();
	CreateOrBindTutorialPromptWidget();
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
	if (UGameplayStatics::IsGamePaused(this) && !bTutorialPromptVisible) { return; }
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
	if (!bTutorialPromptVisible && !UGameplayStatics::SetGamePaused(this, false))
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
	if (bTutorialPromptVisible && TutorialPromptWidget)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(TutorialPromptWidget->TakeWidget());
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		DefaultMouseCursor = EMouseCursor::Default;
		bShowMouseCursor = true;
	}
	else { RestoreGameplayInputMode(); }
}

bool ARLPlayerController::PreparePauseMenuTravel()
{
	if (!bPauseMenuVisible || !UGameplayStatics::SetGamePaused(this, false)) { return false; }
	bPauseMenuVisible = false;
	if (PauseMenuWidget) { PauseMenuWidget->RemoveFromParent(); }
	// Travel abandons the lesson; do not advance its stage or fire completion.
	bTutorialPromptVisible = false;
	if (TutorialPromptWidget) { TutorialPromptWidget->SetVisibility(ESlateVisibility::Collapsed); }
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

void ARLPlayerController::CreateOrBindTutorialPromptWidget()
{
	if (!IsLocalController())
	{
		return;
	}

	if (!TutorialPromptWidget)
	{
		TutorialPromptWidget = CreateWidget<URLTutorialPromptWidget>(
			this, URLTutorialPromptWidget::StaticClass());
		if (TutorialPromptWidget)
		{
			TutorialPromptWidget->AddToViewport(100);
			TutorialPromptWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	BindTutorialPlayer(Cast<ARLPlayerCharacter>(GetPawn()));
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
	if (TutorialRing.IsValid()) { TutorialRing->BeginFadeOut(); }
	TutorialRing.Reset();
	if (bTutorialPromptVisible)
	{
		DismissTutorialPrompt();
	}
	BindTutorialPlayer(nullptr);
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
	if (TutorialStage == ERLTutorialStage::WaitingForMovement)
	{
		bTutorialMovementObserved = true;
	}
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
		if (TutorialStage == ERLTutorialStage::WaitingForRoll && !bWasRolling && RollingCharacter->IsRolling())
		{
			bTutorialRollStarted = true;
		}
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
		if (TutorialStage == ERLTutorialStage::WaitingForMovement)
		{
			float MouseX = 0.0f;
			float MouseY = 0.0f;
			if (GetMousePosition(MouseX, MouseY))
			{
				const FVector2D MousePosition(MouseX, MouseY);
				bTutorialAimObserved |= bTutorialMousePositionValid && !MousePosition.Equals(TutorialMousePosition, 0.1f);
				TutorialMousePosition = MousePosition;
				bTutorialMousePositionValid = true;
			}
		}
	}
}

void ARLPlayerController::BindTutorialPlayer(ARLPlayerCharacter* PlayerCharacter)
{
	if (TutorialBoundPlayer)
	{
		TutorialBoundPlayer->OnParryComboChanged.RemoveDynamic(
			this, &ThisClass::HandleTutorialParryComboChanged);
	}

	TutorialBoundPlayer = PlayerCharacter;
	if (TutorialBoundPlayer)
	{
		TutorialBoundPlayer->OnParryComboChanged.AddUniqueDynamic(
			this, &ThisClass::HandleTutorialParryComboChanged);
	}
}

void ARLPlayerController::QueueTutorialStage(ERLTutorialStage NextStage)
{
	if (!GetWorld() || TutorialStage == ERLTutorialStage::WaitingForStageDelay) { return; }
	PendingTutorialStage = NextStage;
	TutorialStage = ERLTutorialStage::WaitingForStageDelay;
	TutorialPromptReadyTimeSeconds = GetWorld()->GetTimeSeconds() + TutorialStageDelaySeconds;
}

void ARLPlayerController::UpdateTutorial()
{
	if (!bTutorialPromptsEnabled || bTutorialPromptVisible || bPauseMenuVisible || !GetWorld())
	{
		return;
	}

	ARLGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ARLGameModeBase>();
	if (!GameMode || !GameMode->IsCurrentRoundTutorial())
	{
		return;
	}
	if (TutorialStage == ERLTutorialStage::WaitingForStageDelay)
	{
		if (GetWorld()->GetTimeSeconds() < TutorialPromptReadyTimeSeconds) { return; }
		TutorialStage = PendingTutorialStage;
		NextTutorialProjectileRequestTimeSeconds = 0.0f;
	}
	if (GameMode->IsChoosingReward() && TutorialStage != ERLTutorialStage::WaitingForRewardChoice)
	{
		if (TutorialStage != ERLTutorialStage::WaitingForRewardExplanation)
		{
			QueueTutorialStage(ERLTutorialStage::WaitingForRewardExplanation);
			return;
		}
		ShowTutorialPrompt(
			LOCTEXT("RoundRewardTitle", "ROUND CLEAR — CHOOSE A REWARD"),
			LOCTEXT("RoundRewardBody",
				"After clearing a round, choose one of three reward cards.\nThe selected upgrade lasts for the rest of that run.\nClose this message and select a card to finish the tutorial.\nTutorial upgrades reset when you start the main game."),
			ERLTutorialStage::WaitingForRewardChoice);
		return;
	}
	if (GameMode->GetRunState() != ERLRunState::PlayingRound)
	{
		return;
	}
	if (TutorialStage == ERLTutorialStage::WaitingForMovement &&
		bTutorialMovementObserved && bTutorialAimObserved)
	{
		QueueTutorialStage(ERLTutorialStage::WaitingForRollExplanation);
		return;
	}
	if (TutorialStage == ERLTutorialStage::WaitingForRollExplanation)
	{
		ShowTutorialPrompt(LOCTEXT("RollTitle", "DODGE ROLL"),
			LOCTEXT("RollBody", "Hold WASD and press SPACE to roll in that movement direction.\nWithout movement input, you roll forward in the direction you face.\nYou are invulnerable while rolling.\nClose this message and complete one roll to continue."),
			ERLTutorialStage::WaitingForRoll);
		return;
	}
	if (TutorialStage == ERLTutorialStage::WaitingForRoll && bTutorialRollStarted &&
		TutorialBoundPlayer && !TutorialBoundPlayer->IsRolling())
	{
		QueueTutorialStage(ERLTutorialStage::WaitingForNormalProjectile);
		return;
	}
	if (TutorialStage == ERLTutorialStage::WaitingForPerfectExplanation)
	{
		ShowPerfectParryTutorial();
		return;
	}
	if (TutorialStage == ERLTutorialStage::WaitingForCloseRangeExplanation)
	{
		ShowTutorialPrompt(
			LOCTEXT("CloseParryTitle", "CLOSE-RANGE PARRY"),
			LOCTEXT("CloseParryBody",
				"Let a projectile get close to your character before parrying.\nA close-range parry returns a faster, larger shot with extra piercing.\nDistance is measured from you to the projectile, not to the enemy.\nGuard shots still cannot damage enemies or pierce."),
			ERLTutorialStage::WaitingForCloseRangeParry);
		return;
	}

	if (TutorialStage == ERLTutorialStage::WaitingForTutorial)
	{
		ShowTutorialPrompt(
			LOCTEXT("MovementTitle", "MOVE & AIM"),
			LOCTEXT(
				"MovementBody",
				"Use WASD to move. Your character faces the mouse cursor.\nMove and move the mouse after closing this message to continue."),
			ERLTutorialStage::WaitingForMovement);
		return;
	}

	if (TutorialStage == ERLTutorialStage::WaitingForSuccessfulParry &&
		TutorialBoundPlayer && TutorialBoundPlayer->GetParryComboCount() > 0)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("Tutorial advanced after polling a successful parry."));
		QueueTutorialStage(ERLTutorialStage::WaitingForPerfectExplanation);
		return;
	}

	if (TutorialStage == ERLTutorialStage::WaitingForExplosiveResolution)
	{
		bTutorialExplosiveParried |= HasActiveTutorialProjectile(ERLProjectileBehavior::Explosive, true);
		if (bTutorialExplosiveParried &&
			!HasActiveTutorialProjectile(ERLProjectileBehavior::Explosive, true))
		{
			QueueTutorialStage(ERLTutorialStage::WaitingForRallyProjectile);
			UE_LOG(
				LogTemp,
				Display,
				TEXT("Tutorial reflected explosive resolved; rally projectile stage queued."));
			return;
		}
		else
		{
			if (!bTutorialExplosiveParried) { UpdateTutorialProjectileRequest(*GameMode); }
			return;
		}
	}

	if (TutorialStage == ERLTutorialStage::WaitingForComboExplanation)
	{
		ShowComboTutorial();
		return;
	}
	if (TutorialStage == ERLTutorialStage::WaitingForRingExplanation)
	{
		ShowTutorialPrompt(LOCTEXT("RingTitle", "UNPARRYABLE RING"),
			LOCTEXT("RingBody", "The expanding red ring cannot be parried.\nOnly the ring's band deals damage; the inside is safe.\nHold WASD toward the other side and press SPACE to roll through it.\nSuccessfully avoid the ring while rolling to continue."),
			ERLTutorialStage::WaitingForRingDodge);
		return;
	}
	if (TutorialStage == ERLTutorialStage::WaitingForRingDodge)
	{
		UpdateTutorialRingRequest(*GameMode);
		return;
	}
	if (TutorialStage == ERLTutorialStage::WaitingForFinalCombatExplanation)
	{
		ShowTutorialPrompt(LOCTEXT("FinalCombatTitle", "FINISH THE ROUND"),
			LOCTEXT("FinalCombatBody", "You have learned the attacks and how to dodge the ring.\nDefeat the enemy with reflected shots to clear the round\nand choose a reward card."),
			ERLTutorialStage::Complete);
		return;
	}
	UpdateTutorialProjectileRequest(*GameMode);
	if (TutorialStage == ERLTutorialStage::WaitingForCombo && TutorialBoundPlayer &&
		TutorialBoundPlayer->GetParryComboCount() >= TutorialComboBaseline + 2)
	{
		QueueTutorialStage(ERLTutorialStage::WaitingForComboExplanation);
		return;
	}

	if (TutorialStage != ERLTutorialStage::WaitingForNormalProjectile &&
		TutorialStage != ERLTutorialStage::WaitingForExplosiveProjectile &&
		TutorialStage != ERLTutorialStage::WaitingForRallyProjectile &&
		TutorialStage != ERLTutorialStage::WaitingForGuardProjectile)
	{
		return;
	}

	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const float NormalDistanceSquared = FMath::Square(TutorialNormalProjectileTriggerDistance);
	const float SpecialDistanceSquared = FMath::Square(
		FMath::Min(TutorialSpecialProjectileTriggerDistance, 650.0f));
	for (TActorIterator<ARLProjectile> Iterator(GetWorld()); Iterator; ++Iterator)
	{
		const ARLProjectile* Projectile = *Iterator;
		if (!Projectile->IsPoolActive() || Projectile->IsReflected())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(
			Projectile->GetActorLocation(), ControlledPawn->GetActorLocation());
		if (TutorialStage == ERLTutorialStage::WaitingForNormalProjectile &&
			Projectile->CanBeReflected() && !Projectile->IsExplosive() &&
			!Projectile->IsGuardProjectile() && DistanceSquared <= NormalDistanceSquared)
		{
			GameMode->SetTutorialEnemyMovementLocked(false);
			ShowTutorialPrompt(
				LOCTEXT("ParryTitle", "PARRY"),
				LOCTEXT(
					"ParryBody",
					"Press Left Mouse Button to reflect projectiles in front of you.\nThe inner zone is a normal parry; the outer ring is a perfect parry."),
				ERLTutorialStage::WaitingForSuccessfulParry);
			return;
		}

		if (TutorialStage == ERLTutorialStage::WaitingForExplosiveProjectile &&
			Projectile->IsExplosive() && DistanceSquared <= SpecialDistanceSquared)
		{
			ShowTutorialPrompt(
				LOCTEXT("ExplosiveTitle", "EXPLOSIVE — PARRY"),
				LOCTEXT(
					"ExplosiveBody",
					"Parry the blinking red shot to turn it into your bomb.\nIt keeps its speed; only speed rewards make it faster.\nIt explodes on enemy contact or when its restarted timer ends.\nThe practice enemy takes no damage. Parry one to continue."),
				ERLTutorialStage::WaitingForExplosiveResolution);
			return;
		}

		if (TutorialStage == ERLTutorialStage::WaitingForRallyProjectile &&
			Projectile->IsRallyProjectile() && DistanceSquared <= SpecialDistanceSquared)
		{
			ShowTutorialPrompt(LOCTEXT("RallyTitle", "RALLY — HOMING RETURN"),
				LOCTEXT("RallyBody", "Rally shots bounce between enemies before coming toward you.\nParry one to make it home toward the nearest enemy.\nIf that enemy disappears, it seeks another; with none, it flies straight.\nSuccessfully parry one to continue."),
				ERLTutorialStage::WaitingForRallyParry);
			return;
		}

		if (TutorialStage == ERLTutorialStage::WaitingForGuardProjectile &&
			Projectile->IsGuardProjectile() && DistanceSquared <= SpecialDistanceSquared)
		{
			ShowTutorialPrompt(
				LOCTEXT("GuardTitle", "GUARD SHOT"),
				LOCTEXT(
					"GuardBody",
					"You can parry this projectile, but it cannot damage enemies.\nUse it defensively to keep your combo alive."),
				ERLTutorialStage::WaitingForCombo);
			return;
		}
	}
}

void ARLPlayerController::UpdateTutorialRingRequest(ARLGameModeBase& GameMode)
{
	if (TutorialRing.IsValid() && !TutorialRing->IsActorBeingDestroyed()) { return; }
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextTutorialProjectileRequestTimeSeconds) { return; }
	ARLExpandingRingAttack* Ring = GameMode.RequestTutorialRingAttack();
	if (!Ring)
	{
		NextTutorialProjectileRequestTimeSeconds = Now + 0.25f;
		return;
	}
	TutorialRing = Ring;
	Ring->OnPlayerDodged.AddUniqueDynamic(this, &ThisClass::HandleTutorialRingDodged);
	// Bind before activation so an immediate contact cannot lose the success event.
	if (!Ring->StartAttack())
	{
		Ring->Destroy();
		TutorialRing.Reset();
	}
	NextTutorialProjectileRequestTimeSeconds = Now + 0.75f;
}

void ARLPlayerController::HandleTutorialRingDodged(ARLPlayerCharacter* PlayerCharacter)
{
	if (TutorialStage != ERLTutorialStage::WaitingForRingDodge || PlayerCharacter != GetPawn()) { return; }
	QueueTutorialStage(ERLTutorialStage::WaitingForFinalCombatExplanation);
	// Successful practice ends this hazard; do not damage the player during the delay.
	if (TutorialRing.IsValid()) { TutorialRing->BeginFadeOut(); }
	TutorialRing.Reset();
}

void ARLPlayerController::UpdateTutorialProjectileRequest(ARLGameModeBase& GameMode)
{
	ERLProjectileBehavior RequestedBehavior;
	switch (TutorialStage)
	{
	case ERLTutorialStage::WaitingForNormalProjectile:
	case ERLTutorialStage::WaitingForSuccessfulParry:
	case ERLTutorialStage::WaitingForPerfectParry:
	case ERLTutorialStage::WaitingForCloseRangeParry:
		RequestedBehavior = ERLProjectileBehavior::Normal;
		break;
	case ERLTutorialStage::WaitingForExplosiveProjectile:
	case ERLTutorialStage::WaitingForExplosiveResolution:
		RequestedBehavior = ERLProjectileBehavior::Explosive;
		break;
	case ERLTutorialStage::WaitingForRallyProjectile:
	case ERLTutorialStage::WaitingForRallyParry:
		RequestedBehavior = ERLProjectileBehavior::Rally;
		break;
	case ERLTutorialStage::WaitingForGuardProjectile:
	case ERLTutorialStage::WaitingForCombo:
		RequestedBehavior = ERLProjectileBehavior::Guard;
		break;
	default:
		return;
	}

	if (HasActiveTutorialProjectile(RequestedBehavior))
	{
		return;
	}

	const float CurrentTimeSeconds = GetWorld()->GetTimeSeconds();
	if (CurrentTimeSeconds < NextTutorialProjectileRequestTimeSeconds)
	{
		return;
	}

	if (GameMode.RequestTutorialProjectile(RequestedBehavior))
	{
		NextTutorialProjectileRequestTimeSeconds = CurrentTimeSeconds + 0.75f;
	}
	else
	{
		NextTutorialProjectileRequestTimeSeconds = CurrentTimeSeconds + 0.25f;
	}
}

bool ARLPlayerController::HasActiveTutorialProjectile(
	ERLProjectileBehavior ProjectileBehavior, bool bReflected) const
{
	if (!GetWorld())
	{
		return false;
	}

	for (TActorIterator<ARLProjectile> Iterator(GetWorld()); Iterator; ++Iterator)
	{
		const ARLProjectile* Projectile = *Iterator;
		const URLProjectileDefinitionDataAsset* Definition =
			Projectile->GetProjectileDefinition();
		if (Projectile->IsPoolActive() && Projectile->IsReflected() == bReflected && Definition &&
			Definition->Behavior == ProjectileBehavior)
		{
			return true;
		}
	}
	return false;
}

void ARLPlayerController::ShowTutorialPrompt(
	const FText& Title,
	const FText& Body,
	ERLTutorialStage StageAfterDismiss)
{
	if (!TutorialPromptWidget || bTutorialPromptVisible || bPauseMenuVisible)
	{
		return;
	}

	TutorialStageAfterDismiss = StageAfterDismiss;
	bTutorialPromptVisible = true;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Tutorial prompt shown. Current stage: %d, next stage: %d."),
		static_cast<int32>(TutorialStage),
		static_cast<int32>(TutorialStageAfterDismiss));
	TutorialPromptWidget->SetPrompt(Title, Body);
	TutorialPromptWidget->SetVisibility(ESlateVisibility::Visible);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(TutorialPromptWidget->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	if (!UGameplayStatics::SetGamePaused(this, true))
	{
		UE_LOG(LogTemp, Warning, TEXT("Tutorial prompt could not pause the game."));
	}
}

void ARLPlayerController::ShowPerfectParryTutorial()
{
	ShowTutorialPrompt(
		LOCTEXT("PerfectParryTitle", "PERFECT PARRY"),
		LOCTEXT(
			"PerfectParryBody",
			"Parry a projectile in the outer ring to perform a perfect parry.\nA perfect parry splits the reflected shot into three projectiles and covers a wider area."),
		ERLTutorialStage::WaitingForPerfectParry);
}

void ARLPlayerController::DismissTutorialPrompt()
{
	if (!bTutorialPromptVisible || bPauseMenuVisible)
	{
		return;
	}

	bTutorialPromptVisible = false;
	TutorialStage = TutorialStageAfterDismiss;
	if (TutorialStage == ERLTutorialStage::WaitingForMovement)
	{
		bTutorialMovementObserved = false;
		bTutorialAimObserved = false;
		bTutorialMousePositionValid = false;
	}
	if (TutorialStage == ERLTutorialStage::WaitingForRoll) { bTutorialRollStarted = false; }
	if (TutorialStage == ERLTutorialStage::WaitingForExplosiveResolution) { bTutorialExplosiveParried = false; }
	if (TutorialStage == ERLTutorialStage::WaitingForCombo)
	{
		TutorialComboBaseline = TutorialBoundPlayer ? TutorialBoundPlayer->GetParryComboCount() : 0;
		QueueTutorialStage(ERLTutorialStage::WaitingForCombo);
	}
	else if (TutorialStage == ERLTutorialStage::WaitingForRingExplanation)
	{
		QueueTutorialStage(ERLTutorialStage::WaitingForRingExplanation);
	}
	NextTutorialProjectileRequestTimeSeconds = 0.0f;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Tutorial prompt dismissed. New stage: %d."),
		static_cast<int32>(TutorialStage));
	if (TutorialPromptWidget)
	{
		TutorialPromptWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	UGameplayStatics::SetGamePaused(this, false);
	if (TutorialStage == ERLTutorialStage::Complete)
	{
		if (ARLGameModeBase* GameMode = GetWorld()
			? GetWorld()->GetAuthGameMode<ARLGameModeBase>()
			: nullptr)
		{
			GameMode->CompleteTutorialCombatIntroduction();
		}
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ARLPlayerController::HandleTutorialParryComboChanged(
	int32 ComboCount,
	int32 MultiParryCount,
	int32 EnhancementLevel,
	bool bPerfectParry,
	bool bCloseRangeParry)
{
	(void)MultiParryCount;
	(void)EnhancementLevel;
	if (ComboCount > 0 && TutorialStage == ERLTutorialStage::WaitingForExplosiveResolution &&
		HasActiveTutorialProjectile(ERLProjectileBehavior::Explosive, true))
	{
		bTutorialExplosiveParried = true;
		return;
	}
	if (ComboCount > 0 && TutorialStage == ERLTutorialStage::WaitingForRallyParry &&
		HasActiveTutorialProjectile(ERLProjectileBehavior::Rally, true))
	{
		QueueTutorialStage(ERLTutorialStage::WaitingForGuardProjectile);
		return;
	}
	if (TutorialStage == ERLTutorialStage::WaitingForCombo && ComboCount == 0)
	{
		TutorialComboBaseline = 0;
	}

	if (ComboCount > 0 &&
		TutorialStage == ERLTutorialStage::WaitingForSuccessfulParry)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("Tutorial advanced from parry success event. Combo: %d."),
			ComboCount);
		QueueTutorialStage(ERLTutorialStage::WaitingForPerfectExplanation);
		return;
	}
	if (bPerfectParry && TutorialStage == ERLTutorialStage::WaitingForPerfectParry)
	{
		QueueTutorialStage(ERLTutorialStage::WaitingForCloseRangeExplanation);
		return;
	}

	if (bCloseRangeParry && TutorialStage == ERLTutorialStage::WaitingForCloseRangeParry)
	{
		QueueTutorialStage(ERLTutorialStage::WaitingForExplosiveProjectile);
		return;
	}
	if (ComboCount >= TutorialComboBaseline + 2 && TutorialStage == ERLTutorialStage::WaitingForCombo)
	{
		QueueTutorialStage(ERLTutorialStage::WaitingForComboExplanation);
	}
}

void ARLPlayerController::ShowComboTutorial()
{
	if (GetWorld())
	{
		ShowTutorialPrompt(
			LOCTEXT("ComboTitle", "BUILD THE COMBO"),
			LOCTEXT(
				"ComboBody",
				"Your combo continues until you miss a swing.\nReach 3, 5, and 8 consecutive parries to trigger stronger rewards."),
			ERLTutorialStage::WaitingForRingExplanation);
	}
}

bool ARLPlayerController::IsGameplayInputAllowed() const
{
	const ARLGameModeBase* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ARLGameModeBase>()
		: nullptr;
	return !bPauseMenuVisible && !bTutorialPromptVisible && !UGameplayStatics::IsGamePaused(this) &&
		(!GameMode || GameMode->GetRunState() == ERLRunState::PlayingRound);
}

#undef LOCTEXT_NAMESPACE

