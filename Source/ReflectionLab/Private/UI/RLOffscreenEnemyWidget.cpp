#include "UI/RLOffscreenEnemyWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CanvasPanel.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/GameMode/RLGameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElementTypes.h"
#include "Widgets/SWidget.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif

namespace
{
	bool GetEdgePosition(
		const FVector2D& Size, const FVector2D& Direction, float Margin,
		FVector2D& OutPosition)
	{
		const FVector2D HalfSize = Size * 0.5f - FVector2D(Margin, Margin);
		if (HalfSize.X <= 0.0f || HalfSize.Y <= 0.0f || Direction.IsNearlyZero())
		{
			return false;
		}

		const double ScaleX = FMath::IsNearlyZero(Direction.X)
			? TNumericLimits<double>::Max() : HalfSize.X / FMath::Abs(Direction.X);
		const double ScaleY = FMath::IsNearlyZero(Direction.Y)
			? TNumericLimits<double>::Max() : HalfSize.Y / FMath::Abs(Direction.Y);
		OutPosition = Size * 0.5f + Direction * FMath::Min(ScaleX, ScaleY);
		return true;
	}
}

void URLOffscreenEnemyWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		WidgetTree->RootWidget = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("EnemyIndicatorCanvas"));
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void URLOffscreenEnemyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Indicators.Reset();
	// Positions change as the camera moves, even when the widget layout is cached.
	if (const TSharedPtr<SWidget> SlateWidget = GetCachedWidget())
	{
		SlateWidget->Invalidate(EInvalidateWidgetReason::Paint);
	}

	APlayerController* PlayerController = GetOwningPlayer();
	const UWorld* World = GetWorld();
	const ARLGameModeBase* GameMode = World
		? World->GetAuthGameMode<ARLGameModeBase>() : nullptr;
	if (!PlayerController || !PlayerController->GetPawn() || !GameMode ||
		GameMode->GetRunState() != ERLRunState::PlayingRound || World->IsPaused())
	{
		return;
	}

	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
	const FVector2D Size = MyGeometry.GetLocalSize();
	if (ViewportWidth <= 0 || ViewportHeight <= 0 || Size.X <= 0.0f || Size.Y <= 0.0f)
	{
		return;
	}

	const FVector2D PixelToLocal(Size.X / ViewportWidth, Size.Y / ViewportHeight);
	for (TActorIterator<ARLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		const ARLEnemyCharacter* Enemy = *It;
		if (!Enemy->IsPoolActive() || Enemy->IsHidden())
		{
			continue;
		}

		FVector2D ScreenPosition;
		const bool bProjected = PlayerController->ProjectWorldLocationToScreen(
			Enemy->GetActorLocation(), ScreenPosition, true);
		if (bProjected && ScreenPosition.X >= 0.0f && ScreenPosition.X <= ViewportWidth &&
			ScreenPosition.Y >= 0.0f && ScreenPosition.Y <= ViewportHeight)
		{
			continue;
		}

		FVector2D Direction;
		if (bProjected)
		{
			Direction = ScreenPosition * PixelToLocal - Size * 0.5f;
		}
		else if (const APlayerCameraManager* Camera = PlayerController->PlayerCameraManager)
		{
			const FVector ToEnemy = Enemy->GetActorLocation() - Camera->GetCameraLocation();
			const FRotationMatrix CameraRotation(Camera->GetCameraRotation());
			Direction = FVector2D(
				FVector::DotProduct(ToEnemy, CameraRotation.GetUnitAxis(EAxis::Y)),
				-FVector::DotProduct(ToEnemy, CameraRotation.GetUnitAxis(EAxis::Z)));
			if (Direction.IsNearlyZero())
			{
				Direction = FVector2D(0.0f, 1.0f);
			}
		}
		else
		{
			continue;
		}

		FVector2D Position;
		if (GetEdgePosition(Size, Direction, FMath::Max(EdgeMargin, ArrowSize + 4.0f), Position))
		{
			Indicators.Add({ Position, Direction.GetSafeNormal() });
		}
	}
}

int32 URLOffscreenEnemyWidget::NativePaint(
	const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(
		Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const FLinearColor Color = IndicatorColor * InWidgetStyle.GetColorAndOpacityTint();
	for (const FEnemyIndicator& Indicator : Indicators)
	{
		const FVector2D Side(-Indicator.Direction.Y, Indicator.Direction.X);
		const FVector2D Tip = Indicator.Position + Indicator.Direction * ArrowSize;
		const FVector2D Back = Indicator.Position - Indicator.Direction * ArrowSize * 0.6f;
		const TArray<FVector2D> ArrowPoints = {
			Back + Side * ArrowSize * 0.7f, Tip, Back - Side * ArrowSize * 0.7f
		};
		FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 1,
			AllottedGeometry.ToPaintGeometry(), ArrowPoints,
			ESlateDrawEffect::None, Color, true, 3.0f);
	}
	return Indicators.IsEmpty() ? BaseLayer : BaseLayer + 1;
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLOffscreenEnemyEdgeTest,
	"ReflectionLab.UI.OffscreenEnemy.EdgePlacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLOffscreenEnemyEdgeTest::RunTest(const FString& Parameters)
{
	FVector2D Position;
	const FVector2D Size(1920.0f, 1080.0f);
	const TArray<FVector2D> Directions = {
		FVector2D(1, 0), FVector2D(-1, 0), FVector2D(0, 1), FVector2D(0, -1),
		FVector2D(4, 3), FVector2D(-4, -3)
	};
	for (const FVector2D& Direction : Directions)
	{
		TestTrue(TEXT("Offscreen direction has a valid edge position"),
			GetEdgePosition(Size, Direction, 36.0f, Position));
		TestTrue(TEXT("Arrow stays inside the inset rectangle"),
			Position.X >= 36.0f && Position.X <= 1884.0f &&
			Position.Y >= 36.0f && Position.Y <= 1044.0f);
		TestTrue(TEXT("Position lies on an edge"),
			FMath::IsNearlyEqual(Position.X, 36.0) || FMath::IsNearlyEqual(Position.X, 1884.0) ||
			FMath::IsNearlyEqual(Position.Y, 36.0) || FMath::IsNearlyEqual(Position.Y, 1044.0));
		TestTrue(TEXT("Edge position preserves the enemy direction"),
			(Position - Size * 0.5f).GetSafeNormal().Equals(Direction.GetSafeNormal(), 0.001));
	}
	TestFalse(TEXT("Zero direction does not produce an arrow"),
		GetEdgePosition(Size, FVector2D::ZeroVector, 36.0f, Position));
	TestFalse(TEXT("Too small viewport does not produce an arrow"),
		GetEdgePosition(FVector2D(50, 50), FVector2D(1, 0), 36.0f, Position));
	TestTrue(TEXT("Portrait viewport is supported"),
		GetEdgePosition(FVector2D(600, 1000), FVector2D(1, 1), 36.0f, Position));
	TestTrue(TEXT("Portrait arrow intersects the right edge"), Position.Equals(FVector2D(564, 764)));
	return true;
}
#endif
