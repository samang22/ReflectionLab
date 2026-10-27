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
class URLBossHealthWidget;
class URLTutorialFlowComponent;
class URLPauseMenuWidget;
class URLOffscreenEnemyWidget;

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
	void TogglePauseMenu();
	void TogglePlayerInvincibility();
	void ResumeFromPauseMenu();
	void RestartFromPauseMenu();
	void ReturnToMainMenuFromPauseMenu();
	bool IsTutorialRewardChoiceAllowed() const;
	bool IsPauseMenuVisible() const { return bPauseMenuVisible; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

private:
	void OpenPauseMenu();
	void RestoreModalInputMode();
	bool PreparePauseMenuTravel();
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
	void CreateOffscreenEnemyWidget();

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
	TObjectPtr<URLBossHealthWidget> BossHealthWidget;

	UPROPERTY(Transient)
	TObjectPtr<URLPauseMenuWidget> PauseMenuWidget;

	bool bPauseMenuVisible = false;

	UPROPERTY(Transient)
	TObjectPtr<URLOffscreenEnemyWidget> OffscreenEnemyWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Enemy Indicators")
	TSubclassOf<URLOffscreenEnemyWidget> OffscreenEnemyWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	bool bTutorialPromptsEnabled = true;

	UPROPERTY(EditDefaultsOnly, Category = "Tutorial", meta = (ClampMin = "100.0"))
	float TutorialNormalProjectileTriggerDistance = 750.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Tutorial", meta = (ClampMin = "100.0"))
	float TutorialSpecialProjectileTriggerDistance = 650.0f;

	UPROPERTY(VisibleAnywhere, Category = "Tutorial")
	TObjectPtr<URLTutorialFlowComponent> TutorialFlow;
};
