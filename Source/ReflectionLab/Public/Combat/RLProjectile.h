// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class URLProjectilePoolSubsystem;

UCLASS()
class REFLECTIONLAB_API ARLProjectile : public AActor
{
	GENERATED_BODY()

public:
	ARLProjectile();

	void ActivateProjectile(
		const FTransform& SpawnTransform,
		AActor* NewOwner,
		APawn* NewInstigator);

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	bool Reflect(
		AActor* NewOwner,
		APawn* NewInstigator,
		const FVector& NewDirection);

	UFUNCTION(BlueprintPure, Category = "Projectile")
	bool IsReflected() const { return bIsReflected; }

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Visuals")
	TObjectPtr<UStaticMeshComponent> ReflectedTrailMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.0"))
	float DamageAmount = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Movement", meta = (ClampMin = "1.0"))
	float ProjectileSpeed = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Movement", meta = (ClampMin = "1.0"))
	float ReflectedSpeedMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.1"))
	float LifeSeconds = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visuals")
	TObjectPtr<UMaterialInterface> HostileMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visuals")
	TObjectPtr<UMaterialInterface> ReflectedMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visuals")
	FVector ReflectedTrailScale = FVector(0.8f, 0.08f, 0.08f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Visuals")
	float ReflectedTrailOffset = -50.0f;

private:
	friend class URLProjectilePoolSubsystem;

	void DeactivateForPool();
	void UpdateProjectileMaterial();
	bool ShouldIgnoreActor(const AActor* OtherActor) const;
	void ApplyDamageAndReturn(AActor* OtherActor);

	FTimerHandle LifetimeTimerHandle;
	bool bIsActive = false;
	bool bIsReflected = false;
};
