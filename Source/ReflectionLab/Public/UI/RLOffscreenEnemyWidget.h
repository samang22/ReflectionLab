#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RLOffscreenEnemyWidget.generated.h"

UCLASS()
class REFLECTIONLAB_API URLOffscreenEnemyWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(
		const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy Indicators", meta = (ClampMin = "16.0"))
	float EdgeMargin = 36.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy Indicators", meta = (ClampMin = "4.0"))
	float ArrowSize = 12.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy Indicators", meta = (ClampMin = "0.1"))
	float ArrowThickness = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy Indicators")
	FLinearColor IndicatorColor = FLinearColor(1.0f, 0.25f, 0.12f, 0.9f);

private:
	struct FEnemyIndicator
	{
		FVector2D Position;
		FVector2D Direction;
	};

	TArray<FEnemyIndicator> Indicators;
};
