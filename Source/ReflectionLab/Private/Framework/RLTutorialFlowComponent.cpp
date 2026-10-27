#include "Framework/RLTutorialFlowComponent.h"
#include "Framework/PlayerController/RLPlayerController.h"
#include "Framework/GameMode/RLGameModeBase.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLExpandingRingAttack.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Player/RLPlayerCharacter.h"
#include "UI/RLTutorialPromptWidget.h"

#define LOCTEXT_NAMESPACE "ReflectionLabTutorial"

void URLTutorialFlowComponent::BindTutorialPlayer(ARLPlayerCharacter* PlayerCharacter)
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

void URLTutorialFlowComponent::QueueTutorialStage(ERLTutorialStage NextStage)
{
	if (!GetWorld() || TutorialStage == ERLTutorialStage::WaitingForStageDelay) { return; }
	PendingTutorialStage = NextStage;
	TutorialStage = ERLTutorialStage::WaitingForStageDelay;
	TutorialPromptReadyTimeSeconds = GetWorld()->GetTimeSeconds() + TutorialStageDelaySeconds;
}

void URLTutorialFlowComponent::UpdateTutorial()
{
	if (!bTutorialPromptsEnabled || bTutorialPromptVisible || Controller()->IsPauseMenuVisible() || !GetWorld())
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

	const APawn* ControlledPawn = Controller()->GetPawn();
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

void URLTutorialFlowComponent::UpdateTutorialRingRequest(ARLGameModeBase& GameMode)
{
	if (TutorialRing.IsValid() && TutorialRing->IsPoolActive() && !TutorialRing->IsActorBeingDestroyed()) { return; }
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
		Ring->ReturnToPool();
		TutorialRing.Reset();
	}
	NextTutorialProjectileRequestTimeSeconds = Now + 0.75f;
}

void URLTutorialFlowComponent::HandleTutorialRingDodged(ARLPlayerCharacter* PlayerCharacter)
{
	if (TutorialStage != ERLTutorialStage::WaitingForRingDodge || PlayerCharacter != Controller()->GetPawn()) { return; }
	QueueTutorialStage(ERLTutorialStage::WaitingForFinalCombatExplanation);
	// Successful practice ends this hazard; do not damage the player during the delay.
	if (TutorialRing.IsValid()) { TutorialRing->BeginFadeOut(); }
	TutorialRing.Reset();
}

void URLTutorialFlowComponent::UpdateTutorialProjectileRequest(ARLGameModeBase& GameMode)
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

bool URLTutorialFlowComponent::HasActiveTutorialProjectile(
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

void URLTutorialFlowComponent::ShowTutorialPrompt(
	const FText& Title,
	const FText& Body,
	ERLTutorialStage StageAfterDismiss)
{
	if (!TutorialPromptWidget || bTutorialPromptVisible || Controller()->IsPauseMenuVisible())
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
	Controller()->SetInputMode(InputMode);
	Controller()->bShowMouseCursor = true;
	if (!UGameplayStatics::SetGamePaused(this, true))
	{
		UE_LOG(LogTemp, Warning, TEXT("Tutorial prompt could not pause the game."));
	}
}

void URLTutorialFlowComponent::ShowPerfectParryTutorial()
{
	ShowTutorialPrompt(
		LOCTEXT("PerfectParryTitle", "PERFECT PARRY"),
		LOCTEXT(
			"PerfectParryBody",
			"Parry a projectile in the outer ring to perform a perfect parry.\nA perfect parry splits the reflected shot into three projectiles and covers a wider area."),
		ERLTutorialStage::WaitingForPerfectParry);
}

void URLTutorialFlowComponent::DismissTutorialPrompt()
{
	if (!bTutorialPromptVisible || Controller()->IsPauseMenuVisible())
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
	Controller()->SetInputMode(InputMode);
	Controller()->bShowMouseCursor = true;
}

void URLTutorialFlowComponent::HandleTutorialParryComboChanged(
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

void URLTutorialFlowComponent::ShowComboTutorial()
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

void URLTutorialFlowComponent::CreateOrBindTutorialPromptWidget()
{
	if (!Controller()->IsLocalController())
	{
		return;
	}

	if (!TutorialPromptWidget)
	{
		TutorialPromptWidget = CreateWidget<URLTutorialPromptWidget>(
			Controller(), URLTutorialPromptWidget::StaticClass());
		if (TutorialPromptWidget)
		{
			TutorialPromptWidget->AddToViewport(100);
			TutorialPromptWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	BindTutorialPlayer(Cast<ARLPlayerCharacter>(Controller()->GetPawn()));
}
URLTutorialFlowComponent::URLTutorialFlowComponent() { PrimaryComponentTick.bCanEverTick = false; }
ARLPlayerController* URLTutorialFlowComponent::Controller() const { return CastChecked<ARLPlayerController>(GetOwner()); }
void URLTutorialFlowComponent::Initialize(bool bEnabled, float NormalDistance, float SpecialDistance)
{
	bTutorialPromptsEnabled = bEnabled;
	TutorialNormalProjectileTriggerDistance = NormalDistance;
	TutorialSpecialProjectileTriggerDistance = SpecialDistance;
	CreateOrBindTutorialPromptWidget();
}
bool URLTutorialFlowComponent::IsRewardChoiceAllowed() const
{
	return !bTutorialPromptsEnabled || (TutorialStage == ERLTutorialStage::WaitingForRewardChoice && !bTutorialPromptVisible);
}
void URLTutorialFlowComponent::ObserveMovement()
{
	if (TutorialStage == ERLTutorialStage::WaitingForMovement) { bTutorialMovementObserved = true; }
}
void URLTutorialFlowComponent::ObserveRoll(bool bStarted)
{
	if (TutorialStage == ERLTutorialStage::WaitingForRoll && bStarted) { bTutorialRollStarted = true; }
}
void URLTutorialFlowComponent::ObserveAim()
{
	if (TutorialStage != ERLTutorialStage::WaitingForMovement) { return; }
	float MouseX = 0, MouseY = 0;
	if (Controller()->GetMousePosition(MouseX, MouseY))
	{
		const FVector2D MousePosition(MouseX, MouseY);
		bTutorialAimObserved |= bTutorialMousePositionValid && !MousePosition.Equals(TutorialMousePosition, 0.1f);
		TutorialMousePosition = MousePosition;
		bTutorialMousePositionValid = true;
	}
}
void URLTutorialFlowComponent::AbandonPrompt()
{
	bTutorialPromptVisible = false;
	if (TutorialPromptWidget) { TutorialPromptWidget->SetVisibility(ESlateVisibility::Collapsed); }
}
void URLTutorialFlowComponent::RestorePromptInputMode()
{
	if (!bTutorialPromptVisible || !TutorialPromptWidget) { return; }
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(TutorialPromptWidget->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Controller()->SetInputMode(InputMode);
	Controller()->DefaultMouseCursor = EMouseCursor::Default;
	Controller()->bShowMouseCursor = true;
}
void URLTutorialFlowComponent::UnbindPlayer()
{
	if (TutorialRing.IsValid())
	{
		TutorialRing->OnPlayerDodged.RemoveDynamic(this, &ThisClass::HandleTutorialRingDodged);
		TutorialRing->BeginFadeOut();
	}
	TutorialRing.Reset();
	if (bTutorialPromptVisible) { DismissTutorialPrompt(); }
	BindTutorialPlayer(nullptr);
}
void URLTutorialFlowComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	AbandonPrompt();
	UnbindPlayer();
	if (TutorialPromptWidget) { TutorialPromptWidget->RemoveFromParent(); }
	Super::EndPlay(Reason);
}
#undef LOCTEXT_NAMESPACE
