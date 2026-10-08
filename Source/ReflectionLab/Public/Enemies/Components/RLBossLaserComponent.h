#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLBossLaserComponent.generated.h"
class URLBossLaserDataAsset;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UDecalComponent;
class UMaterialInstanceDynamic;

UCLASS(ClassGroup=(ReflectionLab), meta=(BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLBossLaserComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URLBossLaserComponent();
	void Initialize(USkeletalMeshComponent* InMesh, UStaticMeshComponent* InBeam, UDecalComponent* InDecal);
	void Cancel(bool bResumeCombat = false);
	bool IsPreparing() const { return Phase == EPhase::Preparing; }
	bool IsFiring() const { return Phase == EPhase::Firing; }
	bool IsFading() const { return Phase == EPhase::Fading; }
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|Laser") TObjectPtr<URLBossLaserDataAsset> Settings;
	// Opt-in minion group; the boss laser remains independent.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|Laser") bool bSerializeWithPeers = false;
protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	enum class EPhase : uint8 { Waiting, Preparing, Firing, Fading };
	void BeginPreparation();
	void BeginFiring();
	void BeginFadeOut();
	void UpdateVisualOpacity(float Opacity);
	void UpdateBeam(float DeltaTime, bool bApplyDamage);
	void UpdatePreparationPreview();
	void UpdateDecalProgress(float Progress);
	bool HasActivePeerLaser() const;
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Mesh;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Beam;
	UPROPERTY(Transient) TObjectPtr<UDecalComponent> Decal;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> DecalMaterialInstance;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BeamMaterialInstance;
	EPhase Phase = EPhase::Waiting;
	FVector LockedDirection = FVector::ForwardVector;
	float Remaining = 0.0f;
	float DamageDelay = 0.0f;
};
