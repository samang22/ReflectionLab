#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLProjectileDefinitionDataAsset.generated.h"

class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;

UENUM(BlueprintType)
enum class ERLProjectileBehavior : uint8
{
	Normal,
	Explosive,
	DelayedExplosive,
	Fake,
	Guard,
	ParrySplit,
	Rally
};

UENUM(BlueprintType)
enum class ERLShotPattern : uint8
{
	Single,
	Cross
};

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLProjectileDefinitionDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	URLProjectileDefinitionDataAsset();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	ERLProjectileBehavior Behavior = ERLProjectileBehavior::Normal;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.0"))
	float DamageAmount = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Movement", meta = (ClampMin = "1.0"))
	float ProjectileSpeed = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Movement", meta = (ClampMin = "1.0"))
	float ReflectedSpeedMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.1"))
	float LifeSeconds = 999.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visuals", meta = (ClampMin = "0.1"))
	float VisualScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visuals", meta = (ClampMin = "0.0"))
	float FadeOutDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visuals")
	TObjectPtr<UMaterialInterface> HostileMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visuals")
	TObjectPtr<UMaterialInterface> ReflectedMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visuals|VFX")
	TObjectPtr<UNiagaraSystem> ReflectedTrailVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visuals|VFX", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float ReflectedTrailScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Impact")
	bool bExplodesOnEnemyImpact = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Impact", meta = (EditCondition = "bExplodesOnEnemyImpact", EditConditionHides))
	TObjectPtr<URLProjectileDefinitionDataAsset> EnemyImpactExplosionDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float ExplosiveSpeedMultiplier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "1.0"))
	float ExplosiveVisualScale = 2.0f;

	// Also used as the fresh countdown after a successful player parry.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "0.1"))
	float ExplosiveFuseDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "1.0"))
	float ExplosionRadius = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "1.0"))
	float ReflectedExplosionRadius = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "0.0"))
	float ExplosionDamage = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive|Audio")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive|Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float ExplosionSoundVolume = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive|VFX")
	TObjectPtr<UNiagaraSystem> ExplosionVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive|VFX", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float ExplosionVFXScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "0.01"))
	float ExplosiveBlinkStartInterval = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "0.01"))
	float ExplosiveBlinkEndInterval = 0.06f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive|Visuals")
	FLinearColor ExplosiveBaseColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive|Visuals")
	FLinearColor ExplosiveDecalColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive|Visuals")
	FLinearColor ExplosiveWarningColor = FLinearColor(1.0f, 0.85f, 0.08f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive|Visuals", meta = (ClampMin = "0.0"))
	float ExplosiveBaseEmissiveIntensity = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive|Visuals", meta = (ClampMin = "0.0"))
	float ExplosiveWarningEmissiveIntensity = 40.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Delayed Explosive", meta = (ClampMin = "1.0"))
	float DelayedExplosiveTriggerDistance = 320.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Delayed Explosive", meta = (ClampMin = "0.0"))
	float DelayedExplosivePauseDuration = 0.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Delayed Explosive", meta = (ClampMin = "0.1"))
	float DelayedExplosiveResumeSpeedMultiplier = 1.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Fake", meta = (ClampMin = "1.0"))
	float FakeTriggerDistance = 280.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Fake", meta = (ClampMin = "0.0"))
	float FakeRevealDelay = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Fake", meta = (ClampMin = "0.1"))
	float FakeRealSpeedMultiplier = 1.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Fake|Visuals")
	FLinearColor FakeColor = FLinearColor(0.12f, 0.015f, 0.22f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Fake|Visuals", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FakeOpacity = 0.38f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Guard|Visuals")
	FLinearColor GuardColor = FLinearColor(0.7f, 0.12f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Parry Split|Visuals")
	FLinearColor ParrySplitColor = FLinearColor(1.0f, 0.32f, 0.02f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Parry Split", meta = (ClampMin = "1", ClampMax = "6"))
	int32 ParrySplitFragmentCount = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Parry Split", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float ParrySplitSpreadAngle = 70.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Parry Split", meta = (ClampMin = "0.1"))
	float ParrySplitFragmentSpeedMultiplier = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Parry Split", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float ParrySplitFragmentScale = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Rally", meta = (ClampMin = "1"))
	int32 RallyRelayCount = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Rally", meta = (ClampMin = "1.0"))
	float RallySpeedMultiplierPerRelay = 1.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Rally", meta = (ClampMin = "1.0"))
	float RallyVisualScale = 1.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Rally", meta = (ClampMin = "1.0"))
	float RallyMaxSpeedMultiplier = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Rally", meta = (ClampMin = "1.0"))
	float RallyArrivalRadius = 48.0f;
};
