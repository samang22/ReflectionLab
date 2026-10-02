// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/PlayerController/RLPlayerController.h"

#include "Camera/PlayerCameraManager.h"
#include "Combat/RLProjectile.h"
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
#include "UI/RLOffscreenEnemyWidget.h"
#include "UI/RLTutorialPromptWidget.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "ReflectionLabTutorial"

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
			this, URLOffscreenEnemyWidget::StaticClass());
		if (OffscreenEnemyWidget)
		{
			OffscreenEnemyWidget->AddToViewport(5);
		}
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
			GameMode->GetRunState() == ERLRunState::PlayingRound;
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
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ThisClass::ActivateRoll);

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
		TutorialStage = ERLTutorialStage::WaitingForNormalProjectile;
		NextTutorialProjectileRequestTimeSeconds = 0.0f;
	}
}

void ARLPlayerController::ActivateRoll()
{
	if (!IsGameplayInputAllowed())
	{
		return;
	}
	UpdateAimRotation();
	if (ARLPlayerCharacter* RollingCharacter = Cast<ARLPlayerCharacter>(GetPawn()))
	{
		RollingCharacter->StartRoll(GetControlRotation().Vector());
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

void ARLPlayerController::UpdateTutorial()
{
	if (!bTutorialPromptsEnabled || bTutorialPromptVisible || !GetWorld())
	{
		return;
	}

	ARLGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ARLGameModeBase>();
	if (!GameMode || !GameMode->IsCurrentRoundTutorial())
	{
		return;
	}
	if (GameMode->IsChoosingReward() && TutorialStage != ERLTutorialStage::WaitingForRewardChoice)
	{
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
	if (TutorialStage == ERLTutorialStage::WaitingForExplosiveDelay)
	{
		if (GetWorld()->GetTimeSeconds() < TutorialPromptReadyTimeSeconds)
		{
			return;
		}
		TutorialStage = ERLTutorialStage::WaitingForExplosiveProjectile;
		NextTutorialProjectileRequestTimeSeconds = 0.0f;
	}
	if (TutorialStage == ERLTutorialStage::WaitingForPerfectExplanation)
	{
		if (GetWorld()->GetTimeSeconds() >= TutorialPromptReadyTimeSeconds)
		{
			ShowPerfectParryTutorial();
		}
		return;
	}
	if (TutorialStage == ERLTutorialStage::WaitingForCloseRangeExplanation)
	{
		if (GetWorld()->GetTimeSeconds() < TutorialPromptReadyTimeSeconds)
		{
			return;
		}
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
				"Use WASD to move. Your character faces the mouse cursor.\nMove once after closing this message to continue."),
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
		TutorialStage = ERLTutorialStage::WaitingForPerfectExplanation;
		TutorialPromptReadyTimeSeconds = GetWorld()->GetTimeSeconds() + 2.0f;
		return;
	}

	if (TutorialStage == ERLTutorialStage::WaitingForExplosiveResolution)
	{
		if (!HasActiveTutorialProjectile(ERLProjectileBehavior::Explosive))
		{
			TutorialStage = ERLTutorialStage::WaitingForGuardProjectile;
			NextTutorialProjectileRequestTimeSeconds = 0.0f;
			UE_LOG(
				LogTemp,
				Display,
				TEXT("Tutorial explosive resolved; guard projectile stage started."));
		}
		else
		{
			return;
		}
	}

	UpdateTutorialProjectileRequest(*GameMode);
	if (TutorialStage == ERLTutorialStage::WaitingForCombo && TutorialBoundPlayer &&
		TutorialBoundPlayer->GetParryComboCount() >= 2)
	{
		ShowComboTutorial();
	}

	if (TutorialStage != ERLTutorialStage::WaitingForNormalProjectile &&
		TutorialStage != ERLTutorialStage::WaitingForExplosiveProjectile &&
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
				LOCTEXT("ExplosiveTitle", "EXPLOSIVE — DODGE"),
				LOCTEXT(
					"ExplosiveBody",
					"Large red blinking projectiles explode immediately if you try to parry them.\nUse WASD to leave the marked blast area."),
				ERLTutorialStage::WaitingForExplosiveResolution);
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
		RequestedBehavior = ERLProjectileBehavior::Explosive;
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
	ERLProjectileBehavior ProjectileBehavior) const
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
		if (Projectile->IsPoolActive() && !Projectile->IsReflected() && Definition &&
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
	if (!TutorialPromptWidget || bTutorialPromptVisible)
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
	if (!bTutorialPromptVisible)
	{
		return;
	}

	bTutorialPromptVisible = false;
	TutorialStage = TutorialStageAfterDismiss;
	if (TutorialStage == ERLTutorialStage::WaitingForCombo)
	{
		TutorialPromptReadyTimeSeconds = GetWorld()->GetTimeSeconds() + 2.0f;
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

	if (ComboCount > 0 &&
		TutorialStage == ERLTutorialStage::WaitingForSuccessfulParry)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("Tutorial advanced from parry success event. Combo: %d."),
			ComboCount);
		TutorialStage = ERLTutorialStage::WaitingForPerfectExplanation;
		TutorialPromptReadyTimeSeconds = GetWorld()->GetTimeSeconds() + 2.0f;
		return;
	}
	if (bPerfectParry && TutorialStage == ERLTutorialStage::WaitingForPerfectParry)
	{
		TutorialStage = ERLTutorialStage::WaitingForCloseRangeExplanation;
		TutorialPromptReadyTimeSeconds = GetWorld()->GetTimeSeconds() + 2.0f;
		return;
	}

	if (bCloseRangeParry && TutorialStage == ERLTutorialStage::WaitingForCloseRangeParry)
	{
		TutorialStage = ERLTutorialStage::WaitingForExplosiveDelay;
		TutorialPromptReadyTimeSeconds = GetWorld()->GetTimeSeconds() + 2.0f;
		return;
	}
	if (ComboCount >= 2 && TutorialStage == ERLTutorialStage::WaitingForCombo)
	{
		ShowComboTutorial();
	}
}

void ARLPlayerController::ShowComboTutorial()
{
	if (GetWorld() && GetWorld()->GetTimeSeconds() >= TutorialPromptReadyTimeSeconds)
	{
		ShowTutorialPrompt(
			LOCTEXT("ComboTitle", "BUILD THE COMBO"),
			LOCTEXT(
				"ComboBody",
				"Your combo continues until you miss a swing.\nReach 3, 5, and 8 consecutive parries to trigger stronger rewards."),
			ERLTutorialStage::Complete);
	}
}

bool ARLPlayerController::IsGameplayInputAllowed() const
{
	const ARLGameModeBase* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ARLGameModeBase>()
		: nullptr;
	return !bTutorialPromptVisible &&
		(!GameMode || GameMode->GetRunState() == ERLRunState::PlayingRound);
}

#undef LOCTEXT_NAMESPACE

