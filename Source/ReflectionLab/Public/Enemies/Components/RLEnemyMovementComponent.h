#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLEnemyMovementComponent.generated.h"

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLEnemyMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLEnemyMovementComponent();
	void InitializeMovement();
	void ActivateForPool();
	void DeactivateForPool();
	void SetTutorialMovementLocked(bool bLocked);
	void UpdateFacingPlayer();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool bTutorialMovementLocked = false;
};

