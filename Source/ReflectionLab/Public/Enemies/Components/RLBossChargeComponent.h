#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLBossChargeComponent.generated.h"

class ARLProjectile;
class UDecalComponent;
class USkeletalMeshComponent;
class URLBossChargeDataAsset;

UCLASS(ClassGroup=(ReflectionLab), meta=(BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLBossChargeComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URLBossChargeComponent();
	void Initialize(USkeletalMeshComponent* InMesh, UDecalComponent* InDecal, TSubclassOf<ARLProjectile> InProjectileClass);
	void Cancel(bool bResumeCombat = false);
	bool IsActive() const { return Phase != EPhase::Waiting; }
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|Charge") TObjectPtr<URLBossChargeDataAsset> Settings;
protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	enum class EPhase : uint8 { Waiting, Preparing, Charging, Braking, Recovering };
	void BeginPreparation();
	bool UpdatePreview();
	void UpdateCharge(float DeltaTime);
	void FireCounterShots();
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Mesh;
	UPROPERTY(Transient) TObjectPtr<UDecalComponent> Decal;
	UPROPERTY(Transient) TSubclassOf<ARLProjectile> ProjectileClass;
	EPhase Phase = EPhase::Waiting;
	FVector ChargeDirection = FVector::ForwardVector;
	float Remaining = 0.0f;
	float TravelRemaining = 0.0f;
	bool bContactAttempted = false;
};
