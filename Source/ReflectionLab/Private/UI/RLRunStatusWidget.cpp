#include "UI/RLRunStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Combat/RLProjectile.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Framework/PlayerController/RLPlayerController.h"

namespace
{
	const TCHAR* GetRewardArtPath(ERLRunRewardType RewardType)
	{
		switch (RewardType)
		{
		case ERLRunRewardType::WiderArc:
			return TEXT("/Game/ReflectionLab/UI/Rewards/Textures/T_Reward_WideSwing.T_Reward_WideSwing");
		case ERLRunRewardType::ExtendedRange:
			return TEXT("/Game/ReflectionLab/UI/Rewards/Textures/T_Reward_LongReach.T_Reward_LongReach");
		case ERLRunRewardType::PiercingReturn:
			return TEXT("/Game/ReflectionLab/UI/Rewards/Textures/T_Reward_PiercingReturn.T_Reward_PiercingReturn");
		case ERLRunRewardType::PerfectFocus:
			return TEXT("/Game/ReflectionLab/UI/Rewards/Textures/T_Reward_PerfectVolley.T_Reward_PerfectVolley");
		case ERLRunRewardType::VelocityDrive:
			return TEXT("/Game/ReflectionLab/UI/Rewards/Textures/T_Reward_VelocityDrive.T_Reward_VelocityDrive");
		case ERLRunRewardType::CloseCall:
			return TEXT("/Game/ReflectionLab/UI/Rewards/Textures/T_Reward_CloseCall.T_Reward_CloseCall");
		default:
			return nullptr;
		}
	}

	FLinearColor GetRewardAccentColor(ERLRunRewardType RewardType)
	{
		switch (RewardType)
		{
		case ERLRunRewardType::WiderArc:
			return FLinearColor(0.1f, 0.85f, 1.0f);
		case ERLRunRewardType::ExtendedRange:
			return FLinearColor(0.25f, 1.0f, 0.55f);
		case ERLRunRewardType::PiercingReturn:
			return FLinearColor(0.72f, 0.35f, 1.0f);
		case ERLRunRewardType::PerfectFocus:
			return FLinearColor(1.0f, 0.72f, 0.12f);
		case ERLRunRewardType::VelocityDrive:
			return FLinearColor(1.0f, 0.38f, 0.08f);
		case ERLRunRewardType::CloseCall:
			return FLinearColor(1.0f, 0.12f, 0.08f);
		default:
			return FLinearColor(0.2f, 0.9f, 1.0f);
		}
	}
}

void URLRunStatusWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildWidgetTree();
	RefreshDisplay();
}

void URLRunStatusWidget::NativeDestruct()
{
	BindToGameMode(nullptr);
	Super::NativeDestruct();
}

void URLRunStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RoundClearMessageRemaining = FMath::Max(
		0.0f,
		RoundClearMessageRemaining - InDeltaTime);
	StartMessageRemaining = FMath::Max(0.0f, StartMessageRemaining - InDeltaTime);
	UpdateTimerText();
	UpdateWaveText();
	UpdateStateText();
	UpdateExplosiveWarning(InDeltaTime);
}

void URLRunStatusWidget::BindToGameMode(ARLGameModeBase* GameMode)
{
	if (BoundGameMode)
	{
		BoundGameMode->OnRunStateChanged.RemoveDynamic(
			this,
			&ThisClass::HandleRunStateChanged);
		BoundGameMode->OnWaveChanged.RemoveDynamic(
			this,
			&ThisClass::HandleWaveChanged);
	}

	BoundGameMode = GameMode;
	if (BoundGameMode)
	{
		BoundGameMode->OnRunStateChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandleRunStateChanged);
		BoundGameMode->OnWaveChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandleWaveChanged);
	}

	RefreshDisplay();
}

void URLRunStatusWidget::HandleRunStateChanged(
	ERLRunState NewState,
	int32 RoundIndex)
{
	(void)RoundIndex;
	RoundClearMessageRemaining =
		NewState == ERLRunState::Intermission || NewState == ERLRunState::RunCompleted
			? RoundClearMessageDuration
			: 0.0f;
	StartMessageRemaining = NewState == ERLRunState::PlayingRound
		? StartMessageDuration
		: 0.0f;
	RefreshDisplay();
	UpdateInputMode();
}

void URLRunStatusWidget::HandleWaveChanged(
	int32 RoundIndex,
	int32 WaveIndex,
	FName WaveName)
{
	(void)RoundIndex;
	(void)WaveIndex;
	(void)WaveName;
	if (PhaseText)
	{
		UpdateWaveText();
	}
}

void URLRunStatusWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	StatusContainer = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("StatusContainer"));
	UCanvasPanelSlot* StatusSlot = RootCanvas->AddChildToCanvas(StatusContainer);
	StatusSlot->SetAnchors(FAnchors(0.5f, 0.025f));
	StatusSlot->SetAlignment(FVector2D(0.5f, 0.0f));
	StatusSlot->SetAutoSize(true);

	RoundText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("RoundText"));
	RoundText->SetJustification(ETextJustify::Center);
	RoundText->SetColorAndOpacity(FLinearColor(0.55f, 0.85f, 1.0f));
	RoundText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	FSlateFontInfo RoundFont = RoundText->GetFont();
	RoundFont.Size = 20;
	RoundText->SetFont(RoundFont);
	UVerticalBoxSlot* RoundSlot = StatusContainer->AddChildToVerticalBox(RoundText);
	RoundSlot->SetHorizontalAlignment(HAlign_Center);

	TimerText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("TimerText"));
	TimerText->SetJustification(ETextJustify::Center);
	TimerText->SetColorAndOpacity(FLinearColor::White);
	TimerText->SetShadowOffset(FVector2D(3.0f, 3.0f));
	TimerText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	FSlateFontInfo TimerFont = TimerText->GetFont();
	TimerFont.Size = 36;
	TimerText->SetFont(TimerFont);
	UVerticalBoxSlot* TimerSlot = StatusContainer->AddChildToVerticalBox(TimerText);
	TimerSlot->SetHorizontalAlignment(HAlign_Center);

	PhaseText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("PhaseText"));
	PhaseText->SetJustification(ETextJustify::Center);
	PhaseText->SetColorAndOpacity(FLinearColor(0.7f, 0.75f, 0.85f));
	FSlateFontInfo PhaseFont = PhaseText->GetFont();
	PhaseFont.Size = 14;
	PhaseText->SetFont(PhaseFont);
	UVerticalBoxSlot* PhaseSlot = StatusContainer->AddChildToVerticalBox(PhaseText);
	PhaseSlot->SetHorizontalAlignment(HAlign_Center);

	NextPhaseText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("NextPhaseText"));
	NextPhaseText->SetJustification(ETextJustify::Center);
	NextPhaseText->SetColorAndOpacity(FLinearColor(0.5f, 0.65f, 0.8f));
	FSlateFontInfo NextPhaseFont = NextPhaseText->GetFont();
	NextPhaseFont.Size = 12;
	NextPhaseText->SetFont(NextPhaseFont);
	UVerticalBoxSlot* NextPhaseSlot = StatusContainer->AddChildToVerticalBox(NextPhaseText);
	NextPhaseSlot->SetHorizontalAlignment(HAlign_Center);

	StateText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("StateText"));
	StateText->SetJustification(ETextJustify::Center);
	StateText->SetColorAndOpacity(FLinearColor(1.0f, 0.75f, 0.15f));
	StateText->SetShadowOffset(FVector2D(4.0f, 4.0f));
	StateText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
	FSlateFontInfo StateFont = StateText->GetFont();
	StateFont.Size = 48;
	StateText->SetFont(StateFont);
	UCanvasPanelSlot* CenterSlot = RootCanvas->AddChildToCanvas(StateText);
	CenterSlot->SetAnchors(FAnchors(0.5f, 0.42f));
	CenterSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CenterSlot->SetAutoSize(true);

	ExplosiveWarningText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ExplosiveWarningText"));
	ExplosiveWarningText->SetJustification(ETextJustify::Center);
	ExplosiveWarningText->SetColorAndOpacity(FLinearColor(1.0f, 0.08f, 0.02f));
	ExplosiveWarningText->SetShadowOffset(FVector2D(3.0f, 3.0f));
	ExplosiveWarningText->SetShadowColorAndOpacity(
		FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
	FSlateFontInfo WarningFont = ExplosiveWarningText->GetFont();
	WarningFont.Size = 28;
	ExplosiveWarningText->SetFont(WarningFont);
	ExplosiveWarningText->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* WarningSlot = RootCanvas->AddChildToCanvas(ExplosiveWarningText);
	WarningSlot->SetAnchors(FAnchors(0.5f, 0.78f));
	WarningSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	WarningSlot->SetAutoSize(true);

	ResultsContainer = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("ResultsContainer"));
	UCanvasPanelSlot* ResultsSlot = RootCanvas->AddChildToCanvas(ResultsContainer);
	ResultsSlot->SetAnchors(FAnchors(0.5f, 0.58f));
	ResultsSlot->SetAlignment(FVector2D(0.5f, 0.0f));
	ResultsSlot->SetAutoSize(true);
	ResultsContainer->SetVisibility(ESlateVisibility::Collapsed);

	ResultsText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ResultsText"));
	ResultsText->SetJustification(ETextJustify::Center);
	ResultsText->SetColorAndOpacity(FLinearColor::White);
	ResultsText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	FSlateFontInfo ResultsFont = ResultsText->GetFont();
	ResultsFont.Size = 24;
	ResultsText->SetFont(ResultsFont);
	UVerticalBoxSlot* ResultsTextSlot = ResultsContainer->AddChildToVerticalBox(ResultsText);
	ResultsTextSlot->SetHorizontalAlignment(HAlign_Center);
	ResultsTextSlot->SetPadding(FMargin(8.0f, 4.0f, 8.0f, 16.0f));

	RestartButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("RestartButton"));
	RestartButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("RestartLabel"));
	RestartButtonLabel->SetText(FText::FromString(TEXT("RETRY")));
	RestartButtonLabel->SetJustification(ETextJustify::Center);
	RestartButton->AddChild(RestartButtonLabel);
	RestartButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRestartClicked);
	UVerticalBoxSlot* RestartSlot = ResultsContainer->AddChildToVerticalBox(RestartButton);
	RestartSlot->SetHorizontalAlignment(HAlign_Fill);
	RestartSlot->SetPadding(FMargin(24.0f, 4.0f));

	MainMenuButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("MainMenuButton"));
	UTextBlock* MainMenuLabel = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("MainMenuLabel"));
	MainMenuLabel->SetText(FText::FromString(TEXT("MAIN MENU")));
	MainMenuLabel->SetJustification(ETextJustify::Center);
	MainMenuButton->AddChild(MainMenuLabel);
	MainMenuButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleMainMenuClicked);
	UVerticalBoxSlot* MainMenuSlot = ResultsContainer->AddChildToVerticalBox(MainMenuButton);
	MainMenuSlot->SetHorizontalAlignment(HAlign_Fill);
	MainMenuSlot->SetPadding(FMargin(24.0f, 4.0f));

	RewardBackdrop = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("RewardBackdrop"));
	RewardBackdrop->SetBrushColor(FLinearColor(0.005f, 0.012f, 0.03f, 0.92f));
	RewardBackdrop->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* RewardBackdropSlot = RootCanvas->AddChildToCanvas(RewardBackdrop);
	RewardBackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	RewardBackdropSlot->SetOffsets(FMargin(0.0f));

	RewardPromptText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("RewardPromptText"));
	RewardPromptText->SetText(FText::FromString(TEXT("CHOOSE ONE\nREFLECTION PROTOCOL")));
	RewardPromptText->SetJustification(ETextJustify::Center);
	RewardPromptText->SetColorAndOpacity(FLinearColor(0.45f, 0.9f, 1.0f));
	RewardPromptText->SetShadowOffset(FVector2D(3.0f, 3.0f));
	RewardPromptText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
	FSlateFontInfo RewardPromptFont = RewardPromptText->GetFont();
	RewardPromptFont.Size = 28;
	RewardPromptText->SetFont(RewardPromptFont);
	RewardPromptText->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* RewardPromptSlot = RootCanvas->AddChildToCanvas(RewardPromptText);
	RewardPromptSlot->SetAnchors(FAnchors(0.5f, 0.06f));
	RewardPromptSlot->SetAlignment(FVector2D(0.5f, 0.0f));
	RewardPromptSlot->SetAutoSize(true);

	RewardContainer = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("RewardContainer"));
	UCanvasPanelSlot* RewardSlot = RootCanvas->AddChildToCanvas(RewardContainer);
	RewardSlot->SetAnchors(FAnchors(0.06f, 0.16f, 0.94f, 0.86f));
	RewardSlot->SetOffsets(FMargin(0.0f));
	RewardContainer->SetVisibility(ESlateVisibility::Collapsed);
	RewardArtTextures.SetNum(static_cast<int32>(ERLRunRewardType::CloseCall) + 1);
	for (int32 RewardIndex = 0; RewardIndex < RewardArtTextures.Num(); ++RewardIndex)
	{
		const ERLRunRewardType RewardType = static_cast<ERLRunRewardType>(RewardIndex);
		if (const TCHAR* ArtPath = GetRewardArtPath(RewardType))
		{
			RewardArtTextures[RewardIndex] = LoadObject<UTexture2D>(nullptr, ArtPath);
		}
	}

	for (int32 ChoiceIndex = 0; ChoiceIndex < 3; ++ChoiceIndex)
	{
		UButton* RewardButton = WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(),
			*FString::Printf(TEXT("RewardChoiceButton%d"), ChoiceIndex));
		FButtonStyle TransparentButtonStyle = RewardButton->GetStyle();
		TransparentButtonStyle.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		TransparentButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		TransparentButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		TransparentButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		RewardButton->SetStyle(TransparentButtonStyle);
		UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			*FString::Printf(TEXT("RewardCardBorder%d"), ChoiceIndex));
		CardBorder->SetBrushColor(FLinearColor(0.025f, 0.06f, 0.12f, 0.98f));
		UButtonSlot* CardButtonSlot = Cast<UButtonSlot>(RewardButton->AddChild(CardBorder));
		CardButtonSlot->SetPadding(FMargin(0.0f));
		CardButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		CardButtonSlot->SetVerticalAlignment(VAlign_Fill);

		UVerticalBox* RewardContent = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			*FString::Printf(TEXT("RewardChoiceContent%d"), ChoiceIndex));
		UBorderSlot* CardBorderSlot = Cast<UBorderSlot>(CardBorder->AddChild(RewardContent));
		CardBorderSlot->SetPadding(FMargin(12.0f));
		CardBorderSlot->SetHorizontalAlignment(HAlign_Fill);
		CardBorderSlot->SetVerticalAlignment(VAlign_Fill);

		UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("RewardTitle%d"), ChoiceIndex));
		TitleText->SetJustification(ETextJustify::Center);
		TitleText->SetColorAndOpacity(FLinearColor(0.4f, 0.9f, 1.0f));
		TitleText->SetAutoWrapText(true);
		FSlateFontInfo TitleFont = TitleText->GetFont();
		TitleFont.Size = 28;
		TitleText->SetFont(TitleFont);
		USizeBox* TitleSizeBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			*FString::Printf(TEXT("RewardTitleSize%d"), ChoiceIndex));
		TitleSizeBox->SetHeightOverride(80.0f);
		TitleSizeBox->AddChild(TitleText);
		UVerticalBoxSlot* TitleSlot = RewardContent->AddChildToVerticalBox(TitleSizeBox);
		TitleSlot->SetPadding(FMargin(12.0f, 14.0f, 12.0f, 10.0f));
		TitleSlot->SetHorizontalAlignment(HAlign_Fill);

		USizeBox* ArtSizeBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			*FString::Printf(TEXT("RewardArtSize%d"), ChoiceIndex));
		ArtSizeBox->SetHeightOverride(220.0f);
		UBorder* ArtPanel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			*FString::Printf(TEXT("RewardArtPanel%d"), ChoiceIndex));
		ArtPanel->SetBrushColor(FLinearColor(0.04f, 0.18f, 0.28f, 1.0f));
		ArtSizeBox->AddChild(ArtPanel);

		UScaleBox* ArtScaleBox = WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(),
			*FString::Printf(TEXT("RewardArtScale%d"), ChoiceIndex));
		ArtScaleBox->SetStretch(EStretch::ScaleToFill);
		ArtScaleBox->SetStretchDirection(EStretchDirection::Both);
		ArtPanel->AddChild(ArtScaleBox);

		UImage* ArtImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			*FString::Printf(TEXT("RewardArtImage%d"), ChoiceIndex));
		ArtImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		ArtScaleBox->AddChild(ArtImage);
		UVerticalBoxSlot* ArtSlot = RewardContent->AddChildToVerticalBox(ArtSizeBox);
		ArtSlot->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 12.0f));
		ArtSlot->SetHorizontalAlignment(HAlign_Fill);

		UTextBlock* EffectLabel = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("RewardEffectLabel%d"), ChoiceIndex));
		EffectLabel->SetText(FText::FromString(TEXT("REFLECTION PROTOCOL")));
		EffectLabel->SetJustification(ETextJustify::Center);
		EffectLabel->SetColorAndOpacity(FLinearColor(0.72f, 0.78f, 0.92f));
		FSlateFontInfo EffectFont = EffectLabel->GetFont();
		EffectFont.Size = 12;
		EffectLabel->SetFont(EffectFont);
		UVerticalBoxSlot* EffectSlot = RewardContent->AddChildToVerticalBox(EffectLabel);
		EffectSlot->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 4.0f));
		EffectSlot->SetHorizontalAlignment(HAlign_Center);

		UTextBlock* DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("RewardDescription%d"), ChoiceIndex));
		DescriptionText->SetJustification(ETextJustify::Center);
		DescriptionText->SetColorAndOpacity(FLinearColor(0.9f, 0.95f, 1.0f));
		DescriptionText->SetAutoWrapText(true);
		DescriptionText->SetWrapTextAt(360.0f);
		FSlateFontInfo DescriptionFont = DescriptionText->GetFont();
		DescriptionFont.Size = 17;
		DescriptionText->SetFont(DescriptionFont);
		UVerticalBoxSlot* DescriptionSlot = RewardContent->AddChildToVerticalBox(DescriptionText);
		DescriptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		DescriptionSlot->SetPadding(FMargin(18.0f, 4.0f, 18.0f, 10.0f));
		DescriptionSlot->SetHorizontalAlignment(HAlign_Fill);
		DescriptionSlot->SetVerticalAlignment(VAlign_Center);

		UTextBlock* SelectLabel = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("RewardSelectLabel%d"), ChoiceIndex));
		SelectLabel->SetText(FText::FromString(TEXT("CLICK TO SELECT")));
		SelectLabel->SetJustification(ETextJustify::Center);
		SelectLabel->SetColorAndOpacity(FLinearColor(0.92f, 0.96f, 1.0f));
		FSlateFontInfo SelectFont = SelectLabel->GetFont();
		SelectFont.Size = 13;
		SelectLabel->SetFont(SelectFont);
		UVerticalBoxSlot* SelectSlot = RewardContent->AddChildToVerticalBox(SelectLabel);
		SelectSlot->SetPadding(FMargin(12.0f, 4.0f, 12.0f, 14.0f));
		SelectSlot->SetHorizontalAlignment(HAlign_Center);

		switch (ChoiceIndex)
		{
		case 0:
			RewardButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRewardChoiceOneClicked);
			break;
		case 1:
			RewardButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRewardChoiceTwoClicked);
			break;
		default:
			RewardButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRewardChoiceThreeClicked);
			break;
		}

		UHorizontalBoxSlot* ButtonSlot = RewardContainer->AddChildToHorizontalBox(RewardButton);
		ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ButtonSlot->SetPadding(FMargin(14.0f));
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		ButtonSlot->SetVerticalAlignment(VAlign_Fill);
		RewardButtons.Add(RewardButton);
		RewardTitleTexts.Add(TitleText);
		RewardDescriptionTexts.Add(DescriptionText);
		RewardArtPanels.Add(ArtPanel);
		RewardArtImages.Add(ArtImage);
	}
}

void URLRunStatusWidget::RefreshDisplay()
{
	if (!BoundGameMode)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const ERLRunState RunState = BoundGameMode->GetRunState();
	const bool bShowingResults =
		RunState == ERLRunState::TutorialCompleted ||
		RunState == ERLRunState::RunCompleted || RunState == ERLRunState::GameOver;
	const bool bChoosingReward = RunState == ERLRunState::RewardSelection;
	SetVisibility((bShowingResults || bChoosingReward)
		? ESlateVisibility::Visible
		: ESlateVisibility::HitTestInvisible);
	const int32 RoundCount = BoundGameMode->GetRoundCount();
	if (RoundText)
	{
		RoundText->SetText(BoundGameMode->IsCurrentRoundTutorial()
			? FText::FromString(TEXT("TUTORIAL"))
			: FText::FromString(FString::Printf(
				TEXT("ROUND %d / %d"),
				FMath::Max(1, BoundGameMode->GetCurrentRoundNumber()),
				FMath::Max(1, RoundCount))));
	}

	if (PhaseText)
	{
		UpdateWaveText();
	}

	UpdateTimerText();
	UpdateStateText();
	UpdateResultsPanel();
	UpdateRewardChoices();
}

void URLRunStatusWidget::HandleRestartClicked()
{
	if (BoundGameMode)
	{
		if (BoundGameMode->GetRunState() == ERLRunState::TutorialCompleted)
		{
			BoundGameMode->StartMainGame();
		}
		else
		{
			BoundGameMode->RestartRun();
		}
	}
}

void URLRunStatusWidget::HandleMainMenuClicked()
{
	if (BoundGameMode)
	{
		BoundGameMode->ReturnToMainMenu();
	}
}

void URLRunStatusWidget::HandleRewardChoiceOneClicked()
{
	if (BoundGameMode)
	{
		BoundGameMode->SelectReward(0);
	}
}

void URLRunStatusWidget::HandleRewardChoiceTwoClicked()
{
	if (BoundGameMode)
	{
		BoundGameMode->SelectReward(1);
	}
}

void URLRunStatusWidget::HandleRewardChoiceThreeClicked()
{
	if (BoundGameMode)
	{
		BoundGameMode->SelectReward(2);
	}
}

void URLRunStatusWidget::UpdateResultsPanel()
{
	if (!BoundGameMode || !ResultsContainer || !ResultsText || !MainMenuButton)
	{
		return;
	}

	const ERLRunState RunState = BoundGameMode->GetRunState();
	const bool bShowingResults =
		RunState == ERLRunState::TutorialCompleted ||
		RunState == ERLRunState::RunCompleted || RunState == ERLRunState::GameOver;
	ResultsContainer->SetVisibility(
		bShowingResults ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (!bShowingResults)
	{
		return;
	}

	if (RunState == ERLRunState::TutorialCompleted)
	{
		ResultsText->SetText(FText::FromString(TEXT(
			"TUTORIAL COMPLETE\nStart the main run or return to the main menu.")));
		if (RestartButtonLabel)
		{
			RestartButtonLabel->SetText(FText::FromString(TEXT("START GAME")));
		}
	}
	else
	{
		ResultsText->SetText(FText::FromString(FString::Printf(
			TEXT("ROUNDS  %d / %d\nBEST COMBO  %d"),
			BoundGameMode->GetRoundsCleared(),
			BoundGameMode->GetRoundCount(),
			BoundGameMode->GetBestParryCombo())));
		if (RestartButtonLabel)
		{
			RestartButtonLabel->SetText(FText::FromString(TEXT("RETRY")));
		}
	}
	MainMenuButton->SetIsEnabled(BoundGameMode->CanReturnToMainMenu());
}

void URLRunStatusWidget::UpdateRewardChoices()
{
	if (!RewardContainer || !BoundGameMode)
	{
		return;
	}

	const bool bChoosingReward = BoundGameMode->IsChoosingReward();
	if (StatusContainer)
	{
		StatusContainer->SetVisibility(
			bChoosingReward ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (RewardPromptText)
	{
		RewardPromptText->SetVisibility(
			bChoosingReward ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (ARLPlayerController* PlayerController = Cast<ARLPlayerController>(GetOwningPlayer()))
	{
		PlayerController->SetGameplayHUDVisible(!bChoosingReward);
	}
	if (RewardBackdrop)
	{
		RewardBackdrop->SetVisibility(
			bChoosingReward ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	RewardContainer->SetVisibility(
		bChoosingReward ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	for (int32 ChoiceIndex = 0; ChoiceIndex < RewardButtons.Num(); ++ChoiceIndex)
	{
		const bool bValidChoice = bChoosingReward &&
			ChoiceIndex < BoundGameMode->GetRewardChoiceCount();
		if (RewardButtons[ChoiceIndex])
		{
			RewardButtons[ChoiceIndex]->SetIsEnabled(bValidChoice);
		}
		if (RewardTitleTexts.IsValidIndex(ChoiceIndex) && RewardTitleTexts[ChoiceIndex])
		{
			RewardTitleTexts[ChoiceIndex]->SetText(
				bValidChoice ? BoundGameMode->GetRewardChoiceTitle(ChoiceIndex) : FText::GetEmpty());
		}
		if (RewardDescriptionTexts.IsValidIndex(ChoiceIndex) && RewardDescriptionTexts[ChoiceIndex])
		{
			RewardDescriptionTexts[ChoiceIndex]->SetText(
				bValidChoice ? BoundGameMode->GetRewardChoiceDescription(ChoiceIndex) : FText::GetEmpty());
		}
		const TOptional<ERLRunRewardType> RewardType = bValidChoice
			? BoundGameMode->GetRewardChoiceType(ChoiceIndex)
			: TOptional<ERLRunRewardType>();
		const FLinearColor CardAccent = RewardType.IsSet()
			? GetRewardAccentColor(RewardType.GetValue())
			: FLinearColor(0.2f, 0.9f, 1.0f);
		if (RewardArtPanels.IsValidIndex(ChoiceIndex) && RewardArtPanels[ChoiceIndex])
		{
			RewardArtPanels[ChoiceIndex]->SetBrushColor(
				FLinearColor(CardAccent.R * 0.12f, CardAccent.G * 0.12f, CardAccent.B * 0.12f, 1.0f));
		}
		if (RewardTitleTexts.IsValidIndex(ChoiceIndex) && RewardTitleTexts[ChoiceIndex])
		{
			RewardTitleTexts[ChoiceIndex]->SetColorAndOpacity(CardAccent);
		}
		if (RewardArtImages.IsValidIndex(ChoiceIndex) && RewardArtImages[ChoiceIndex])
		{
			UTexture2D* ArtTexture = nullptr;
			if (RewardType.IsSet())
			{
				const int32 RewardIndex = static_cast<int32>(RewardType.GetValue());
				if (RewardArtTextures.IsValidIndex(RewardIndex))
				{
					ArtTexture = RewardArtTextures[RewardIndex];
				}
			}
			RewardArtImages[ChoiceIndex]->SetBrushFromTexture(ArtTexture, true);
			RewardArtImages[ChoiceIndex]->SetVisibility(ArtTexture
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
		}
	}
}

void URLRunStatusWidget::UpdateInputMode()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController || !BoundGameMode)
	{
		return;
	}

	const ERLRunState RunState = BoundGameMode->GetRunState();
	if (RunState == ERLRunState::RewardSelection)
	{
		FInputModeGameAndUI InputMode;
		if (RewardButtons.IsValidIndex(0))
		{
			InputMode.SetWidgetToFocus(RewardButtons[0]->GetCachedWidget());
		}
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
	}
	else if (RunState == ERLRunState::TutorialCompleted ||
		RunState == ERLRunState::RunCompleted || RunState == ERLRunState::GameOver)
	{
		FInputModeUIOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
	}
	else
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
	}
}

void URLRunStatusWidget::UpdateTimerText()
{
	if (!BoundGameMode || !TimerText)
	{
		return;
	}

	const ERLRunState RunState = BoundGameMode->GetRunState();
	if (RunState != ERLRunState::PlayingRound)
	{
		TimerText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	TimerText->SetVisibility(ESlateVisibility::HitTestInvisible);
	TimerText->SetText(FText::FromString(FString::Printf(
		TEXT("ENEMIES  %d"),
		BoundGameMode->GetRemainingEnemyCount())));
}

void URLRunStatusWidget::UpdateWaveText()
{
	if (!BoundGameMode || !PhaseText || !NextPhaseText)
	{
		return;
	}
	if (BoundGameMode->GetRunState() != ERLRunState::PlayingRound)
	{
		PhaseText->SetText(FText::GetEmpty());
		NextPhaseText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const int32 WaveIndex = BoundGameMode->GetCurrentWaveIndex();
	const int32 WaveCount = BoundGameMode->GetCurrentWaveCount();
	const FName WaveName = BoundGameMode->GetCurrentWaveName();
	if (WaveIndex == INDEX_NONE || WaveCount <= 0)
	{
		PhaseText->SetText(FText::GetEmpty());
		NextPhaseText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	PhaseText->SetText(FText::FromString(FString::Printf(
		TEXT("WAVE %d / %d  %s"),
		WaveIndex + 1,
		WaveCount,
		*WaveName.ToString())));
	NextPhaseText->SetVisibility(ESlateVisibility::Collapsed);
}

void URLRunStatusWidget::UpdateStateText()
{
	if (!BoundGameMode || !StateText)
	{
		return;
	}

	switch (BoundGameMode->GetRunState())
	{
	case ERLRunState::Waiting:
		SetStateMessage(
			FText::FromString(TEXT("READY")),
			FLinearColor(1.0f, 0.75f, 0.15f),
			48);
		break;
	case ERLRunState::Countdown:
	{
		const int32 CountdownNumber = FMath::Max(
			1,
			FMath::CeilToInt(BoundGameMode->GetCountdownRemainingSeconds()));
		SetStateMessage(
			FText::AsNumber(CountdownNumber),
			FLinearColor(1.0f, 0.78f, 0.08f),
			96);
		break;
	}
	case ERLRunState::PlayingRound:
		if (StartMessageRemaining > 0.0f)
		{
			SetStateMessage(
				FText::FromString(TEXT("START")),
				FLinearColor(0.1f, 1.0f, 0.45f),
				72);
		}
		else
		{
			StateText->SetVisibility(ESlateVisibility::Collapsed);
		}
		break;
	case ERLRunState::RewardSelection:
		StateText->SetVisibility(ESlateVisibility::Collapsed);
		break;
	case ERLRunState::Intermission:
		if (RoundClearMessageRemaining > 0.0f)
		{
			SetStateMessage(
				FText::FromString(BoundGameMode->IsCurrentRoundTutorial()
					? TEXT("TUTORIAL COMPLETE")
					: TEXT("ROUND CLEAR")),
				FLinearColor(0.1f, 0.9f, 1.0f),
				72);
		}
		else
		{
			SetStateMessage(
				FText::FromString(FString::Printf(
					TEXT("NEXT ROUND IN %d"),
					FMath::CeilToInt(BoundGameMode->GetIntermissionRemainingSeconds()))),
				FLinearColor(1.0f, 0.75f, 0.15f),
				48);
		}
		break;
	case ERLRunState::TutorialCompleted:
		SetStateMessage(
			FText::FromString(TEXT("TUTORIAL COMPLETE")),
			FLinearColor(0.1f, 0.9f, 1.0f),
			64);
		break;
	case ERLRunState::RunCompleted:
		SetStateMessage(
			FText::FromString(
				RoundClearMessageRemaining > 0.0f
					? TEXT("ROUND CLEAR")
					: TEXT("RUN COMPLETE")),
			FLinearColor(0.1f, 0.9f, 1.0f),
			RoundClearMessageRemaining > 0.0f ? 72 : 56);
		break;
	case ERLRunState::GameOver:
		SetStateMessage(
			FText::FromString(TEXT("FAILED")),
			FLinearColor(1.0f, 0.08f, 0.03f),
			72);
		break;
	default:
		StateText->SetVisibility(ESlateVisibility::Collapsed);
		break;
	}
}

void URLRunStatusWidget::SetStateMessage(
	const FText& Message,
	const FLinearColor& Color,
	int32 FontSize)
{
	if (!StateText)
	{
		return;
	}

	StateText->SetText(Message);
	StateText->SetColorAndOpacity(Color);
	FSlateFontInfo StateFont = StateText->GetFont();
	StateFont.Size = FontSize;
	StateText->SetFont(StateFont);
	StateText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void URLRunStatusWidget::UpdateExplosiveWarning(float DeltaTime)
{
	if (!ExplosiveWarningText)
	{
		return;
	}

	ExplosiveWarningUpdateAccumulator += FMath::Max(0.0f, DeltaTime);
	if (ExplosiveWarningUpdateAccumulator < ExplosiveWarningUpdateInterval)
	{
		return;
	}
	ExplosiveWarningUpdateAccumulator = 0.0f;

	const APawn* PlayerPawn = GetOwningPlayerPawn();
	UWorld* World = GetWorld();
	if (!PlayerPawn || !World ||
		!BoundGameMode || BoundGameMode->GetRunState() != ERLRunState::PlayingRound)
	{
		ExplosiveWarningText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const ARLProjectile* MostUrgentProjectile = nullptr;
	float MostUrgentFuse = TNumericLimits<float>::Max();
	float MostUrgentDistance = TNumericLimits<float>::Max();
	for (TActorIterator<ARLProjectile> Iterator(World); Iterator; ++Iterator)
	{
		const ARLProjectile* Projectile = *Iterator;
		if (!Projectile->IsPoolActive() || !Projectile->IsExplosive())
		{
			continue;
		}

		const FVector ToPlayer = PlayerPawn->GetActorLocation() - Projectile->GetActorLocation();
		const float Distance = ToPlayer.Size();
		if (Distance > ExplosiveWarningDistance)
		{
			continue;
		}

		const FVector Velocity = Projectile->GetVelocity();
		const bool bInsideBlastRadius = Distance <= Projectile->GetExplosionRadius();
		const bool bApproachingPlayer = !Velocity.IsNearlyZero() &&
			FVector::DotProduct(Velocity.GetSafeNormal(), ToPlayer.GetSafeNormal()) > 0.25f;
		if (!bInsideBlastRadius && !bApproachingPlayer)
		{
			continue;
		}

		const float FuseRemaining = Projectile->GetExplosiveFuseRemainingSeconds();
		if (FuseRemaining < MostUrgentFuse ||
			(FMath::IsNearlyEqual(FuseRemaining, MostUrgentFuse) &&
				Distance < MostUrgentDistance))
		{
			MostUrgentProjectile = Projectile;
			MostUrgentFuse = FuseRemaining;
			MostUrgentDistance = Distance;
		}
	}

	if (!MostUrgentProjectile)
	{
		ExplosiveWarningText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const bool bInsideBlastRadius =
		MostUrgentDistance <= MostUrgentProjectile->GetExplosionRadius();
	if (bInsideBlastRadius)
	{
		ExplosiveWarningText->SetText(FText::FromString(FString::Printf(
			TEXT("BLAST DANGER  %.1fs"),
			MostUrgentFuse)));
	}
	else
	{
		ExplosiveWarningText->SetText(FText::FromString(FString::Printf(
			TEXT("EXPLOSIVE INCOMING  %.1fs  %.1fm"),
			MostUrgentFuse,
			MostUrgentDistance / 100.0f)));
	}
	const float Pulse = 0.72f +
		0.28f * FMath::Sin(World->GetTimeSeconds() * 12.0f);
	ExplosiveWarningText->SetRenderOpacity(Pulse);
	ExplosiveWarningText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

FText URLRunStatusWidget::FormatTime(float Seconds)
{
	const int32 TotalSeconds = FMath::Max(0, FMath::CeilToInt(Seconds));
	return FText::FromString(FString::Printf(
		TEXT("%02d:%02d"),
		TotalSeconds / 60,
		TotalSeconds % 60));
}
