// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class USoundConcurrency;
class URLProjectilePoolSubsystem;
class URLProjectileDefinitionDataAsset;

USTRUCT(BlueprintType)
struct FRLProjectileReflectionParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Reflection")
	float SpeedMultiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Reflection", meta = (ClampMin = "0.1"))
	float VisualScaleMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Reflection", meta = (ClampMin = "0"))
	int32 PierceCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Reflection", meta = (ClampMin = "0"))
	int32 ReflectionChain = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Reflection")
	bool bPerfectParry = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Reflection")
	bool bCloseRangeParry = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Reflection")
	bool bOverdrive = false;
};

UCLASS()
class REFLECTIONLAB_API ARLProjectile : public AActor
{
	GENERATED_BODY()

public:
	ARLProjectile();
	virtual void Tick(float DeltaTime) override;

	void ActivateProjectile(
		const FTransform& SpawnTransform,
		AActor* NewOwner,
		APawn* NewInstigator);

	void InitializeFromDefinition(
		URLProjectileDefinitionDataAsset* Definition,
		AActor* TargetActor,
		bool bConfigureBehavior = true);

	URLProjectileDefinitionDataAsset* GetProjectileDefinition() const
	{
		return ActiveDefinition;
	}

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	bool Reflect(
		AActor* NewOwner,
		APawn* NewInstigator,
		const FVector& NewDirection,
		const FRLProjectileReflectionParams& ReflectionParams);

	UFUNCTION(BlueprintPure, Category = "Projectile")
	bool IsReflected() const { return bIsReflected; }

	UFUNCTION(BlueprintPure, Category = "Projectile")
	bool CanBeReflected() const { return bCanBeReflected; }

	UFUNCTION(BlueprintPure, Category = "Projectile|Guard")
	bool IsGuardProjectile() const { return bIsGuardProjectile; }

	UFUNCTION(BlueprintPure, Category = "Projectile|Explosive")
	bool IsExplosive() const { return bIsExplosive; }

	UFUNCTION(BlueprintPure, Category = "Projectile")
	bool IsPoolActive() const { return bIsActive; }

	UFUNCTION(BlueprintPure, Category = "Projectile|Explosive")
	float GetExplosiveFuseRemainingSeconds() const
	{
		return bIsExplosive
			? FMath::Max(0.0f, ExplosiveFuseDuration - ExplosiveElapsedTime)
			: 0.0f;
	}

	UFUNCTION(BlueprintPure, Category = "Projectile|Explosive")
	float GetExplosionRadius() const { return ExplosionRadius; }

	UFUNCTION(BlueprintCallable, Category = "Projectile|Explosive")
	bool Detonate();

	UFUNCTION(BlueprintCallable, Category = "Projectile|Explosive")
	void ConfigureAsExplosive();

	UFUNCTION(BlueprintCallable, Category = "Projectile|Delayed Explosive")
	void ConfigureAsDelayedExplosive(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "Projectile|Fake")
	void ConfigureAsFake(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "Projectile|Guard")
	void ConfigureAsGuard();

	UFUNCTION(BlueprintCallable, Category = "Projectile|Parry Split")
	void ConfigureAsParrySplit();

	UFUNCTION(BlueprintCallable, Category = "Projectile|Rally")
	void ConfigureAsRally(AActor* FinalTarget, int32 InMaxRallies, float InSpeedMultiplierPerRally);

	UFUNCTION(BlueprintPure, Category = "Projectile|Reflection")
	float GetBaseReflectedSpeedMultiplier() const { return ReflectedSpeedMultiplier; }

	UFUNCTION(BlueprintPure, Category = "Projectile|Reflection")
	int32 GetRemainingPierces() const { return RemainingPierces; }

	UFUNCTION(BlueprintPure, Category = "Projectile|Reflection")
	int32 GetReflectionChain() const { return ReflectionChain; }

	UFUNCTION(BlueprintPure, Category = "Projectile|Reflection")
	bool WasPerfectParried() const { return bWasPerfectParried; }

	UFUNCTION(BlueprintPure, Category = "Projectile|Reflection")
	bool WasCloseRangeParried() const { return bWasCloseRangeParried; }

	UFUNCTION(BlueprintPure, Category = "Projectile|Reflection")
	bool WasOverdriveReflected() const { return bWasOverdriveReflected; }

	void ReturnToPool();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleProjectileHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UFUNCTION()
	void HandleProjectileOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Visuals|VFX")
	TObjectPtr<UNiagaraComponent> ReflectedTrailComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.0"))
	float DamageAmount = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Movement", meta = (ClampMin = "1.0"))
	float ProjectileSpeed = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Movement", meta = (ClampMin = "1.0"))
	float ReflectedSpeedMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.1"))
	float LifeSeconds = 999.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Bounds")
	FVector2D PlayAreaCenter = FVector2D::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Bounds", meta = (ClampMin = "1.0"))
	FVector2D PlayAreaHalfExtent = FVector2D(2200.0f, 2200.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visuals", meta = (ClampMin = "0.0"))
	float FadeOutDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visuals")
	TObjectPtr<UMaterialInterface> HostileMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visuals")
	TObjectPtr<UMaterialInterface> ReflectedMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float ExplosiveSpeedMultiplier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "1.0"))
	float ExplosiveVisualScale = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "0.1"))
	float ExplosiveFuseDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "1.0"))
	float ExplosionRadius = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive", meta = (ClampMin = "0.0"))
	float ExplosionDamage = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive|Audio")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Explosive|Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float ExplosionSoundVolume = 0.8f;

	UPROPERTY(Transient)
	TObjectPtr<USoundConcurrency> ExplosionSoundConcurrency;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Rally", meta = (ClampMin = "1.0"))
	float RallyVisualScale = 1.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Rally", meta = (ClampMin = "1.0"))
	float RallyMaxSpeedMultiplier = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Rally", meta = (ClampMin = "1.0"))
	float RallyArrivalRadius = 48.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visuals|Afterimage", meta = (ClampMin = "1", ClampMax = "8"))
	int32 ReflectedAfterimageCount = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visuals|Afterimage", meta = (ClampMin = "0.01", ClampMax = "0.2"))
	float ReflectedAfterimageSampleInterval = 0.035f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visuals|Afterimage", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float ReflectedAfterimageScaleFalloff = 0.12f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> ReflectedAfterimageMeshes;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ExplosiveMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FadeMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SpecialMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> ReflectedTrailVFX;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> ExplosionVFX;

	float ReflectedTrailScale = 1.0f;
	float ExplosionVFXScale = 1.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Projectile|Explosive")
	void OnExploded(const FVector& ExplosionLocation, float BlastRadius);

private:
	friend class URLProjectilePoolSubsystem;

	void DeactivateForPool();
	void UpdateFadeOut(float DeltaTime);
	void CompleteReturnToPool();
	void CreateReflectedAfterimages();
	void ResetReflectedAfterimages();
	void UpdateReflectedAfterimages(float DeltaTime);
	void UpdateProjectileMaterial();
	void ActivateReflectedTrail(float VisualScaleMultiplier);
	void DeactivateReflectedTrail();
	void UpdateExplosive(float DeltaTime);
	void ApplyExplosiveBlinkColor(bool bUseWarningColor);
	void UpdateDelayedExplosive(float DeltaTime);
	void UpdateFake(float DeltaTime);
	void RevealFakeProjectile();
	void SpawnParrySplitFragments(
		AActor* OriginalOwner,
		APawn* OriginalInstigator,
		AActor* PlayerActor);
	void ConfigureAsSplitFragment(float SpeedMultiplier, float VisualScale);
	void ApplySpecialColor(const FLinearColor& Color, float EmissiveIntensity, float Opacity = 1.0f);
	void SpawnExplosionVisual(const FVector& ExplosionLocation, float BlastRadius);
	void Explode();
	void TriggerExplosion(bool bDamagePlayer, bool bDamageEnemies);
	void UpdateRally(float DeltaTime);
	void AdvanceRally();
	AActor* FindNextRallyTarget(AActor* RelaySource) const;
	void SetRallyTarget(AActor* NewTarget);
	void SetProjectileOwnerAndIgnore(AActor* NewOwner);
	void SetProjectileSpeed(float NewSpeed, const FVector& Direction);
	bool IsOutsidePlayArea() const;
	bool TryDetonateOnPlayerContact(AActor* OtherActor);
	bool TryExplodeOnEnemyContact(AActor* OtherActor);
	bool ShouldIgnoreActor(const AActor* OtherActor) const;
	void ApplyDamageAndReturn(AActor* OtherActor);
	void ApplyDefinitionStats(const URLProjectileDefinitionDataAsset& Definition);

	FTimerHandle LifetimeTimerHandle;
	TArray<FTransform> ReflectedAfterimageHistory;
	FVector DefaultProjectileMeshScale = FVector::OneVector;
	float DefaultCollisionRadius = 16.0f;
	float ReflectedAfterimageSampleAccumulator = 0.0f;
	float FadeOutElapsedTime = 0.0f;
	float ExplosiveElapsedTime = 0.0f;
	float ExplosiveNextBlinkTime = 0.0f;
	float DelayedExplosivePauseElapsedTime = 0.0f;
	float FakeDormantElapsedTime = 0.0f;
	float RallyCurrentSpeed = 0.0f;
	float RallySpeedMultiplierPerRally = 1.15f;
	int32 RallyCount = 0;
	int32 MaxRallies = 0;
	TWeakObjectPtr<AActor> RallyTarget;
	TWeakObjectPtr<AActor> RallyFinalTarget;
	TWeakObjectPtr<AActor> DelayedExplosiveTarget;
	TWeakObjectPtr<AActor> FakeTarget;
	bool bIsActive = false;
	bool bIsFadingOut = false;
	bool bIsReflected = false;
	bool bCanBeReflected = true;
	bool bIsExplosive = false;
	bool bExplosiveBlinkWarning = false;
	bool bIsDelayedExplosive = false;
	bool bDelayedExplosivePaused = false;
	bool bDelayedExplosiveResumed = false;
	bool bIsFakeProjectile = false;
	bool bFakeDormant = false;
	bool bIsGuardProjectile = false;
	bool bSplitsOnParry = false;
	bool bIsRallyProjectile = false;
	bool bRallyFinalShot = false;
	int32 RemainingPierces = 0;
	int32 ReflectionChain = 0;
	bool bWasPerfectParried = false;
	bool bWasCloseRangeParried = false;
	bool bWasOverdriveReflected = false;
	bool bExplodesOnEnemyImpact = false;

	UPROPERTY(Transient)
	TObjectPtr<URLProjectileDefinitionDataAsset> ActiveDefinition;
};
