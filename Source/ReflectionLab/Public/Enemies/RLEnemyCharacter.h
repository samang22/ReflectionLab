// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/RLDifficultyScheduleDataAsset.h"
#include "Engine/DataTable.h"
#include "GameFramework/Character.h"
#include "RLEnemyCharacter.generated.h"

class USceneComponent;
class USoundBase;
class UMaterialInterface;
class UStaticMesh;
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

	UFUNCTION(BlueprintCallable, Category = "Enemy|Difficulty")
	void ApplyDifficultyPhase(const FRLDifficultyPhase& DifficultyPhase);

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback", meta = (ClampMin = "0.0"))
	float HitSoundVolume = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback", meta = (ClampMin = "0.1"))
	float HitSoundPitchMin = 0.94f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback", meta = (ClampMin = "0.1"))
	float HitSoundPitchMax = 1.06f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback|Death")
	TObjectPtr<UStaticMesh> DeathEffectShardMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback|Death")
	TObjectPtr<UMaterialInterface> DeathEffectShardMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback|Death")
	FVector DeathEffectShardScale = FVector(0.22f, 0.07f, 0.07f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback|Death", meta = (ClampMin = "1.0"))
	float DeathEffectRadius = 75.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback|Death")
	FLinearColor DeathEffectColor = FLinearColor(1.0f, 0.015f, 0.005f, 1.0f);

private:
	friend class URLEnemyPoolSubsystem;

	void ApplyCombatConfig();
	void CacheBaseCombatValues();
	void SpawnDeathEffect();
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
	int32 ExplosiveShotInterval = 5;
	int32 RallyShotInterval = 3;
	int32 RallyRelayCount = 2;
	float RallySpeedMultiplierPerRelay = 1.15f;
	float BaseAttackInterval = 3.0f;
	int32 BaseShotsPerBurst = 1;
	float BaseTimeBetweenShots = 0.5f;
	int32 RemainingShotsInBurst = 0;
	int32 ShotsFiredSinceActivation = 0;
	bool bIsPoolActive = true;
};
