#include "UI/RLPauseMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/PlayerController/RLPlayerController.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "ReflectionLabPause"

void URLPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	BuildWidgetTree();
}

void URLPauseMenuWidget::ShowMenu(bool bCanReturnToMainMenu)
{
	if (MainMenuButton) { MainMenuButton->SetIsEnabled(bCanReturnToMainMenu); }
	CancelConfirmation();
}

void URLPauseMenuWidget::CancelConfirmation()
{
	Confirmation = ERLPauseConfirmation::None;
	if (TitleText) { TitleText->SetText(LOCTEXT("Paused", "PAUSED")); }
	if (BodyText) { BodyText->SetText(LOCTEXT("ResumeHint", "Press ESC to resume.")); }
	if (MenuButtons) { MenuButtons->SetVisibility(ESlateVisibility::Visible); }
	if (ConfirmationButtons) { ConfirmationButtons->SetVisibility(ESlateVisibility::Collapsed); }
	if (ResumeButton && IsInViewport()) { ResumeButton->SetKeyboardFocus(); }
}

void URLPauseMenuWidget::ShowConfirmation(ERLPauseConfirmation Action)
{
	Confirmation = Action;
	if (TitleText)
	{
		TitleText->SetText(Action == ERLPauseConfirmation::RestartRun
			? LOCTEXT("RestartTitle", "RESTART RUN?")
			: LOCTEXT("MainMenuTitle", "BACK TO MAIN MENU?"));
	}
	if (BodyText)
	{
		BodyText->SetText(LOCTEXT("ProgressWarning",
			"Your current run progress and rewards will be lost.\nAre you sure?"));
	}
	if (MenuButtons) { MenuButtons->SetVisibility(ESlateVisibility::Collapsed); }
	if (ConfirmationButtons) { ConfirmationButtons->SetVisibility(ESlateVisibility::Visible); }
	// Default to the safe action, not confirmation.
	if (CancelButton) { CancelButton->SetKeyboardFocus(); }
}

FReply URLPauseMenuWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (!InKeyEvent.IsRepeat())
		{
			if (ARLPlayerController* Controller = GetOwningPlayer<ARLPlayerController>())
			{
				Controller->TogglePauseMenu();
			}
		}
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void URLPauseMenuWidget::HandleResumeClicked()
{
	if (ARLPlayerController* Controller = GetOwningPlayer<ARLPlayerController>())
	{
		Controller->ResumeFromPauseMenu();
	}
}

void URLPauseMenuWidget::HandleRestartClicked()
{
	ShowConfirmation(ERLPauseConfirmation::RestartRun);
}

void URLPauseMenuWidget::HandleMainMenuClicked()
{
	if (MainMenuButton && MainMenuButton->GetIsEnabled())
	{
		ShowConfirmation(ERLPauseConfirmation::MainMenu);
	}
}

void URLPauseMenuWidget::HandleConfirmClicked()
{
	if (ARLPlayerController* Controller = GetOwningPlayer<ARLPlayerController>())
	{
		if (Confirmation == ERLPauseConfirmation::RestartRun) { Controller->RestartFromPauseMenu(); }
		else if (Confirmation == ERLPauseConfirmation::MainMenu) { Controller->ReturnToMainMenuFromPauseMenu(); }
	}
}

void URLPauseMenuWidget::HandleCancelClicked()
{
	CancelConfirmation();
}

void URLPauseMenuWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget) { return; }
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = Root;
	UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dimmer"));
	Dimmer->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f));
	UCanvasPanelSlot* DimmerSlot = Root->AddChildToCanvas(Dimmer);
	DimmerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	DimmerSlot->SetOffsets(FMargin(0.0f));

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PausePanel"));
	Panel->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.07f, 0.98f));
	Panel->SetPadding(FMargin(40.0f, 32.0f));
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetSize(FVector2D(560.0f, 440.0f));
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
	Panel->SetContent(Content);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetColorAndOpacity(FLinearColor(0.1f, 0.9f, 1.0f));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 32;
	TitleText->SetFont(TitleFont);
	Content->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Body"));
	BodyText->SetAutoWrapText(true);
	BodyText->SetJustification(ETextJustify::Center);
	FSlateFontInfo BodyFont = BodyText->GetFont();
	BodyFont.Size = 18;
	BodyText->SetFont(BodyFont);
	UVerticalBoxSlot* BodySlot = Content->AddChildToVerticalBox(BodyText);
	BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	BodySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));

	MenuButtons = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuButtons"));
	Content->AddChildToVerticalBox(MenuButtons);
	ConfirmationButtons = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ConfirmationButtons"));
	Content->AddChildToVerticalBox(ConfirmationButtons);
	auto AddButton = [this](UVerticalBox* Box, const TCHAR* Name, const FText& Label)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
		Button->SetBackgroundColor(FLinearColor(0.08f, 0.18f, 0.25f));
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(Label);
		Text->SetJustification(ETextJustify::Center);
		Text->SetMargin(FMargin(16.0f, 12.0f));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = 21;
		Text->SetFont(Font);
		Button->SetContent(Text);
		Box->AddChildToVerticalBox(Button)->SetPadding(FMargin(0.0f, 5.0f));
		return Button;
	};
	ResumeButton = AddButton(MenuButtons, TEXT("ResumeButton"), LOCTEXT("Resume", "RESUME"));
	RestartButton = AddButton(MenuButtons, TEXT("RestartButton"), LOCTEXT("Restart", "RESTART RUN"));
	MainMenuButton = AddButton(MenuButtons, TEXT("MainMenuButton"), LOCTEXT("MainMenu", "BACK TO MAIN MENU"));
	ConfirmButton = AddButton(ConfirmationButtons, TEXT("ConfirmButton"), LOCTEXT("Confirm", "CONFIRM"));
	CancelButton = AddButton(ConfirmationButtons, TEXT("CancelButton"), LOCTEXT("Cancel", "CANCEL"));
	ResumeButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleResumeClicked);
	RestartButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRestartClicked);
	MainMenuButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleMainMenuClicked);
	ConfirmButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirmClicked);
	CancelButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCancelClicked);
	ShowMenu(true);
}

#undef LOCTEXT_NAMESPACE
