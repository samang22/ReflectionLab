#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLTutorialFlowComponent.generated.h"
class ARLPlayerController;
class ARLPlayerCharacter;
class ARLExpandingRingAttack;
class URLTutorialPromptWidget;
enum class ERLProjectileBehavior : uint8;
enum class ERLTutorialStage : uint8
{
	WaitingForTutorial,
	WaitingForMovement,
	WaitingForStageDelay,
	WaitingForRollExplanation,
	WaitingForRoll,
	WaitingForNormalProjectile,
	WaitingForSuccessfulParry,
	WaitingForPerfectExplanation,
	WaitingForPerfectParry,
	WaitingForCloseRangeExplanation,
	WaitingForCloseRangeParry,
	WaitingForExplosiveProjectile,
	WaitingForExplosiveResolution,
	WaitingForRallyProjectile,
	WaitingForRallyParry,
	WaitingForGuardProjectile,
	WaitingForCombo,
	WaitingForComboExplanation,
	WaitingForRingExplanation,
	WaitingForRingDodge,
	WaitingForFinalCombatExplanation,
	WaitingForRewardExplanation,
	WaitingForRewardChoice,
	Complete,
};

UCLASS(ClassGroup=(ReflectionLab))
class REFLECTIONLAB_API URLTutorialFlowComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URLTutorialFlowComponent();
	void Initialize(bool bEnabled, float NormalDistance, float SpecialDistance);
	void UpdateTutorial();
	void DismissTutorialPrompt();
	void UnbindPlayer();
	void AbandonPrompt();
	void RestorePromptInputMode();
	void ObserveMovement();
	void ObserveAim();
	void ObserveRoll(bool bStarted);
	bool IsPromptVisible() const { return bTutorialPromptVisible; }
	bool IsRewardChoiceAllowed() const;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	ARLPlayerController* Controller() const;
	void CreateOrBindTutorialPromptWidget();
	void QueueTutorialStage(ERLTutorialStage NextStage);
	void UpdateTutorialRingRequest(class ARLGameModeBase& GameMode);

	UFUNCTION()
	void HandleTutorialRingDodged(class ARLPlayerCharacter* PlayerCharacter);
	void ShowTutorialPrompt(
		const FText& Title,
		const FText& Body,
		ERLTutorialStage StageAfterDismiss);
	void ShowPerfectParryTutorial();
	void ShowComboTutorial();
	void UpdateTutorialProjectileRequest(class ARLGameModeBase& GameMode);
	bool HasActiveTutorialProjectile(ERLProjectileBehavior ProjectileBehavior, bool bReflected = false) const;
	void BindTutorialPlayer(class ARLPlayerCharacter* PlayerCharacter);

	UFUNCTION()
	void HandleTutorialParryComboChanged(
		int32 ComboCount,
		int32 MultiParryCount,
		int32 EnhancementLevel,
		bool bPerfectParry,
		bool bCloseRangeParry);

	UPROPERTY(Transient) TObjectPtr<URLTutorialPromptWidget> TutorialPromptWidget;
	UPROPERTY(Transient)
	TObjectPtr<class ARLPlayerCharacter> TutorialBoundPlayer;

	bool bTutorialPromptsEnabled = true;

	float TutorialNormalProjectileTriggerDistance = 750.0f;

	float TutorialSpecialProjectileTriggerDistance = 650.0f;

	ERLTutorialStage TutorialStage = ERLTutorialStage::WaitingForTutorial;
	ERLTutorialStage TutorialStageAfterDismiss = ERLTutorialStage::WaitingForTutorial;
	bool bTutorialPromptVisible = false;
	float NextTutorialProjectileRequestTimeSeconds = 0.0f;
	float TutorialPromptReadyTimeSeconds = 0.0f;
	ERLTutorialStage PendingTutorialStage = ERLTutorialStage::WaitingForTutorial;
	bool bTutorialMovementObserved = false;
	bool bTutorialAimObserved = false;
	bool bTutorialMousePositionValid = false;
	FVector2D TutorialMousePosition = FVector2D::ZeroVector;
	bool bTutorialRollStarted = false;
	int32 TutorialComboBaseline = 0;
	bool bTutorialExplosiveParried = false;
	TWeakObjectPtr<ARLExpandingRingAttack> TutorialRing;
	static constexpr float TutorialStageDelaySeconds = 2.0f;
};
