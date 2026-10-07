#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLBossAimComponent.generated.h"

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLBossAimComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URLBossAimComponent();
	void AimAtPlayer(FName SocketName);
	bool IsAligned(FName SocketName) const;
	bool RequestShot(FSimpleDelegate OnAligned);
	void CancelShot();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Aim", meta = (ClampMin = "0.1"))
	float RotationInterpSpeed = 8.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Aim", meta = (ClampMin = "0.01", ClampMax = "5.0"))
	float AlignmentToleranceDegrees = 0.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Aim", meta = (ClampMin = "0.0"))
	float ShotAimHoldDuration = 0.2f;
protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
	float ShotAimRemaining = 0.0f;
	FSimpleDelegate PendingShot;
};
