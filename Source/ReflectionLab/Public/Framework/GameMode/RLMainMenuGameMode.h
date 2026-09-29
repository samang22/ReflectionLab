#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RLMainMenuGameMode.generated.h"

class URLMainMenuWidget;

UCLASS()
class REFLECTIONLAB_API ARLMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARLMainMenuGameMode();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<URLMainMenuWidget> MainMenuWidget;
};
