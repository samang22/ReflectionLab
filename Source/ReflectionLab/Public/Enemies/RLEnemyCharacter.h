// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Character.h"
#include "RLEnemyCharacter.generated.h"

class USceneComponent;
class ARLProjectile;
class URLEnemyPoolSubsystem;

UCLASS()
class REFLECTIONLAB_API ARLEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ARLEnemyCharacter();

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "Enemy|Pool")
	bool IsPoolActive() const { return bIsPoolActive; }

	UFUNCTION(BlueprintCallable, Category = "Enemy|Pool")
	void ReturnToPool();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Config", meta = (RowType = "/Script/ReflectionLab.RLEnemyCombatRow"))
	FDataTableRowHandle CombatConfig;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	void StartFiring();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	void StopFiring();

	virtual void Fire();
	virtual void Die();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Health")
	float CurrentHealth = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	TObjectPtr<USceneComponent> MuzzlePoint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat")
	TSubclassOf<ARLProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat")
	bool bAutoStartFiring = true;

private:
	friend class URLEnemyPoolSubsystem;

	void ApplyCombatConfig();
	void ActivateFromPool(const FTransform& SpawnTransform);
	void DeactivateForPool();
	void BeginBurst();
	void FireNextShot();

	FTimerHandle AttackTimerHandle;
	FTimerHandle BurstTimerHandle;
	float MaxHealth = 1.0f;
	int32 ProjectilePoolPrewarmCount = 16;
	float AttackInterval = 3.0f;
	int32 ShotsPerBurst = 1;
	float TimeBetweenShots = 0.5f;
	float InitialFireDelay = 1.0f;
	int32 RemainingShotsInBurst = 0;
	bool bIsPoolActive = true;
};
