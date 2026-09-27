#include "UI/RLParryComboWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Player/RLPlayerCharacter.h"

void URLParryComboWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildWidgetTree();
	ResetDisplay();
}

void URLParryComboWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (ComboBreakTimeRemaining > 0.0f)
	{
		ComboBreakTimeRemaining = FMath::Max(0.0f, ComboBreakTimeRemaining - InDeltaTime);
		if (ComboBreakTimeRemaining <= 0.0f)
		{
			ResetDisplay();
			return;
		}
	}

	PopTimeRemaining = FMath::Max(0.0f, PopTimeRemaining - InDeltaTime);
	const float PopAlpha = FMath::Clamp(PopTimeRemaining / 0.12f, 0.0f, 1.0f);
	SetRenderScale(FVector2D(FMath::Lerp(1.0f, 1.25f, PopAlpha)));
}

void URLParryComboWidget::BindToPlayer(ARLPlayerCharacter* PlayerCharacter)
{
	if (BoundPlayerCharacter)
	{
		BoundPlayerCharacter->OnParryComboChanged.RemoveDynamic(
			this,
			&ThisClass::HandleParryComboChanged);
	}

	BoundPlayerCharacter = PlayerCharacter;
	if (!BoundPlayerCharacter)
	{
		ResetDisplay();
		return;
	}

	BoundPlayerCharacter->OnParryComboChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleParryComboChanged);
}

void URLParryComboWidget::HandleParryComboChanged(
	int32 ComboCount,
	int32 MultiParryCount,
	bool bPerfectParry,
	bool bCloseRangeParry)
{
	if (ComboCount <= 0)
	{
		if (DisplayedComboCount <= 0)
		{
			ResetDisplay();
			return;
		}

		const bool bOverdriveConsumed = DisplayedComboCount >= 8;
		DisplayedComboCount = 0;
		ComboBreakTimeRemaining = bOverdriveConsumed ? 0.9f : 0.65f;
		PopTimeRemaining = 0.12f;
		SetVisibility(ESlateVisibility::HitTestInvisible);
		if (FeedbackText)
		{
			FeedbackText->SetText(FText::FromString(
				bOverdriveConsumed ? TEXT("OVERDRIVE!") : TEXT("COMBO BREAK")));
			FeedbackText->SetColorAndOpacity(
				bOverdriveConsumed
					? FLinearColor(1.0f, 0.65f, 0.05f)
					: FLinearColor(1.0f, 0.12f, 0.05f));
			FeedbackText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (ComboText)
		{
			ComboText->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (ComboProgressBar)
		{
			ComboProgressBar->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (MilestoneText)
		{
			MilestoneText->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	DisplayedComboCount = ComboCount;
	ComboBreakTimeRemaining = 0.0f;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	PopTimeRemaining = 0.12f;

	FString Feedback;
	if (bPerfectParry && bCloseRangeParry)
	{
		Feedback = TEXT("PERFECT CLOSE!");
	}
	else if (bPerfectParry)
	{
		Feedback = TEXT("PERFECT!");
	}
	else if (bCloseRangeParry)
	{
		Feedback = TEXT("CLOSE!");
	}

	if (MultiParryCount > 1)
	{
		if (!Feedback.IsEmpty())
		{
			Feedback += TEXT("  |  ");
		}
		Feedback += FString::Printf(TEXT("MULTI PARRY x%d"), MultiParryCount);
	}

	if (FeedbackText)
	{
		FeedbackText->SetColorAndOpacity(FLinearColor(0.3f, 0.9f, 1.0f));
		FeedbackText->SetText(FText::FromString(Feedback));
		FeedbackText->SetVisibility(
			Feedback.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (ComboText)
	{
		ComboText->SetVisibility(ESlateVisibility::HitTestInvisible);
		ComboText->SetText(FText::FromString(FString::Printf(TEXT("%d COMBO"), ComboCount)));
		const FLinearColor ComboColor = ComboCount >= 5
			? FLinearColor(1.0f, 0.65f, 0.05f)
			: ComboCount >= 3
				? FLinearColor(0.05f, 0.75f, 1.0f)
				: FLinearColor::White;
		ComboText->SetColorAndOpacity(ComboColor);
	}

	UpdateMilestoneProgress(ComboCount);
}

void URLParryComboWidget::UpdateMilestoneProgress(int32 ComboCount)
{
	int32 PreviousMilestone = 0;
	int32 NextMilestone = 3;
	if (ComboCount > 5)
	{
		PreviousMilestone = 5;
		NextMilestone = 8;
	}
	else if (ComboCount > 3)
	{
		PreviousMilestone = 3;
		NextMilestone = 5;
	}

	const bool bReachedFinalMilestone = ComboCount >= 8;
	const float Progress = bReachedFinalMilestone
		? 1.0f
		: static_cast<float>(ComboCount - PreviousMilestone) /
			static_cast<float>(NextMilestone - PreviousMilestone);
	if (ComboProgressBar)
	{
		ComboProgressBar->SetVisibility(ESlateVisibility::HitTestInvisible);
		ComboProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
	}

	if (MilestoneText)
	{
		MilestoneText->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (bReachedFinalMilestone)
		{
			MilestoneText->SetText(FText::FromString(TEXT("MAX STREAK")));
		}
		else if (ComboCount == NextMilestone)
		{
			MilestoneText->SetText(FText::FromString(TEXT("MILESTONE!")));
		}
		else
		{
			MilestoneText->SetText(FText::FromString(
				FString::Printf(TEXT("NEXT: %d"), NextMilestone)));
		}
	}
}

void URLParryComboWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UVerticalBox* ComboContainer = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("ComboContainer"));
	UCanvasPanelSlot* ContainerSlot = RootCanvas->AddChildToCanvas(ComboContainer);
	ContainerSlot->SetAnchors(FAnchors(0.5f, 0.12f));
	ContainerSlot->SetAlignment(FVector2D(0.5f, 0.0f));
	ContainerSlot->SetAutoSize(true);

	FeedbackText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("FeedbackText"));
	FeedbackText->SetJustification(ETextJustify::Center);
	FeedbackText->SetColorAndOpacity(FLinearColor(0.3f, 0.9f, 1.0f));
	FeedbackText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	FeedbackText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
	FSlateFontInfo FeedbackFont = FeedbackText->GetFont();
	FeedbackFont.Size = 20;
	FeedbackText->SetFont(FeedbackFont);
	UVerticalBoxSlot* FeedbackSlot = ComboContainer->AddChildToVerticalBox(FeedbackText);
	FeedbackSlot->SetHorizontalAlignment(HAlign_Center);

	ComboText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ComboText"));
	ComboText->SetJustification(ETextJustify::Center);
	ComboText->SetShadowOffset(FVector2D(3.0f, 3.0f));
	ComboText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	FSlateFontInfo ComboFont = ComboText->GetFont();
	ComboFont.Size = 42;
	ComboText->SetFont(ComboFont);
	UVerticalBoxSlot* ComboSlot = ComboContainer->AddChildToVerticalBox(ComboText);
	ComboSlot->SetHorizontalAlignment(HAlign_Center);

	USizeBox* ProgressSizeBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		TEXT("ProgressSizeBox"));
	ProgressSizeBox->SetWidthOverride(220.0f);
	ProgressSizeBox->SetHeightOverride(8.0f);
	ComboProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(),
		TEXT("ComboProgressBar"));
	ComboProgressBar->SetFillColorAndOpacity(FLinearColor(0.05f, 0.75f, 1.0f));
	ProgressSizeBox->AddChild(ComboProgressBar);
	UVerticalBoxSlot* ProgressSlot = ComboContainer->AddChildToVerticalBox(ProgressSizeBox);
	ProgressSlot->SetHorizontalAlignment(HAlign_Center);
	ProgressSlot->SetPadding(FMargin(0.0f, 5.0f, 0.0f, 0.0f));

	MilestoneText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("MilestoneText"));
	MilestoneText->SetJustification(ETextJustify::Center);
	MilestoneText->SetColorAndOpacity(FLinearColor(0.65f, 0.8f, 1.0f));
	FSlateFontInfo MilestoneFont = MilestoneText->GetFont();
	MilestoneFont.Size = 13;
	MilestoneText->SetFont(MilestoneFont);
	UVerticalBoxSlot* MilestoneSlot = ComboContainer->AddChildToVerticalBox(MilestoneText);
	MilestoneSlot->SetHorizontalAlignment(HAlign_Center);
	MilestoneSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));
}

void URLParryComboWidget::ResetDisplay()
{
	PopTimeRemaining = 0.0f;
	ComboBreakTimeRemaining = 0.0f;
	DisplayedComboCount = 0;
	SetRenderScale(FVector2D(1.0f));
	SetVisibility(ESlateVisibility::Collapsed);
	if (ComboProgressBar)
	{
		ComboProgressBar->SetPercent(0.0f);
	}
}
