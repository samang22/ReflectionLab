#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLBossOverloadComponent.generated.h"

class ARLProjectile;
class URLBossOverloadDataAsset;
class URLHealthComponent;
class USphereComponent;
class UDecalComponent;
class USkeletalMeshComponent;
class UPrimitiveComponent;

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLBossOverloadComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URLBossOverloadComponent();
	void Initialize(URLHealthComponent* InHealth, USkeletalMeshComponent* InMesh,
		USphereComponent* InVolume, UDecalComponent* InDecal, TSubclassOf<ARLProjectile> InProjectileClass);
	void Reset();
	bool TryAbsorb(ARLProjectile* Projectile);
	bool OwnsVolume(const UPrimitiveComponent* Component) const;
	bool IsOverloading() const { return bOverloading; }
	bool WantsOverload() const { return bOverloading || bPendingOverload; }
	bool IsAbsorbing() const { return bAbsorbing; }
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Overload")
	TObjectPtr<URLBossOverloadDataAsset> Settings;
protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float NewMaxHealth);
	void BeginOverload();
	void FinishOverload(bool bResumeCombat);
	void FireSpinningShot();
	UPROPERTY(Transient) TObjectPtr<URLHealthComponent> Health;
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Mesh;
	UPROPERTY(Transient) TObjectPtr<USphereComponent> Volume;
	UPROPERTY(Transient) TObjectPtr<UDecalComponent> Decal;
	UPROPERTY(Transient) TSubclassOf<ARLProjectile> ProjectileClass;
	FVector BaseMeshScale = FVector::OneVector;
	float PreviousHealth = 0.0f;
	float AccumulatedDamage = 0.0f;
	float Elapsed = 0.0f;
	float UntilNextVolley = 0.0f;
	bool bOverloading = false;
	bool bAbsorbing = false;
	bool bPendingOverload = false;
};
