#include "UI/RLPlayerHealthWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Engine/World.h"
#include "Player/RLPlayerCharacter.h"

void URLPlayerHealthWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildWidgetTree();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void URLPlayerHealthWidget::NativeDestruct()
{
	BindToPlayer(nullptr);
	Super::NativeDestruct();
}

void URLPlayerHealthWidget::NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (FeedbackTimeRemaining > 0.0f)
	{
		FeedbackTimeRemaining = FMath::Max(
			0.0f,
			FeedbackTimeRemaining - InDeltaTime);
	}
	UpdateDamageFragments(InDeltaTime);
	RefreshHealthSegments();
}

void URLPlayerHealthWidget::BindToPlayer(ARLPlayerCharacter* PlayerCharacter)
{
	if (BoundPlayerCharacter)
	{
		BoundPlayerCharacter->OnHealthChanged.RemoveDynamic(
			this,
			&ThisClass::HandleHealthChanged);
	}

	BoundPlayerCharacter = PlayerCharacter;
	FeedbackTimeRemaining = 0.0f;
	FeedbackSegmentIndex = INDEX_NONE;

	if (!BoundPlayerCharacter)
	{
		DisplayedCurrentHealth = 0.0f;
		DisplayedMaxHealth = 0.0f;
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	BoundPlayerCharacter->OnHealthChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleHealthChanged);
	DisplayedCurrentHealth = BoundPlayerCharacter->GetCurrentHealth();
	DisplayedMaxHealth = BoundPlayerCharacter->GetMaxHealth();
	RebuildHealthSegments();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void URLPlayerHealthWidget::HandleHealthChanged(
	float CurrentHealth,
	float MaxHealth)
{
	const float PreviousHealth = DisplayedCurrentHealth;
	const int32 PreviousSegmentCount = FMath::CeilToInt(DisplayedMaxHealth);
	DisplayedCurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);
	DisplayedMaxHealth = FMath::Max(0.0f, MaxHealth);

	if (PreviousSegmentCount != FMath::CeilToInt(DisplayedMaxHealth))
	{
		RebuildHealthSegments();
	}

	if (!FMath::IsNearlyEqual(PreviousHealth, DisplayedCurrentHealth))
	{
		bRecoveryFeedback = DisplayedCurrentHealth > PreviousHealth;
		const int32 PreviousFilledSegmentCount = FMath::CeilToInt(PreviousHealth);
		const int32 FilledSegmentCount = FMath::CeilToInt(DisplayedCurrentHealth);
		FeedbackSegmentIndex = bRecoveryFeedback
			? FilledSegmentCount - 1
			: FilledSegmentCount;
		FeedbackSegmentIndex = FMath::Clamp(
			FeedbackSegmentIndex,
			0,
			FMath::Max(0, HealthSegments.Num() - 1));
		FeedbackTimeRemaining = FeedbackDuration;

		if (!bRecoveryFeedback)
		{
			for (int32 SegmentIndex = FilledSegmentCount;
				SegmentIndex < PreviousFilledSegmentCount;
				++SegmentIndex)
			{
				SpawnDamageFragments(SegmentIndex);
			}
		}
	}

	RefreshHealthSegments();
}

void URLPlayerHealthWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	HealthContainer = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("HealthContainer"));
	UCanvasPanelSlot* HealthSlot = RootCanvas->AddChildToCanvas(HealthContainer);
	HealthSlot->SetAnchors(FAnchors(0.035f, 0.9f));
	HealthSlot->SetAlignment(FVector2D(0.0f, 1.0f));
	HealthSlot->SetAutoSize(true);

	HealthLabel = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("HealthLabel"));
	HealthLabel->SetText(FText::FromString(TEXT("HP")));
	HealthLabel->SetColorAndOpacity(FilledColor);
	HealthLabel->SetShadowOffset(FVector2D(3.0f, 3.0f));
	HealthLabel->SetShadowColorAndOpacity(
		FilledColor.CopyWithNewOpacity(0.8f));
	FSlateFontInfo LabelFont = HealthLabel->GetFont();
	LabelFont.Size = 30;
	LabelFont.OutlineSettings.OutlineSize = 2;
	LabelFont.OutlineSettings.OutlineColor = FilledColor.CopyWithNewOpacity(0.55f);
	HealthLabel->SetFont(LabelFont);
	USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("HealthLabelBox"));
	LabelBox->SetHeightOverride(SegmentDisplaySize + 8.0f);
	LabelBox->AddChild(HealthLabel);
	HealthLabel->SetJustification(ETextJustify::Center);
	// Use the same fixed cell height for the label and the diamonds.
	Cast<USizeBoxSlot>(HealthLabel->Slot)->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* LabelSlot = HealthContainer->AddChildToHorizontalBox(LabelBox);
	LabelSlot->SetVerticalAlignment(VAlign_Bottom);
	LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));

	HealthSegmentsContainer = WidgetTree->ConstructWidget<UUniformGridPanel>(
		UUniformGridPanel::StaticClass(),
		TEXT("HealthSegmentsContainer"));
	HealthSegmentsContainer->SetMinDesiredSlotWidth(SegmentDisplaySize + 8.0f);
	HealthSegmentsContainer->SetMinDesiredSlotHeight(SegmentDisplaySize + 8.0f);
	UHorizontalBoxSlot* SegmentsSlot =
		HealthContainer->AddChildToHorizontalBox(HealthSegmentsContainer);
	SegmentsSlot->SetVerticalAlignment(VAlign_Bottom);
}

void URLPlayerHealthWidget::RebuildHealthSegments()
{
	if (!HealthSegmentsContainer)
	{
		return;
	}

	for (UTextBlock* HealthSegment : HealthSegments)
	{
		HealthSegmentsContainer->RemoveChild(HealthSegment);
	}
	HealthSegments.Reset();

	const int32 SegmentCount = FMath::Max(1, FMath::CeilToInt(DisplayedMaxHealth));
	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		UTextBlock* HealthSegment = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("HealthSegment_%d"), SegmentIndex));
		HealthSegment->SetText(FText::FromString(TEXT("\u25C6")));
		// The diamond glyph sits above the visual center of its font layout box.
		HealthSegment->SetRenderTranslation(FVector2D(0.0f, SegmentVerticalOffset));
		HealthSegment->SetShadowOffset(FVector2D(3.0f, 3.0f));
		HealthSegment->SetShadowColorAndOpacity(FilledColor.CopyWithNewOpacity(0.8f));
		FSlateFontInfo SegmentFont = HealthSegment->GetFont();
		SegmentFont.Size = FMath::RoundToInt(SegmentDisplaySize);
		SegmentFont.OutlineSettings.OutlineSize = 3;
		SegmentFont.OutlineSettings.OutlineColor = FilledColor.CopyWithNewOpacity(0.55f);
		HealthSegment->SetFont(SegmentFont);
		UUniformGridSlot* SegmentSlot = HealthSegmentsContainer->AddChildToUniformGrid(
			HealthSegment, SegmentIndex / SegmentsPerRow, SegmentIndex % SegmentsPerRow);
		SegmentSlot->SetHorizontalAlignment(HAlign_Center);
		SegmentSlot->SetVerticalAlignment(VAlign_Center);
		HealthSegments.Add(HealthSegment);
	}

	RefreshHealthSegments();
}

void URLPlayerHealthWidget::RefreshHealthSegments()
{
	const int32 FilledSegmentCount = FMath::Clamp(
		FMath::CeilToInt(DisplayedCurrentHealth),
		0,
		HealthSegments.Num());
	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	for (int32 SegmentIndex = 0; SegmentIndex < HealthSegments.Num(); ++SegmentIndex)
	{
		UTextBlock* HealthSegment = HealthSegments[SegmentIndex];
		if (!HealthSegment)
		{
			continue;
		}

		FLinearColor SegmentColor = SegmentIndex < FilledSegmentCount
			? FilledColor
			: EmptyColor;
		if (FeedbackTimeRemaining > 0.0f && SegmentIndex == FeedbackSegmentIndex)
		{
			const float FlashAlpha = 0.55f + 0.45f *
				FMath::Sin(WorldTime * 45.0f);
			SegmentColor = FLinearColor::LerpUsingHSV(
				SegmentColor,
				bRecoveryFeedback ? RecoveryColor : DamageColor,
				FlashAlpha);
		}
		else if (FilledSegmentCount == 1 && SegmentIndex == 0)
		{
			const float PulseAlpha = 0.2f + 0.2f *
				(0.5f + 0.5f * FMath::Sin(WorldTime * 5.0f));
			SegmentColor = FLinearColor::LerpUsingHSV(
				FilledColor,
				DamageColor,
				PulseAlpha);
		}

		HealthSegment->SetColorAndOpacity(SegmentColor);
		HealthSegment->SetShadowColorAndOpacity(
			SegmentColor.CopyWithNewOpacity(0.8f));
		FSlateFontInfo SegmentFont = HealthSegment->GetFont();
		SegmentFont.OutlineSettings.OutlineColor =
			SegmentColor.CopyWithNewOpacity(0.55f);
		HealthSegment->SetFont(SegmentFont);
	}
}

void URLPlayerHealthWidget::SpawnDamageFragments(int32 SegmentIndex)
{
	if (!RootCanvas || SegmentIndex < 0)
	{
		return;
	}

	constexpr int32 SafeSegmentsPerRow = SegmentsPerRow;
	const int32 Column = SegmentIndex % SafeSegmentsPerRow;
	const int32 Row = SegmentIndex / SafeSegmentsPerRow;
	FVector2D SegmentOrigin(
		72.0f + Column * (SegmentDisplaySize + 8.0f),
		-28.0f - Row * (SegmentDisplaySize + 4.0f));
	if (HealthSegments.IsValidIndex(SegmentIndex) &&
		HealthSegments[SegmentIndex] &&
		RootCanvas)
	{
		const FGeometry& SegmentGeometry =
			HealthSegments[SegmentIndex]->GetCachedGeometry();
		const FVector2D SegmentSize = SegmentGeometry.GetLocalSize();
		if (!SegmentSize.IsNearlyZero())
		{
			const FVector2D AbsoluteSegmentCenter = SegmentGeometry.LocalToAbsolute(
				SegmentSize * 0.5f);
			SegmentOrigin = RootCanvas->GetCachedGeometry().AbsoluteToLocal(
				AbsoluteSegmentCenter);
		}
	}

	for (int32 FragmentIndex = 0; FragmentIndex < DamageFragmentCount; ++FragmentIndex)
	{
		UTextBlock* FragmentWidget = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(
				TEXT("HealthFragment_%d"),
				NextFragmentId++));
		FragmentWidget->SetText(FText::FromString(TEXT("\u25C6")));
		FragmentWidget->SetColorAndOpacity(DamageColor);
		FragmentWidget->SetShadowOffset(FVector2D(1.0f, 1.0f));
		FragmentWidget->SetShadowColorAndOpacity(
			DamageColor.CopyWithNewOpacity(0.85f));
		FSlateFontInfo FragmentFont = FragmentWidget->GetFont();
		FragmentFont.Size = FMath::RandRange(9, 16);
		FragmentFont.OutlineSettings.OutlineSize = 1;
		FragmentFont.OutlineSettings.OutlineColor =
			DamageColor.CopyWithNewOpacity(0.65f);
		FragmentWidget->SetFont(FragmentFont);

		UCanvasPanelSlot* FragmentSlot = RootCanvas->AddChildToCanvas(FragmentWidget);
		FragmentSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		FragmentSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		FragmentSlot->SetAutoSize(true);

		FHealthFragment& Fragment = ActiveFragments.AddDefaulted_GetRef();
		Fragment.Widget = FragmentWidget;
		Fragment.Position = SegmentOrigin + FVector2D(
			FMath::RandRange(-10.0f, 10.0f),
			FMath::RandRange(-8.0f, 8.0f));
		Fragment.Velocity = FVector2D(
			FMath::RandRange(-180.0f, 180.0f),
			FMath::RandRange(-280.0f, -90.0f));
		Fragment.Rotation = FMath::RandRange(0.0f, 360.0f);
		Fragment.RotationSpeed = FMath::RandRange(-480.0f, 480.0f);
		Fragment.MaxLifetime = DamageFragmentLifetime *
			FMath::RandRange(0.8f, 1.15f);
		Fragment.TimeRemaining = Fragment.MaxLifetime;
		FragmentSlot->SetPosition(Fragment.Position);
		FragmentWidget->SetRenderTransformAngle(Fragment.Rotation);
	}
}

void URLPlayerHealthWidget::UpdateDamageFragments(float DeltaTime)
{
	for (int32 FragmentIndex = ActiveFragments.Num() - 1;
		FragmentIndex >= 0;
		--FragmentIndex)
	{
		FHealthFragment& Fragment = ActiveFragments[FragmentIndex];
		UTextBlock* FragmentWidget = Fragment.Widget.Get();
		if (!FragmentWidget)
		{
			ActiveFragments.RemoveAtSwap(FragmentIndex);
			continue;
		}

		Fragment.TimeRemaining -= DeltaTime;
		if (Fragment.TimeRemaining <= 0.0f)
		{
			if (RootCanvas)
			{
				RootCanvas->RemoveChild(FragmentWidget);
			}
			ActiveFragments.RemoveAtSwap(FragmentIndex);
			continue;
		}

		Fragment.Velocity.Y += DamageFragmentGravity * DeltaTime;
		Fragment.Position += Fragment.Velocity * DeltaTime;
		Fragment.Rotation += Fragment.RotationSpeed * DeltaTime;
		if (UCanvasPanelSlot* FragmentSlot =
			Cast<UCanvasPanelSlot>(FragmentWidget->Slot))
		{
			FragmentSlot->SetPosition(Fragment.Position);
		}
		FragmentWidget->SetRenderTransformAngle(Fragment.Rotation);
		FragmentWidget->SetRenderOpacity(FMath::Clamp(
			Fragment.TimeRemaining / FMath::Max(KINDA_SMALL_NUMBER, Fragment.MaxLifetime),
			0.0f,
			1.0f));
	}
}
