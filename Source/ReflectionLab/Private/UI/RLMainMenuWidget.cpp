#include "UI/RLMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "ReflectionLabMainMenu"

namespace
{
	UButton* AddMenuButton(
		UWidgetTree& WidgetTree,
		UVerticalBox& Container,
		FName ButtonName,
		FName LabelName,
		const FText& Label)
	{
		USizeBox* ButtonSize = WidgetTree.ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			*FString::Printf(TEXT("%sSize"), *ButtonName.ToString()));
		ButtonSize->SetWidthOverride(360.0f);
		ButtonSize->SetHeightOverride(64.0f);

		UButton* Button = WidgetTree.ConstructWidget<UButton>(
			UButton::StaticClass(), ButtonName);
		UTextBlock* LabelWidget = WidgetTree.ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), LabelName);
		LabelWidget->SetText(Label);
		LabelWidget->SetJustification(ETextJustify::Center);
		LabelWidget->SetColorAndOpacity(FLinearColor::White);
		FSlateFontInfo LabelFont = LabelWidget->GetFont();
		LabelFont.Size = 24;
		LabelWidget->SetFont(LabelFont);
		Button->AddChild(LabelWidget);
		ButtonSize->AddChild(Button);

		UVerticalBoxSlot* Slot = Container.AddChildToVerticalBox(ButtonSize);
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetPadding(FMargin(0.0f, 8.0f));
		return Button;
	}
}

void URLMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildWidgetTree();
}

void URLMainMenuWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(FLinearColor(0.008f, 0.012f, 0.025f, 0.38f));
	UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(0.0f));

	UVerticalBox* MenuContainer = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("MenuContainer"));
	UCanvasPanelSlot* MenuSlot = RootCanvas->AddChildToCanvas(MenuContainer);
	MenuSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	MenuSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	MenuSlot->SetAutoSize(true);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(LOCTEXT("GameTitle", "REFLECTION LAB"));
	Title->SetJustification(ETextJustify::Center);
	Title->SetColorAndOpacity(FLinearColor(0.2f, 0.85f, 1.0f));
	Title->SetShadowOffset(FVector2D(4.0f, 4.0f));
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 58;
	Title->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = MenuContainer->AddChildToVerticalBox(Title);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 40.0f));

	UButton* TutorialButton = AddMenuButton(
		*WidgetTree,
		*MenuContainer,
		TEXT("TutorialButton"),
		TEXT("TutorialLabel"),
		LOCTEXT("TutorialButton", "TUTORIAL"));
	TutorialButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleTutorialClicked);

	UButton* StartGameButton = AddMenuButton(
		*WidgetTree,
		*MenuContainer,
		TEXT("StartGameButton"),
		TEXT("StartGameLabel"),
		LOCTEXT("StartGameButton", "START GAME"));
	StartGameButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleStartGameClicked);

	UButton* OptionsButton = AddMenuButton(
		*WidgetTree,
		*MenuContainer,
		TEXT("OptionsButton"),
		TEXT("OptionsLabel"),
		LOCTEXT("OptionsButton", "OPTIONS"));
	OptionsButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleOptionsClicked);

	UButton* ExitGameButton = AddMenuButton(
		*WidgetTree,
		*MenuContainer,
		TEXT("ExitGameButton"),
		TEXT("ExitGameLabel"),
		LOCTEXT("ExitGameButton", "EXIT GAME"));
	ExitGameButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleExitGameClicked);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetText(FText::GetEmpty());
	StatusText->SetJustification(ETextJustify::Center);
	StatusText->SetColorAndOpacity(FLinearColor(1.0f, 0.72f, 0.18f));
	FSlateFontInfo StatusFont = StatusText->GetFont();
	StatusFont.Size = 16;
	StatusText->SetFont(StatusFont);
	UVerticalBoxSlot* StatusSlot = MenuContainer->AddChildToVerticalBox(StatusText);
	StatusSlot->SetHorizontalAlignment(HAlign_Center);
	StatusSlot->SetPadding(FMargin(0.0f, 20.0f, 0.0f, 0.0f));
}

void URLMainMenuWidget::HandleTutorialClicked()
{
	OpenGameplayLevel(TEXT("Tutorial"));
}

void URLMainMenuWidget::HandleStartGameClicked()
{
	OpenGameplayLevel(TEXT("Main"));
}

void URLMainMenuWidget::HandleOptionsClicked()
{
	if (StatusText)
	{
		StatusText->SetText(LOCTEXT(
			"OptionsComingSoon",
			"Options will be added in the next UI pass."));
	}
}

void URLMainMenuWidget::HandleExitGameClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void URLMainMenuWidget::OpenGameplayLevel(const FString& RunMode) const
{
	if (GameplayLevelName.IsNone())
	{
		return;
	}
	UGameplayStatics::OpenLevel(
		this,
		GameplayLevelName,
		true,
		FString::Printf(TEXT("RunMode=%s"), *RunMode));
}

#undef LOCTEXT_NAMESPACE
