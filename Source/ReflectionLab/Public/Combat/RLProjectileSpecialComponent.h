#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/RLProjectileSpecialSettings.h"
#include "RLProjectileSpecialComponent.generated.h"

class APawn;
class URLProjectileExplosionComponent;
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
	bool IsExplosive() const;
	bool IsDelayedExplosive() const;
	bool IsFakeProjectile() const { return bIsFakeProjectile; }
	bool IsGuardProjectile() const { return bIsGuardProjectile; }
	bool SplitsOnParry() const { return bSplitsOnParry; }
	bool ExplodesOnEnemyImpact() const;
	float GetExplosionRadius() const;
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
		AActor* PlayerActor, const FGuid& ParryAttemptId);
	void Explode();
	void TriggerExplosion(bool bDamagePlayer, bool bDamageEnemies);

private:
	void ConfigureAsSplitFragment(float SpeedMultiplier, float VisualScale);
	void RevealFakeProjectile();
	URLProjectileExplosionComponent* Explosion() const;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Special", meta = (AllowPrivateAccess = "true"))
	FRLProjectileSpecialSettings Settings;

	float FakeDormantElapsedTime = 0;
	TWeakObjectPtr<AActor> FakeTarget;
	bool bIsFakeProjectile = false;
	bool bFakeDormant = false;
	bool bIsGuardProjectile = false;
	bool bSplitsOnParry = false;
};
