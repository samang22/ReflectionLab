#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/RLProjectileVisualSettings.h"
#include "RLProjectileVisualComponent.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UNiagaraSystem;
class URLProjectileDefinitionDataAsset;

// Owns visual settings and state. ARLProjectile drives the update order.
UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLProjectileVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLProjectileVisualComponent();
	void ApplyDefinitionSettings(const URLProjectileDefinitionDataAsset& Definition);
	void ResetRuntimeState();
	UMaterialInterface* GetReflectedMaterial() const { return Settings.ReflectedMaterial; }
	void BeginFadeOut();
	bool UpdateFadeOut(float DeltaTime);
	bool HasImmediateFadeOut() const { return Settings.FadeOutDuration <= KINDA_SMALL_NUMBER; }
	void CreateReflectedAfterimages();
	void ResetReflectedAfterimages();
	void UpdateReflectedAfterimages(float DeltaTime);
	void UpdateProjectileMaterial();
	void ActivateReflectedTrail(float VisualScaleMultiplier);
	void UpdateReflectedTrailLocation();
	void DeactivateReflectedTrail();
	void ApplySpecialColor(
		const FLinearColor& Color,
		float EmissiveIntensity,
		float Opacity = 1.0f);

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visual", meta = (AllowPrivateAccess = "true"))
	FRLProjectileVisualSettings Settings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> ReflectedAfterimageMeshes;
	TArray<FTransform> ReflectedAfterimageHistory;
	float ReflectedAfterimageSampleAccumulator = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FadeMaterialInstance;
	float FadeOutElapsedTime = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SpecialMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> ReflectedTrailVFX;
	float ReflectedTrailScale = 1.0f;
};
