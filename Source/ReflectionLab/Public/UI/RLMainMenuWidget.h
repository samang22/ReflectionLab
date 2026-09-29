#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RLMainMenuWidget.generated.h"

class UTextBlock;

UCLASS()
class REFLECTIONLAB_API URLMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION()
	void HandleTutorialClicked();

	UFUNCTION()
	void HandleStartGameClicked();

	UFUNCTION()
	void HandleOptionsClicked();

	void BuildWidgetTree();
	void OpenGameplayLevel(const FString& RunMode) const;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	FName GameplayLevelName = TEXT("Main");

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;
};
