#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/RLProjectileSpecialSettings.h"
#include "RLProjectileExplosionComponent.generated.h"
class URLProjectileDefinitionDataAsset;
class UMaterialInstanceDynamic;
class USoundConcurrency;
class UNiagaraSystem;

// Explosion configuration, fuse, delayed motion and feedback share one reset boundary.
UCLASS(ClassGroup=(ReflectionLab))
class REFLECTIONLAB_API URLProjectileExplosionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URLProjectileExplosionComponent();
	void SetDefaults(const FRLProjectileExplosionSettings& Defaults) { Settings = Defaults; }
	void ApplyDefinitionSettings(const URLProjectileDefinitionDataAsset& Definition);
	void InitializeFeedback();
	void ResetRuntimeState();
	void DisableBehavior();
	void PrepareForReflection();
	void RestartReflectedFuse();
	bool IsExplosive() const { return bIsExplosive; }
	bool IsDelayedExplosive() const { return bIsDelayedExplosive; }
	bool ExplodesOnEnemyImpact() const { return bExplodesOnEnemyImpact; }
	float GetExplosionRadius() const { return Settings.ExplosionRadius; }
	float GetFuseRemainingSeconds() const;
	float CalculateReturnSpeed(float RequestedMultiplier, float DefaultMultiplier) const;
	void CalculateReturnScales(float RequestedScale, float& VisualScale, float& CollisionScale) const;
	void ConfigureAsExplosive();
	void ConfigureAsDelayedExplosive(AActor* TargetActor);
	bool Detonate();
	void UpdateExplosive(float DeltaTime);
	void UpdateDelayedExplosive(float DeltaTime);
	void Explode();
	void TriggerExplosion(bool bDamagePlayer, bool bDamageEnemies);
private:
	void ApplyExplosiveBlinkColor(bool bUseWarningColor);
	void SpawnExplosionVisual(const FVector& ExplosionLocation, float BlastRadius);
	void ApplyExplosionDamage(const FVector& ExplosionLocation, float BlastRadius, bool bDamagePlayer, bool bDamageEnemies);
	UPROPERTY(Transient) FRLProjectileExplosionSettings Settings;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ExplosiveMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<USoundConcurrency> ExplosionSoundConcurrency;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> ExplosionVFX;
	float ExplosionVFXScale = 1.0f;
	float ReflectedExplosionRadius = 300.0f;
	float ExplosiveElapsedTime = 0.0f;
	float ExplosiveNextBlinkTime = 0.0f;
	float DelayedExplosivePauseElapsedTime = 0.0f;
	TWeakObjectPtr<AActor> DelayedExplosiveTarget;
	bool bIsExplosive = false;
	bool bExplosiveBlinkWarning = false;
	bool bIsDelayedExplosive = false;
	bool bDelayedExplosivePaused = false;
	bool bDelayedExplosiveResumed = false;
	bool bExplodesOnEnemyImpact = false;
};
