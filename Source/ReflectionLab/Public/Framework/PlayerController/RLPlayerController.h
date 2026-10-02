// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RLPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class URLParryComboWidget;
class URLPlayerHealthWidget;
class URLRunStatusWidget;
class URLTutorialPromptWidget;
class URLOffscreenEnemyWidget;
class ARLExpandingRingAttack;
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

UCLASS()
class REFLECTIONLAB_API ARLPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARLPlayerController();
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	void DismissTutorialPrompt();
	void SetGameplayHUDVisible(bool bVisible);
	bool IsTutorialRewardChoiceAllowed() const
	{
		return !bTutorialPromptsEnabled ||
			(TutorialStage == ERLTutorialStage::WaitingForRewardChoice && !bTutorialPromptVisible);
	}

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

private:
	void MoveForward();
	void MoveBackward();
	void MoveLeft();
	void MoveRight();
	void Move(const FVector2D& Direction);
	void ActivateParry();
	void ActivateRoll();
	void UpdateAimRotation();
	void RestoreGameplayInputMode();
	bool IsGameplayInputAllowed() const;
	void CreateOrBindParryComboWidget();
	void CreateOrBindPlayerHealthWidget();
	void CreateOrBindRunStatusWidget();
	void CreateOrBindTutorialPromptWidget();
	void CreateOffscreenEnemyWidget();
	void UpdateTutorial();
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
	bool HasActiveTutorialProjectile(ERLProjectileBehavior ProjectileBehavior) const;
	void BindTutorialPlayer(class ARLPlayerCharacter* PlayerCharacter);

	UFUNCTION()
	void HandleTutorialParryComboChanged(
		int32 ComboCount,
		int32 MultiParryCount,
		int32 EnhancementLevel,
		bool bPerfectParry,
		bool bCloseRangeParry);

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveForwardAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveBackwardAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveLeftAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveRightAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ReflectAction;

	UPROPERTY(Transient)
	TObjectPtr<URLParryComboWidget> ParryComboWidget;

	UPROPERTY(Transient)
	TObjectPtr<URLPlayerHealthWidget> PlayerHealthWidget;

	UPROPERTY(Transient)
	TObjectPtr<URLRunStatusWidget> RunStatusWidget;

	UPROPERTY(Transient)
	TObjectPtr<URLTutorialPromptWidget> TutorialPromptWidget;

	UPROPERTY(Transient)
	TObjectPtr<URLOffscreenEnemyWidget> OffscreenEnemyWidget;

	UPROPERTY(Transient)
	TObjectPtr<class ARLPlayerCharacter> TutorialBoundPlayer;

	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	bool bTutorialPromptsEnabled = true;

	UPROPERTY(EditDefaultsOnly, Category = "Tutorial", meta = (ClampMin = "100.0"))
	float TutorialNormalProjectileTriggerDistance = 750.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Tutorial", meta = (ClampMin = "100.0"))
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
	TWeakObjectPtr<ARLExpandingRingAttack> TutorialRing;
	static constexpr float TutorialStageDelaySeconds = 2.0f;
};
