// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RLPlayerCharacter.generated.h"

class UCameraComponent;
class UDecalComponent;
class UAnimMontage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class URLParryTuningDataAsset;
class URLPlayerStatsDataAsset;
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

	UFUNCTION(BlueprintPure, Category = "Player|Hit Recovery")
	bool IsInHitRecovery() const { return bHitRecoveryActive; }

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Player|Health")
	void Die();
	virtual void Die_Implementation();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Health")
	float MaxHealth = 3.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Health")
	float CurrentHealth = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Hit Recovery")
	float HitRecoveryDuration = 0.6f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Hit Recovery")
	float HitRecoveryMovementSpeedMultiplier = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Config")
	TObjectPtr<URLPlayerStatsDataAsset> PlayerStatsData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Hit Recovery|Feedback")
	TObjectPtr<UMaterialInterface> HitFlashMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Hit Recovery|Feedback", meta = (ClampMin = "0.0"))
	float HitFlashInterval = 0.08f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parry", meta = (AllowPrivateAccess = "true", DisplayName = "Parry Detection Zone"))
	TObjectPtr<USphereComponent> ReflectionZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDecalComponent> ParryRangeIndicator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> ParryRangeIndicatorMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	bool bShowParryRangeIndicator = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	FLinearColor ParryAvailableIndicatorColor = FLinearColor(0.05f, 1.0f, 0.15f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	FLinearColor ParryCooldownIndicatorColor = FLinearColor(1.0f, 0.05f, 0.03f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	FLinearColor ParrySuccessIndicatorColor = FLinearColor(0.02f, 0.45f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float ParrySuccessIndicatorDuration = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Animation", meta = (AllowPrivateAccess = "true", DisplayName = "Parry Montage"))
	TObjectPtr<UAnimMontage> ParryMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Animation", meta = (AllowPrivateAccess = "true", DisplayName = "Mirrored Parry Montage"))
	TObjectPtr<UAnimMontage> MirroredParryMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback", meta = (AllowPrivateAccess = "true", DisplayName = "Parry Impact Sound"))
	TObjectPtr<USoundBase> ParryImpactSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Config", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URLParryTuningDataAsset> ParryTuningData;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Parry|Chain")
	int32 ParryChainCount = 0;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void ApplyPlayerStats();
	void ApplyParryTuning();
	void UpdateParryRangeIndicator();
	void UpdateParryIndicatorColor();
	void ShowParrySuccessIndicator();
	void ClearParrySuccessIndicator();
	void TriggerParryHitStop();
	void RestoreTimeDilation();
	void BeginHitRecovery();
	void EndHitRecovery();
	void StartHitFlash();
	void ToggleHitFlash();
	void StopHitFlash();
	void UpdateParry();
	bool TryParryProjectile(ARLProjectile* Projectile);
	void EndParry(bool bSucceeded);
	void ResetParryCooldown();
	void ResetParryChain();

	FTimerHandle ParryAttemptTimerHandle;
	FTimerHandle ParryCooldownTimerHandle;
	FTimerHandle ParryChainResetTimerHandle;
	FTimerHandle ParrySuccessIndicatorTimerHandle;
	FTimerHandle ParryHitStopTimerHandle;
	FTimerHandle HitRecoveryTimerHandle;
	FTimerHandle HitFlashTimerHandle;

	float ReflectionCooldown = 0.5f;
	float SuccessfulParryCooldown = 0.03f;
	float ParryChainGracePeriod = 1.0f;
	float ReflectionRange = 140.0f;
	float ReflectionHalfAngleDegrees = 50.0f;
	float ParryIndicatorIdleOpacity = 0.18f;
	float ParryIndicatorActiveOpacity = 0.38f;
	float ParryIndicatorSuccessOpacity = 0.55f;
	float ParryIndicatorUnavailableOpacity = 0.22f;
	float ParryImpactSoundVolume = 0.65f;
	float ParryHitStopDuration = 0.04f;
	float ParryHitStopTimeDilation = 0.1f;
	float PreHitRecoveryMaxWalkSpeed = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ParryRangeIndicatorMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PreviousOverlayMaterial;

	bool bParryAttemptInProgress = false;
	bool bParryActive = false;
	bool bParryOnCooldown = false;
	bool bPlayMirroredParryNext = false;
	bool bShowingParrySuccessIndicator = false;
	bool bParryHitStopActive = false;
	bool bHitRecoveryActive = false;
	bool bHitFlashVisible = false;
};
