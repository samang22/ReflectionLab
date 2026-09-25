// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RLEnemyCharacter.generated.h"

class USceneComponent;
class ARLProjectile;

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

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	void StartFiring();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	void StopFiring();

	virtual void Fire();
	virtual void Die();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 1.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Health")
	float CurrentHealth = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	TObjectPtr<USceneComponent> MuzzlePoint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat")
	TSubclassOf<ARLProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "0"))
	int32 ProjectilePoolPrewarmCount = 16;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "0.1"))
	float AttackInterval = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "1"))
	int32 ShotsPerBurst = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "0.01"))
	float TimeBetweenShots = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "0.0"))
	float InitialFireDelay = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat")
	bool bAutoStartFiring = true;

private:
	void BeginBurst();
	void FireNextShot();

	FTimerHandle AttackTimerHandle;
	FTimerHandle BurstTimerHandle;
	int32 RemainingShotsInBurst = 0;
};
