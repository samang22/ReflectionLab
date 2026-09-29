#include "Framework/GameMode/RLMainMenuGameMode.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "UI/RLMainMenuWidget.h"

ARLMainMenuGameMode::ARLMainMenuGameMode()
{
	DefaultPawnClass = nullptr;
	bStartPlayersAsSpectators = true;
}

void ARLMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = GetWorld()
		? GetWorld()->GetFirstPlayerController()
		: nullptr;
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("Main menu requires a local player controller."));
		return;
	}

	MainMenuWidget = CreateWidget<URLMainMenuWidget>(
		PlayerController,
		URLMainMenuWidget::StaticClass());
	if (!MainMenuWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create the main menu widget."));
		return;
	}

	MainMenuWidget->AddToViewport(100);
	FInputModeUIOnly InputMode;
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;
}
