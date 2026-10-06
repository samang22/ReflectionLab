#pragma once

#include "CoreMinimal.h"
#include "RLProjectileSpecialSettings.generated.h"

class USoundBase;

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLProjectileSpecialSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float ExplosiveSpeedMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "1.0"))
	float ExplosiveVisualScale = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "0.1"))
	float ExplosiveFuseDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "1.0"))
	float ExplosionRadius = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "0.0"))
	float ExplosionDamage = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosive|Audio")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosive|Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float ExplosionSoundVolume = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "0.01"))
	float ExplosiveBlinkStartInterval = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "0.01"))
	float ExplosiveBlinkEndInterval = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosive|Visuals")
	FLinearColor ExplosiveBaseColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosive|Visuals")
	FLinearColor ExplosiveDecalColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosive|Visuals")
	FLinearColor ExplosiveWarningColor = FLinearColor(1.0f, 0.85f, 0.08f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosive|Visuals", meta = (ClampMin = "0.0"))
	float ExplosiveBaseEmissiveIntensity = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosive|Visuals", meta = (ClampMin = "0.0"))
	float ExplosiveWarningEmissiveIntensity = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Delayed Explosive", meta = (ClampMin = "1.0"))
	float DelayedExplosiveTriggerDistance = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Delayed Explosive", meta = (ClampMin = "0.0"))
	float DelayedExplosivePauseDuration = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Delayed Explosive", meta = (ClampMin = "0.1"))
	float DelayedExplosiveResumeSpeedMultiplier = 1.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Fake", meta = (ClampMin = "1.0"))
	float FakeTriggerDistance = 280.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Fake", meta = (ClampMin = "0.0"))
	float FakeRevealDelay = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Fake", meta = (ClampMin = "0.1"))
	float FakeRealSpeedMultiplier = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Fake|Visuals")
	FLinearColor FakeColor = FLinearColor(0.12f, 0.015f, 0.22f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Fake|Visuals", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FakeOpacity = 0.38f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Guard|Visuals")
	FLinearColor GuardColor = FLinearColor(0.7f, 0.12f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Parry Split|Visuals")
	FLinearColor ParrySplitColor = FLinearColor(1.0f, 0.32f, 0.02f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Parry Split", meta = (ClampMin = "1", ClampMax = "6"))
	int32 ParrySplitFragmentCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Parry Split", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float ParrySplitSpreadAngle = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Parry Split", meta = (ClampMin = "0.1"))
	float ParrySplitFragmentSpeedMultiplier = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Parry Split", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float ParrySplitFragmentScale = 0.75f;
};
