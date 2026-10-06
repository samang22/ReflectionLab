#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLEnemyMovementComponent.generated.h"

class URLEnemyMovementDataAsset;

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
	void BeginReposition();
	void CancelReposition();
	bool IsRepositioning() const { return bWaitingToMove || bMoving; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	TObjectPtr<URLEnemyMovementDataAsset> MovementSettings;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	void SelectMovingEnemy();
	void RequestReposition();
	bool bSelectedToMove = false;
	bool bWaitingToMove = false;
	bool bMoving = false;
	float RepositionTimeRemaining = 0.0f;
	bool bTutorialMovementLocked = false;
};

