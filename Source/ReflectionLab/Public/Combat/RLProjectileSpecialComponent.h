#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/RLProjectileSpecialSettings.h"
#include "RLProjectileSpecialComponent.generated.h"

class APawn;
class UMaterialInstanceDynamic;
class UNiagaraSystem;
class USoundConcurrency;
class URLProjectileDefinitionDataAsset;

// Owns special behavior settings and state. ARLProjectile drives updates.
UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLProjectileSpecialComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLProjectileSpecialComponent();
	void ApplyDefinitionSettings(const URLProjectileDefinitionDataAsset& Definition);
	void InitializeFeedback();
	void ResetRuntimeState();
	bool IsExplosive() const { return bIsExplosive; }
	bool IsDelayedExplosive() const { return bIsDelayedExplosive; }
	bool IsFakeProjectile() const { return bIsFakeProjectile; }
	bool IsGuardProjectile() const { return bIsGuardProjectile; }
	bool SplitsOnParry() const { return bSplitsOnParry; }
	bool ExplodesOnEnemyImpact() const { return bExplodesOnEnemyImpact; }
	float GetExplosionRadius() const { return Settings.ExplosionRadius; }
	float GetFuseRemainingSeconds() const;
	float CalculateReturnSpeed(float RequestedMultiplier, float DefaultMultiplier) const;
	void CalculateReturnScales(float RequestedScale, float& VisualScale, float& CollisionScale) const;
	bool GetVisualStyle(FLinearColor& Color, float& Emissive, float& Opacity) const;
	void DisableBehavior();
	void PrepareForReflection();
	void RestartReflectedFuse();
	void ConfigureAsExplosive();
	void ConfigureAsDelayedExplosive(AActor* TargetActor);
	void ConfigureAsFake(AActor* TargetActor);
	void ConfigureAsGuard();
	void ConfigureAsParrySplit();
	bool Detonate();
	void UpdateExplosive(float DeltaTime);
	void UpdateDelayedExplosive(float DeltaTime);
	void UpdateFake(float DeltaTime);
	void SpawnParrySplitFragments(
		AActor* OriginalOwner,
		APawn* OriginalInstigator,
		AActor* PlayerActor);
	void Explode();
	void TriggerExplosion(bool bDamagePlayer, bool bDamageEnemies);

private:
	void SpawnExplosionVisual(
		const FVector& ExplosionLocation,
		float BlastRadius);
	void ApplyExplosionDamage(
		const FVector& ExplosionLocation,
		float BlastRadius,
		bool bDamagePlayer,
		bool bDamageEnemies);
	void ConfigureAsSplitFragment(float SpeedMultiplier, float VisualScale);
	void RevealFakeProjectile();
	void ApplyExplosiveBlinkColor(bool bUseWarningColor);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Special", meta = (AllowPrivateAccess = "true"))
	FRLProjectileSpecialSettings Settings;

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
	float FakeDormantElapsedTime = 0.0f;
	TWeakObjectPtr<AActor> DelayedExplosiveTarget;
	TWeakObjectPtr<AActor> FakeTarget;
	bool bIsExplosive = false;
	bool bExplosiveBlinkWarning = false;
	bool bIsDelayedExplosive = false;
	bool bDelayedExplosivePaused = false;
	bool bDelayedExplosiveResumed = false;
	bool bIsFakeProjectile = false;
	bool bFakeDormant = false;
	bool bIsGuardProjectile = false;
	bool bSplitsOnParry = false;
	bool bExplodesOnEnemyImpact = false;
};
