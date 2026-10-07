#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RLBossHealthWidget.generated.h"

class ARLRobotBossCharacter;
class UProgressBar;
class UVerticalBox;

UCLASS()
class REFLECTIONLAB_API URLBossHealthWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
private:
	TWeakObjectPtr<ARLRobotBossCharacter> BoundBoss;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Panel;
	float SearchDelay = 0.0f;
};
