#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RLRewardSelectionWidget.generated.h"
class ARLGameModeBase;
class UTextBlock;
class UButton;
class UBorder;
class UHorizontalBox;
class UImage;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRLRewardSelectionFinished, int32, ChoiceIndex);

UCLASS()
class REFLECTIONLAB_API URLRewardSelectionWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	FRLRewardSelectionFinished OnSelectionFinished;
	void BindToGameMode(ARLGameModeBase* GameMode);
	void Refresh();
	UWidget* GetDefaultFocusTarget() const;
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
	void BuildWidgetTree();
	UFUNCTION() void HandleRewardChoiceOneClicked();
	UFUNCTION() void HandleRewardChoiceTwoClicked();
	UFUNCTION() void HandleRewardChoiceThreeClicked();
	void UpdateRewardChoices();
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> RewardContainer;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> RewardBackdrop;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RewardPromptText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> RewardButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> RewardCardBorders;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> RewardTitleTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> RewardDescriptionTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> RewardArtPanels;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> RewardArtImages;

	void UpdateRewardEntrance(float DeltaSeconds);
	void BeginRewardSelectionAnimation(int32 ChoiceIndex);
	void UpdateRewardSelectionAnimation(float DeltaSeconds);
	int32 SelectedRewardIndex = INDEX_NONE;
	float RewardSelectionElapsed = 0.0f;
	float RewardEntranceElapsed = 0.0f;
	bool bRewardEntranceVisible = false;
	bool bSelectionSubmitted = false;
	UPROPERTY(Transient) TObjectPtr<ARLGameModeBase> BoundGameMode;
};
