#include "UI/RLBossHealthWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Enemies/RLRobotBossCharacter.h"
#include "Enemies/Components/RLBossOverloadComponent.h"
#include "Player/Components/RLHealthComponent.h"
#include "EngineUtils.h"

void URLBossHealthWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;
	auto* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel = Box;
	auto* BoxSlot = Canvas->AddChildToCanvas(Box);
	// Center the boss bar along the bottom of the viewport.
	BoxSlot->SetAnchors(FAnchors(0.5f, 1.0f));
	BoxSlot->SetAlignment(FVector2D(0.5f, 1.0f));
	BoxSlot->SetPosition(FVector2D(0.0f, -20.0f));
	BoxSlot->SetSize(FVector2D(580.0f, 20.0f));
	HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	Box->AddChildToVerticalBox(HealthBar)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	SetVisibility(ESlateVisibility::HitTestInvisible);
	Panel->SetVisibility(ESlateVisibility::Collapsed);
}

void URLBossHealthWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!GetOwningPlayerPawn()) { Panel->SetVisibility(ESlateVisibility::Collapsed); return; }
	if (!BoundBoss.IsValid() || !BoundBoss->IsPoolActive())
	{
		BoundBoss.Reset();
		SearchDelay -= InDeltaTime;
		if (SearchDelay <= 0.0f && GetWorld())
		{
			SearchDelay = 0.25f;
			for (TActorIterator<ARLRobotBossCharacter> It(GetWorld()); It; ++It)
			{
				if (It->IsPoolActive()) { BoundBoss = *It; break; }
			}
		}
	}
	ARLRobotBossCharacter* Boss = BoundBoss.Get();
	URLHealthComponent* Health = Boss ? Boss->FindComponentByClass<URLHealthComponent>() : nullptr;
	if (!Boss || Boss->IsSpawning() || !Health || Health->IsDead())
	{
		Panel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const auto* Overload = Boss->FindComponentByClass<URLBossOverloadComponent>();
	const bool bAbsorbing = Overload && Overload->IsAbsorbing();
	HealthBar->SetPercent(FMath::Clamp(Health->GetCurrentHealth() / FMath::Max(1.0f, Health->GetMaxHealth()), 0.0f, 1.0f));
	HealthBar->SetFillColorAndOpacity(bAbsorbing ? FLinearColor(0.7f, 0.04f, 1.0f) : FLinearColor(1.0f, 0.12f, 0.04f));
	Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
}
