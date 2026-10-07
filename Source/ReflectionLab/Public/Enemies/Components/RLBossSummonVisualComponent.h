#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLBossSummonVisualComponent.generated.h"

class UDecalComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class ARLEnemyCharacter;
class USkeletalMeshComponent;

USTRUCT()
struct FRLSummonMarker
{
	GENERATED_BODY()
	UPROPERTY(Transient) TObjectPtr<UDecalComponent> Decal;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Material;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> SourceMaterial;
	TWeakObjectPtr<ARLEnemyCharacter> Minion;
	float Elapsed = 0.0f;
	float Duration = 0.6f;
	float FadeElapsed = 0.0f;
	bool bActive = false;
	bool bAttached = false;
	bool bPulseOnly = false;
};

UCLASS(ClassGroup=(ReflectionLab))
class REFLECTIONLAB_API URLBossSummonVisualComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URLBossSummonVisualComponent();
	int32 ShowMarker(const FVector& GroundLocation, float Duration, float Radius, UMaterialInterface* Material, bool bPulseOnly = false);
	void AttachMinion(int32 MarkerIndex, ARLEnemyCharacter* Minion);
	void HideMarker(int32 MarkerIndex);
	void PlayBossPulse(USkeletalMeshComponent* Mesh, UMaterialInterface* Material);
	void Reset();
protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	void RestorePulse();
	// Reuse owner-registered decals rather than allocating an actor each summon.
	UPROPERTY(Transient) TArray<FRLSummonMarker> Markers;
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> PulseMesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> PulseMaterial;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> PulseSourceMaterial;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInterface>> PulseOriginalMaterials;
	float PulseElapsed = 0.0f;
};
