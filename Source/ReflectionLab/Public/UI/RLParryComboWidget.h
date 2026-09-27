#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RLParryComboWidget.generated.h"

class ARLPlayerCharacter;
class UProgressBar;
class UTextBlock;

UCLASS()
class REFLECTIONLAB_API URLParryComboWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindToPlayer(ARLPlayerCharacter* PlayerCharacter);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void HandleParryComboChanged(
		int32 ComboCount,
		int32 MultiParryCount,
		bool bPerfectParry,
		bool bCloseRangeParry);

	void BuildWidgetTree();
	void ResetDisplay();
	void UpdateMilestoneProgress(int32 ComboCount);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FeedbackText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ComboText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ComboProgressBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MilestoneText;

	UPROPERTY(Transient)
	TObjectPtr<ARLPlayerCharacter> BoundPlayerCharacter;

	float PopTimeRemaining = 0.0f;
	float ComboBreakTimeRemaining = 0.0f;
	int32 DisplayedComboCount = 0;
};
