// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/RLProjectileReflectionTypes.h"
#include "RLProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class URLProjectilePoolSubsystem;
class URLProjectileFeedbackComponent;
class URLProjectileVisualComponent;
class URLProjectileSpecialComponent;
class URLProjectileRallyComponent;
class URLProjectileContactComponent;
class URLProjectileDefinitionDataAsset;

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
	bool IsGuardProjectile() const;

	UFUNCTION(BlueprintPure, Category = "Projectile|Explosive")
	bool IsExplosive() const;

	UFUNCTION(BlueprintPure, Category = "Projectile|Rally")
	bool IsRallyProjectile() const;

	UFUNCTION(BlueprintPure, Category = "Projectile")
	bool IsPoolActive() const { return bIsActive; }

	UFUNCTION(BlueprintPure, Category = "Projectile|Explosive")
	float GetExplosiveFuseRemainingSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Projectile|Explosive")
	float GetExplosionRadius() const;

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
	int32 GetRemainingPierces() const { return ReflectionState.RemainingPierces; }

	UFUNCTION(BlueprintPure, Category = "Projectile|Reflection")
	int32 GetReflectionChain() const { return ReflectionState.ReflectionChain; }

	UFUNCTION(BlueprintPure, Category = "Projectile|Reflection")
	bool WasPerfectParried() const { return ReflectionState.bWasPerfectParried; }

	UFUNCTION(BlueprintPure, Category = "Projectile|Reflection")
	bool WasCloseRangeParried() const { return ReflectionState.bWasCloseRangeParried; }

	UFUNCTION(BlueprintPure, Category = "Projectile|Reflection")
	bool WasOverdriveReflected() const { return ReflectionState.bWasOverdriveReflected; }

	void ReturnToPool();

	// Component-facing queries and lifecycle commands. No mutable state is exposed.
	bool IsFadingOut() const { return bIsFadingOut; }
	float GetBaseSpeed() const { return ProjectileSpeed; }
	float GetLifetimeSeconds() const { return LifeSeconds; }
	float GetDamageAmount() const { return DamageAmount; }
	const FVector& GetDefaultVisualScale() const { return DefaultProjectileMeshScale; }
	float GetDefaultCollisionRadius() const { return DefaultCollisionRadius; }
	USphereComponent* GetCollisionSphere() const { return CollisionComponent; }
	UStaticMeshComponent* GetVisualMesh() const { return ProjectileMesh; }
	UNiagaraComponent* GetTrailComponent() const { return ReflectedTrailComponent; }
	URLProjectileVisualComponent* GetVisualComponent() const { return VisualComponent; }
	URLProjectileSpecialComponent* GetSpecialComponent() const { return SpecialComponent; }
	URLProjectileRallyComponent* GetRallyComponent() const { return RallyComponent; }
	URLProjectileContactComponent* GetContactComponent() const { return ContactComponent; }
	void SetParryEnabled(bool bEnabled) { bCanBeReflected = bEnabled; }
	void SetProjectileOwnerAndIgnore(AActor* NewOwner);
	void SetProjectileSpeed(float NewSpeed, const FVector& Direction);
	void PauseMotion();
	void RestartLifetime(float Duration);
	void NotifyExplosion(const FVector& Location, float Radius);
	bool TryConsumePierce(float AppliedDamage);
	// Pool lifecycle endpoint; the pool owns bucket bookkeeping.
	void DeactivateForPool();

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Feedback")
	TObjectPtr<URLProjectileFeedbackComponent> FeedbackComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components")
	TObjectPtr<URLProjectileVisualComponent> VisualComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components")
	TObjectPtr<URLProjectileSpecialComponent> SpecialComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components")
	TObjectPtr<URLProjectileRallyComponent> RallyComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components")
	TObjectPtr<URLProjectileContactComponent> ContactComponent;

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

	UFUNCTION(BlueprintImplementableEvent, Category = "Projectile|Explosive")
	void OnExploded(const FVector& ExplosionLocation, float BlastRadius);

private:

	void CompleteReturnToPool();
	bool IsOutsidePlayArea() const;
	void ApplyDefinitionStats(const URLProjectileDefinitionDataAsset& Definition);

	FTimerHandle LifetimeTimerHandle;
	FVector DefaultProjectileMeshScale = FVector::OneVector;
	float DefaultCollisionRadius = 16.0f;
	bool bIsActive = false;
	bool bIsFadingOut = false;
	bool bIsReflected = false;
	bool bCanBeReflected = true;
	FRLProjectileReflectionState ReflectionState;

	UPROPERTY(Transient)
	TObjectPtr<URLProjectileDefinitionDataAsset> ActiveDefinition;
};
