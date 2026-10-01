#include "UI/RLTutorialPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/PlayerController/RLPlayerController.h"

#define LOCTEXT_NAMESPACE "ReflectionLabTutorial"

void URLTutorialPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildWidgetTree();
}

void URLTutorialPromptWidget::SetPrompt(const FText& Title, const FText& Body)
{
	if (TitleText)
	{
		TitleText->SetText(Title);
	}
	if (BodyText)
	{
		BodyText->SetText(Body);
	}
}

void URLTutorialPromptWidget::HandleContinueClicked()
{
	if (ARLPlayerController* PlayerController = GetOwningPlayer<ARLPlayerController>())
	{
		PlayerController->DismissTutorialPrompt();
	}
}

void URLTutorialPromptWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("Dimmer"));
	Dimmer->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.68f));
	UCanvasPanelSlot* DimmerSlot = RootCanvas->AddChildToCanvas(Dimmer);
	DimmerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	DimmerSlot->SetOffsets(FMargin(0.0f));

	UBorder* PromptPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("PromptPanel"));
	PromptPanel->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.07f, 0.98f));
	PromptPanel->SetPadding(FMargin(48.0f, 36.0f));
	UCanvasPanelSlot* PromptSlot = RootCanvas->AddChildToCanvas(PromptPanel);
	PromptSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PromptSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PromptSlot->SetSize(FVector2D(900.0f, 500.0f));

	UVerticalBox* PromptContent = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("PromptContent"));
	PromptPanel->SetContent(PromptContent);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetColorAndOpacity(FLinearColor(0.1f, 0.9f, 1.0f));
	TitleText->SetShadowOffset(FVector2D(3.0f, 3.0f));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 38;
	TitleText->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = PromptContent->AddChildToVerticalBox(TitleText);
	TitleSlot->SetHorizontalAlignment(HAlign_Fill);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 22.0f));

	BodyText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("BodyText"));
	BodyText->SetJustification(ETextJustify::Center);
	BodyText->SetAutoWrapText(true);
	BodyText->SetColorAndOpacity(FLinearColor::White);
	FSlateFontInfo BodyFont = BodyText->GetFont();
	BodyFont.Size = 22;
	BodyText->SetFont(BodyFont);
	UVerticalBoxSlot* BodySlot = PromptContent->AddChildToVerticalBox(BodyText);
	BodySlot->SetHorizontalAlignment(HAlign_Fill);
	BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	BodySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 64.0f));

	ContinueButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(), TEXT("ContinueButton"));
	UTextBlock* ContinueLabel = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("ContinueLabel"));
	ContinueLabel->SetText(LOCTEXT("ContinueButton", "CONTINUE"));
	ContinueLabel->SetJustification(ETextJustify::Center);
	FSlateFontInfo ButtonFont = ContinueLabel->GetFont();
	ButtonFont.Size = 20;
	ContinueLabel->SetFont(ButtonFont);
	ContinueButton->AddChild(ContinueLabel);
	ContinueButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleContinueClicked);
	UVerticalBoxSlot* ButtonSlot = PromptContent->AddChildToVerticalBox(ContinueButton);
	ButtonSlot->SetHorizontalAlignment(HAlign_Center);
	ButtonSlot->SetPadding(FMargin(180.0f, 0.0f));
}

#undef LOCTEXT_NAMESPACE
