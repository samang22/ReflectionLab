// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RLPlayerCharacter.generated.h"

class UCameraComponent;
class UAnimMontage;
class USoundBase;
class USphereComponent;
class USpringArmComponent;
class ARLProjectile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FRLPlayerHealthChangedSignature,
	float,
	CurrentHealth,
	float,
	MaxHealth);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FRLParryChainChangedSignature,
	int32,
	ParryChainCount);

UCLASS()
class REFLECTIONLAB_API ARLPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARLPlayerCharacter();

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "Player|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Player|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Player|Health")
	bool IsDead() const { return CurrentHealth <= 0.0f; }

	UPROPERTY(BlueprintAssignable, Category = "Player|Health")
	FRLPlayerHealthChangedSignature OnHealthChanged;

	UFUNCTION(BlueprintCallable, Category = "Parry")
	void StartParry();

	UFUNCTION(BlueprintCallable, Category = "Parry|Animation")
	void BeginParryWindow();

	UFUNCTION(BlueprintCallable, Category = "Parry|Animation")
	void EndParryWindow();

	UFUNCTION(BlueprintPure, Category = "Parry")
	bool IsParryActive() const { return bParryActive; }

	UFUNCTION(BlueprintPure, Category = "Parry")
	bool IsParryOnCooldown() const { return bParryOnCooldown; }

	UFUNCTION(BlueprintPure, Category = "Parry")
	int32 GetParryChainCount() const { return ParryChainCount; }

	UPROPERTY(BlueprintAssignable, Category = "Parry")
	FRLParryChainChangedSignature OnParryChainChanged;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Player|Health")
	void Die();
	virtual void Die_Implementation();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 3.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Health")
	float CurrentHealth = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parry", meta = (AllowPrivateAccess = "true", DisplayName = "Parry Detection Zone"))
	TObjectPtr<USphereComponent> ReflectionZone;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Animation", meta = (AllowPrivateAccess = "true", DisplayName = "Parry Montage"))
	TObjectPtr<UAnimMontage> ParryMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback", meta = (AllowPrivateAccess = "true", DisplayName = "Parry Impact Sound"))
	TObjectPtr<USoundBase> ParryImpactSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Timing", meta = (ClampMin = "0.0", DisplayName = "Failed Parry Cooldown"))
	float ReflectionCooldown = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Timing", meta = (ClampMin = "0.0"))
	float SuccessfulParryCooldown = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Chain", meta = (ClampMin = "0.0"))
	float ParryChainGracePeriod = 0.8f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Parry|Chain")
	int32 ParryChainCount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Detection", meta = (ClampMin = "1.0", DisplayName = "Parry Range"))
	float ReflectionRange = 110.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Detection", meta = (ClampMin = "0.0", ClampMax = "180.0", DisplayName = "Parry Half Angle"))
	float ReflectionHalfAngleDegrees = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Detection", meta = (ClampMin = "1", DisplayName = "Max Parries Per Activation"))
	int32 MaxReflectionsPerActivation = 1;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void UpdateParry();
	bool TryParryProjectile(ARLProjectile* Projectile);
	void EndParry(bool bSucceeded);
	void ResetParryCooldown();
	void ResetParryChain();

	FTimerHandle ParryAttemptTimerHandle;
	FTimerHandle ParryCooldownTimerHandle;
	FTimerHandle ParryChainResetTimerHandle;

	int32 ParriesThisActivation = 0;
	bool bParryAttemptInProgress = false;
	bool bParryActive = false;
	bool bParryOnCooldown = false;
};
