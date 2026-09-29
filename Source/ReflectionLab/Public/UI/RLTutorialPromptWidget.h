#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RLTutorialPromptWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class REFLECTIONLAB_API URLTutorialPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPrompt(const FText& Title, const FText& Body);

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION()
	void HandleContinueClicked();

	void BuildWidgetTree();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ContinueButton;
};
