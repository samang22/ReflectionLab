#include "UI/RLRewardSelectionWidget.h"
#include "Framework/GameMode/RLGameModeBase.h"
#include "Framework/PlayerController/RLPlayerController.h"
#include "Data/RLRunRewardDataAsset.h"
#include "Blueprint/WidgetTree.h"
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
#include "Engine/Texture2D.h"

void URLRewardSelectionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildWidgetTree();
}
void URLRewardSelectionWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	UpdateRewardEntrance(DeltaTime);
	UpdateRewardSelectionAnimation(DeltaTime);
}
void URLRewardSelectionWidget::BindToGameMode(ARLGameModeBase* GameMode)
{
	BoundGameMode = GameMode;
	bRewardEntranceVisible = false;
	SelectedRewardIndex = INDEX_NONE;
	bSelectionSubmitted = false;
	Refresh();
}
void URLRewardSelectionWidget::Refresh()
{
	if (!BoundGameMode)
	{
		SelectedRewardIndex = INDEX_NONE;
		bRewardEntranceVisible = false;
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	SetVisibility(BoundGameMode->IsChoosingReward() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	UpdateRewardChoices();
}
UWidget* URLRewardSelectionWidget::GetDefaultFocusTarget() const
{
	return RewardButtons.IsValidIndex(0) ? RewardButtons[0].Get() : nullptr;
}
void URLRewardSelectionWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget) { return; }
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RewardCanvas"));
	WidgetTree->RootWidget = RootCanvas;
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
	RewardPromptText->SetText(NSLOCTEXT("ReflectionLab", "CalibrationReward", "CALIBRATION COMPLETE\nSELECT ONE CORRECTION MODULE"));
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
		RewardCardBorders.Add(CardBorder);
		RewardTitleTexts.Add(TitleText);
		RewardDescriptionTexts.Add(DescriptionText);
		RewardArtPanels.Add(ArtPanel);
		RewardArtImages.Add(ArtImage);
	}
}

void URLRewardSelectionWidget::UpdateRewardEntrance(float DeltaSeconds)
{
	if (!bRewardEntranceVisible || SelectedRewardIndex != INDEX_NONE)
	{
		return;
	}
	RewardEntranceElapsed = FMath::Min(0.5f,
		RewardEntranceElapsed + FMath::Max(0.0f, DeltaSeconds) * 0.5f);
	if (RewardBackdrop)
	{
		RewardBackdrop->SetRenderOpacity(FMath::Clamp(RewardEntranceElapsed / 0.15f, 0.0f, 1.0f));
	}
	for (int32 Index = 0; Index < RewardButtons.Num(); ++Index)
	{
		if (UButton* Button = RewardButtons[Index])
		{
			const float Progress = FMath::Clamp(
				(RewardEntranceElapsed - 0.05f - Index * 0.06f) / 0.23f, 0.0f, 1.0f);
			const float Eased = 1.0f - FMath::Pow(1.0f - Progress, 3.0f);
			Button->SetRenderOpacity(Eased);
			Button->SetRenderTranslation(FVector2D(0.0f, 25.0f * (1.0f - Eased)));
			Button->SetRenderScale(FVector2D(FMath::Lerp(0.96f, 1.0f, Eased)));
			Button->SetVisibility(Progress > 0.0f
				? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
		}
	}
}

void URLRewardSelectionWidget::HandleRewardChoiceOneClicked()
{
	BeginRewardSelectionAnimation(0);
}

void URLRewardSelectionWidget::HandleRewardChoiceTwoClicked()
{
	BeginRewardSelectionAnimation(1);
}

void URLRewardSelectionWidget::HandleRewardChoiceThreeClicked()
{
	BeginRewardSelectionAnimation(2);
}

void URLRewardSelectionWidget::BeginRewardSelectionAnimation(int32 ChoiceIndex)
{
	if (BoundGameMode && BoundGameMode->IsCurrentRoundTutorial())
	{
		const ARLPlayerController* Controller = Cast<ARLPlayerController>(GetOwningPlayer());
		if (Controller && !Controller->IsTutorialRewardChoiceAllowed()) { return; }
	}
	if (!IsValid(BoundGameMode) || !BoundGameMode->IsChoosingReward() ||
		SelectedRewardIndex != INDEX_NONE || ChoiceIndex < 0 ||
		ChoiceIndex >= BoundGameMode->GetRewardChoiceCount())
	{
		return;
	}
	SelectedRewardIndex = ChoiceIndex;
	RewardSelectionElapsed = 0.0f;
	for (UButton* Button : RewardButtons)
	{
		if (Button)
		{
			Button->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	UpdateRewardSelectionAnimation(0.0f);
}

void URLRewardSelectionWidget::UpdateRewardSelectionAnimation(float DeltaSeconds)
{
	if (SelectedRewardIndex == INDEX_NONE)
	{
		return;
	}
	if (!IsValid(BoundGameMode) || !BoundGameMode->IsChoosingReward())
	{
		SelectedRewardIndex = INDEX_NONE;
		return;
	}
	RewardSelectionElapsed += FMath::Max(0.0f, DeltaSeconds);
	const float Fade = FMath::Clamp((RewardSelectionElapsed - 0.3f) / 0.3f, 0.0f, 1.0f);
	for (int32 Index = 0; Index < RewardButtons.Num(); ++Index)
	{
		if (UButton* Button = RewardButtons[Index])
		{
			const bool bSelected = Index == SelectedRewardIndex;
			Button->SetRenderOpacity(bSelected ? 1.0f - Fade
				: 1.0f - FMath::Clamp(RewardSelectionElapsed / 0.2f, 0.0f, 1.0f));
			const float Scale = bSelected ? FMath::Lerp(1.0f, 1.04f,
				FMath::Clamp(RewardSelectionElapsed / 0.15f, 0.0f, 1.0f)) : 1.0f;
			Button->SetRenderScale(FVector2D(Scale));
			Button->SetRenderTranslation(FVector2D(0.0f, bSelected ? -25.0f * Fade : 0.0f));
		}
		if (Index == SelectedRewardIndex && RewardCardBorders.IsValidIndex(Index))
		{
			RewardCardBorders[Index]->SetBrushColor(FLinearColor(0.08f, 0.28f, 0.38f, 1.0f));
		}
	}
	if (RewardBackdrop)
	{
		RewardBackdrop->SetRenderOpacity(1.0f - Fade);
	}
	if (RewardPromptText)
	{
		RewardPromptText->SetRenderOpacity(1.0f - Fade);
	}
	if (RewardSelectionElapsed >= 0.6f && !bSelectionSubmitted)
	{
		// State changes synchronously; keep the input guard set until the screen closes.
		bSelectionSubmitted = true;
		OnSelectionFinished.Broadcast(SelectedRewardIndex);
	}
}

void URLRewardSelectionWidget::UpdateRewardChoices()
{
	if (!RewardContainer || !BoundGameMode)
	{
		return;
	}

	const bool bChoosingReward = BoundGameMode->IsChoosingReward();
	if (bChoosingReward != bRewardEntranceVisible)
	{
		bSelectionSubmitted = false;
		bRewardEntranceVisible = bChoosingReward;
		RewardEntranceElapsed = 0.0f;
		SelectedRewardIndex = INDEX_NONE;
		RewardSelectionElapsed = 0.0f;
		if (RewardPromptText)
		{
			RewardPromptText->SetRenderOpacity(1.0f);
		}
		for (UBorder* Border : RewardCardBorders)
		{
			if (Border)
			{
				Border->SetBrushColor(FLinearColor(0.025f, 0.06f, 0.12f, 0.98f));
			}
		}
	}
	if (RewardPromptText)
	{
		RewardPromptText->SetVisibility(
			bChoosingReward ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
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
			RewardButtons[ChoiceIndex]->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
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
		const URLRunRewardDataAsset* Reward = bValidChoice
			? BoundGameMode->GetRewardChoiceDefinition(ChoiceIndex) : nullptr;
		const FLinearColor CardAccent = Reward
			? Reward->AccentColor
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
			UTexture2D* ArtTexture = Reward ? Reward->Illustration.Get() : nullptr;
			RewardArtImages[ChoiceIndex]->SetBrushFromTexture(ArtTexture, true);
			RewardArtImages[ChoiceIndex]->SetVisibility(ArtTexture
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
		}
	}
	UpdateRewardEntrance(0.0f);
}