#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RLPauseMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

enum class ERLPauseConfirmation : uint8
{
	None,
	RestartRun,
	MainMenu,
};

UCLASS()
class REFLECTIONLAB_API URLPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowMenu(bool bCanReturnToMainMenu);
	bool IsConfirming() const { return Confirmation != ERLPauseConfirmation::None; }
	void CancelConfirmation();

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildWidgetTree();
	void ShowConfirmation(ERLPauseConfirmation Action);
	UFUNCTION()
	void HandleResumeClicked();
	UFUNCTION()
	void HandleRestartClicked();
	UFUNCTION()
	void HandleMainMenuClicked();
	UFUNCTION()
	void HandleConfirmClicked();
	UFUNCTION()
	void HandleCancelClicked();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BodyText;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> MenuButtons;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ConfirmationButtons;
	UPROPERTY(Transient)
	TObjectPtr<UButton> ResumeButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> RestartButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> MainMenuButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> CancelButton;

	ERLPauseConfirmation Confirmation = ERLPauseConfirmation::None;
};
