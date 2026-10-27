#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLAccelerationBarrier.generated.h"
class UBoxComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class URLBallisticsExperimentComponent;
class ARLProjectile;

UCLASS(NotBlueprintable)
class REFLECTIONLAB_API ARLAccelerationBarrier : public AActor
{
	GENERATED_BODY()
public:
	ARLAccelerationBarrier();
	void Configure(const FVector& Size, UMaterialInterface* Material, URLBallisticsExperimentComponent* Field);
	void HandleProjectile(ARLProjectile* Projectile, const FVector& ContactPosition);
	virtual void Tick(float DeltaSeconds) override;
private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Sensor;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Surface;
	TWeakObjectPtr<URLBallisticsExperimentComponent> Experiment;
	float FlashRemaining = 0;
};
