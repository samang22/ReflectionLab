#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/GameMode/RLGameModeBase.h"
#include "RLRunStatusWidget.generated.h"

class UTextBlock;
class UButton;
class UVerticalBox;

UCLASS()
class REFLECTIONLAB_API URLRunStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindToGameMode(ARLGameModeBase* GameMode);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void HandleRunStateChanged(ERLRunState NewState, int32 RoundIndex);

	UFUNCTION()
	void HandleWaveChanged(
		int32 RoundIndex,
		int32 WaveIndex,
		FName WaveName);

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleMainMenuClicked();

	void BuildWidgetTree();
	void RefreshDisplay();
	void UpdateTimerText();
	void UpdateWaveText();
	void UpdateStateText();
	void UpdateResultsPanel();
	void UpdateInputMode();
	void UpdateExplosiveWarning(float DeltaTime);
	void SetStateMessage(const FText& Message, const FLinearColor& Color, int32 FontSize);
	static FText FormatTime(float Seconds);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RoundText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TimerText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PhaseText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NextPhaseText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StateText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ExplosiveWarningText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ResultsContainer;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResultsText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RestartButtonLabel;

	UPROPERTY(Transient)
	TObjectPtr<UButton> MainMenuButton;

	UPROPERTY(Transient)
	TObjectPtr<ARLGameModeBase> BoundGameMode;

	UPROPERTY(EditDefaultsOnly, Category = "Warning", meta = (ClampMin = "100.0"))
	float ExplosiveWarningDistance = 1400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Warning", meta = (ClampMin = "0.02"))
	float ExplosiveWarningUpdateInterval = 0.08f;

	UPROPERTY(EditDefaultsOnly, Category = "Run Presentation", meta = (ClampMin = "0.0"))
	float RoundClearMessageDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Run Presentation", meta = (ClampMin = "0.0"))
	float StartMessageDuration = 0.65f;

	float ExplosiveWarningUpdateAccumulator = 0.0f;
	float RoundClearMessageRemaining = 0.0f;
	float StartMessageRemaining = 0.0f;
};
