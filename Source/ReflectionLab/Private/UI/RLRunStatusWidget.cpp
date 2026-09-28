#include "UI/RLRunStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Combat/RLProjectile.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

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
	UpdatePhaseText();
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
		BoundGameMode->OnDifficultyPhaseChanged.RemoveDynamic(
			this,
			&ThisClass::HandleDifficultyPhaseChanged);
	}

	BoundGameMode = GameMode;
	if (BoundGameMode)
	{
		BoundGameMode->OnRunStateChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandleRunStateChanged);
		BoundGameMode->OnDifficultyPhaseChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandleDifficultyPhaseChanged);
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

void URLRunStatusWidget::HandleDifficultyPhaseChanged(
	int32 RoundIndex,
	int32 PhaseIndex,
	FName PhaseName)
{
	(void)RoundIndex;
	(void)PhaseIndex;
	(void)PhaseName;
	if (PhaseText)
	{
		UpdatePhaseText();
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

	UVerticalBox* StatusContainer = WidgetTree->ConstructWidget<UVerticalBox>(
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
	UTextBlock* RestartLabel = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("RestartLabel"));
	RestartLabel->SetText(FText::FromString(TEXT("RETRY")));
	RestartLabel->SetJustification(ETextJustify::Center);
	RestartButton->AddChild(RestartLabel);
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
		RunState == ERLRunState::RunCompleted || RunState == ERLRunState::GameOver;
	SetVisibility(bShowingResults
		? ESlateVisibility::Visible
		: ESlateVisibility::HitTestInvisible);
	const int32 RoundIndex = BoundGameMode->GetCurrentRoundIndex();
	const int32 RoundCount = BoundGameMode->GetRoundCount();
	if (RoundText)
	{
		RoundText->SetText(FText::FromString(FString::Printf(
			TEXT("ROUND %d / %d"),
			FMath::Max(0, RoundIndex) + 1,
			FMath::Max(1, RoundCount))));
	}

	if (PhaseText)
	{
		UpdatePhaseText();
	}

	UpdateTimerText();
	UpdateStateText();
	UpdateResultsPanel();
}

void URLRunStatusWidget::HandleRestartClicked()
{
	if (BoundGameMode)
	{
		BoundGameMode->RestartRun();
	}
}

void URLRunStatusWidget::HandleMainMenuClicked()
{
	if (BoundGameMode)
	{
		BoundGameMode->ReturnToMainMenu();
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
		RunState == ERLRunState::RunCompleted || RunState == ERLRunState::GameOver;
	ResultsContainer->SetVisibility(
		bShowingResults ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (!bShowingResults)
	{
		return;
	}

	ResultsText->SetText(FText::FromString(FString::Printf(
		TEXT("ROUNDS  %d / %d\nTIME  %s\nBEST COMBO  %d"),
		BoundGameMode->GetRoundsCleared(),
		BoundGameMode->GetRoundCount(),
		*FormatTime(BoundGameMode->GetTotalRunElapsedSeconds()).ToString(),
		BoundGameMode->GetBestParryCombo())));
	MainMenuButton->SetIsEnabled(BoundGameMode->CanReturnToMainMenu());
}

void URLRunStatusWidget::UpdateInputMode()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController || !BoundGameMode)
	{
		return;
	}

	const ERLRunState RunState = BoundGameMode->GetRunState();
	if (RunState == ERLRunState::RunCompleted || RunState == ERLRunState::GameOver)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
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
	if (RunState == ERLRunState::Countdown ||
		RunState == ERLRunState::Waiting ||
		RunState == ERLRunState::RunCompleted ||
		RunState == ERLRunState::GameOver)
	{
		TimerText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	TimerText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (RunState == ERLRunState::Intermission)
	{
		const float Remaining = BoundGameMode->GetIntermissionRemainingSeconds();
		TimerText->SetText(FormatTime(Remaining));
		return;
	}

	TimerText->SetText(FormatTime(BoundGameMode->GetRoundRemainingSeconds()));
}

void URLRunStatusWidget::UpdatePhaseText()
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

	const int32 PhaseIndex = BoundGameMode->GetCurrentPhaseIndex();
	const int32 PhaseCount = BoundGameMode->GetCurrentPhaseCount();
	const FName PhaseName = BoundGameMode->GetCurrentPhaseName();
	if (PhaseIndex == INDEX_NONE || PhaseCount <= 0)
	{
		PhaseText->SetText(FText::GetEmpty());
		NextPhaseText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	PhaseText->SetText(FText::FromString(FString::Printf(
		TEXT("PHASE %d / %d  %s"),
		PhaseIndex + 1,
		PhaseCount,
		*PhaseName.ToString())));

	const float NextPhaseRemaining = BoundGameMode->GetNextPhaseRemainingSeconds();
	if (NextPhaseRemaining < 0.0f)
	{
		NextPhaseText->SetText(FText::FromString(TEXT("FINAL PHASE")));
	}
	else
	{
		NextPhaseText->SetText(FText::FromString(FString::Printf(
			TEXT("NEXT PHASE IN %s"),
			*FormatTime(NextPhaseRemaining).ToString())));
	}
	NextPhaseText->SetVisibility(ESlateVisibility::HitTestInvisible);
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
	case ERLRunState::Intermission:
		if (RoundClearMessageRemaining > 0.0f)
		{
			SetStateMessage(
				FText::FromString(TEXT("ROUND CLEAR")),
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
