// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/RLRunRewardTypes.h"
#include "GameFramework/Character.h"
#include "RLPlayerCharacter.generated.h"

class UCameraComponent;
class UDecalComponent;
class UAnimMontage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
	FRLParryComboChangedSignature,
	int32,
	ComboCount,
	int32,
	MultiParryCount,
	int32,
	EnhancementLevel,
	bool,
	bPerfectParry,
	bool,
	bCloseRangeParry);

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
	void RestoreHealth(float Amount);
	bool HasPerfectRecoveryReward() const { return RunRewardPerfectRecoveryAmount > 0; }

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

	UFUNCTION(BlueprintPure, Category = "Parry|Combo")
	int32 GetParryComboCount() const { return ParryChainCount; }

	UFUNCTION(BlueprintPure, Category = "Parry|Enhancement")
	int32 GetParryEnhancementLevel() const { return ParryEnhancementLevel; }

	UFUNCTION(BlueprintCallable, Category = "Run|Rewards")
	void ApplyRunReward(ERLRunRewardType RewardType);

	UFUNCTION(BlueprintCallable, Category = "Run|Rewards")
	void ResetRunRewards();

	UPROPERTY(BlueprintAssignable, Category = "Parry")
	FRLParryChainChangedSignature OnParryChainChanged;

	UPROPERTY(BlueprintAssignable, Category = "Parry|Combo")
	FRLParryComboChangedSignature OnParryComboChanged;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Player|Health")
	void Die();
	virtual void Die_Implementation();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Health")
	float MaxHealth = 10.0f;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Hit Recovery|Feedback")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Hit Recovery|Feedback", meta = (ClampMin = "0.0"))
	float HitSoundVolume = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Hit Recovery|Feedback", meta = (ClampMin = "0.1"))
	float HitSoundPitchMin = 0.97f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Hit Recovery|Feedback", meta = (ClampMin = "0.1"))
	float HitSoundPitchMax = 1.03f;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parry|Overdrive|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> OverdriveAuraComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> ParryRangeIndicatorMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	bool bShowParryRangeIndicator = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	FLinearColor ParryAvailableIndicatorColor = FLinearColor(0.05f, 1.0f, 0.15f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	FLinearColor PerfectParryAvailableIndicatorColor = FLinearColor(0.05f, 1.0f, 0.45f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	FLinearColor ParryCooldownIndicatorColor = FLinearColor(1.0f, 0.05f, 0.03f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	FLinearColor PerfectParryCooldownIndicatorColor = FLinearColor(1.0f, 0.25f, 0.03f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	FLinearColor ParrySuccessIndicatorColor = FLinearColor(0.02f, 0.45f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	FLinearColor PerfectParrySuccessIndicatorColor = FLinearColor(0.02f, 0.75f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float ParrySuccessIndicatorDuration = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Animation", meta = (AllowPrivateAccess = "true", DisplayName = "Parry Montage"))
	TObjectPtr<UAnimMontage> ParryMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Animation", meta = (AllowPrivateAccess = "true", DisplayName = "Mirrored Parry Montage"))
	TObjectPtr<UAnimMontage> MirroredParryMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback", meta = (AllowPrivateAccess = "true", DisplayName = "Parry Impact Sound"))
	TObjectPtr<USoundBase> ParryImpactSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback", meta = (AllowPrivateAccess = "true", DisplayName = "Parry Swing Sound"))
	TObjectPtr<USoundBase> ParrySwingSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Config", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URLParryTuningDataAsset> ParryTuningData;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Parry|Chain")
	int32 ParryChainCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Parry|Enhancement")
	int32 ParryEnhancementLevel = 1;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Parry|Enhancement")
	int32 EnhancementComboProgress = 0;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	friend class FRLTutorialRewardTest;
	void ApplyPlayerStats();
	void ApplyParryTuning();
	void UpdateParryRangeIndicator();
	void UpdateOverdriveAura();
	void UpdateParryIndicatorColor();
	void ShowParrySuccessIndicator();
	void ClearParrySuccessIndicator();
	void PlayParrySwingSound() const;
	void PlayParryImpactSound(const FVector& SoundLocation, int32 EnhancementLevel) const;
	void TriggerParryHitStop(bool bPerfectParry, bool bOverdrive);
	void RestoreTimeDilation();
	void BeginHitRecovery();
	void EndHitRecovery();
	void StartHitFlash();
	void ToggleHitFlash();
	void StopHitFlash();
	void UpdateParry();
	bool IsProjectileWithinParryArc(const ARLProjectile* Projectile) const;
	bool DetonateExplosiveOnParryAttempt();
	bool TryParryProjectile(
		ARLProjectile* Projectile,
		int32 ResultingCombo,
		bool& bOutPerfectParry,
		bool& bOutCloseRangeParry);
	void RegisterSuccessfulParry(
		int32 MultiParryCount,
		bool bPerfectParry,
		bool bCloseRangeParry,
		bool bOverdrive);
	void SpawnAdditionalReflectedProjectiles(
		ARLProjectile* SourceProjectile,
		const TArray<FVector>& SplitDirections,
		const struct FRLProjectileReflectionParams& ReflectionParams);
	void EndParry(bool bSucceeded);
	void ResetParryCooldown();
	void ResetParryChain();
	void ConsumeOverdriveEnhancement();
	void DowngradeParryEnhancement();
	void SetParryEnhancementLevel(int32 NewLevel);
	int32 GetEnhancementComboRequirement() const;
	void ApplyRunRewardModifiers();

	FTimerHandle ParryAttemptTimerHandle;
	FTimerHandle ParryCooldownTimerHandle;
	FTimerHandle ParrySuccessIndicatorTimerHandle;
	FTimerHandle ParryHitStopTimerHandle;
	FTimerHandle HitRecoveryTimerHandle;
	FTimerHandle HitFlashTimerHandle;

	float ReflectionCooldown = 0.5f;
	float SuccessfulParryCooldown = 0.03f;
	float PerfectParryOuterBandWidth = 30.0f;
	int32 PerfectSplitProjectileCount = 3;
	float PerfectSplitAngleDegrees = 40.0f;
	float PerfectHitStopDurationMultiplier = 1.75f;
	int32 BasePierceCount = 0;
	float MaxReflectedSpeedMultiplier = 2.0f;
	float BaseReflectedProjectileScale = 1.35f;
	float CloseRangeThreshold = 55.0f;
	int32 CloseRangePierceCount = 3;
	float CloseRangeProjectileScale = 1.7f;
	int32 ComboSpeedMilestone = 3;
	int32 ComboExtraProjectileMilestone = 5;
	int32 EnhancementStage2Combo = 3;
	int32 EnhancementStage3Combo = 6;
	int32 EnhancementStage4Combo = 9;
	float ComboExtraProjectileSpreadAngle = 18.0f;
	int32 OverdriveComboThreshold = 8;
	int32 OverdriveProjectileCount = 5;
	float OverdriveSpreadAngleDegrees = 100.0f;
	float OverdriveProjectileScale = 2.0f;
	int32 OverdrivePierceCount = 3;
	float OverdriveHitStopDurationMultiplier = 2.5f;
	float ReflectionRange = 140.0f;
	float ReflectionHalfAngleDegrees = 50.0f;
	float ParryIndicatorIdleOpacity = 0.18f;
	float ParryIndicatorActiveOpacity = 0.38f;
	float ParryIndicatorSuccessOpacity = 0.55f;
	float ParryIndicatorUnavailableOpacity = 0.22f;
	float ParryImpactSoundVolume = 0.65f;
	float ParrySwingSoundVolume = 0.45f;
	TArray<TObjectPtr<USoundBase>> ParryComboImpactSounds;
	TArray<float> ParryComboImpactSoundVolumes;
	float ParryHitStopDuration = 0.04f;
	float ParryHitStopTimeDilation = 0.1f;
	float OverdriveAuraBaseScale = 1.0f;
	float EnhancementAuraStage2ScaleMultiplier = 0.1f;
	float EnhancementAuraStage3ScaleMultiplier = 0.3f;
	float EnhancementAuraStage4ScaleMultiplier = 0.7f;
	float PreHitRecoveryMaxWalkSpeed = 0.0f;
	float RunRewardRangeMultiplier = 1.0f;
	float RunRewardArcBonusDegrees = 0.0f;
	int32 RunRewardPierceBonus = 0;
	int32 RunRewardPerfectSplitBonus = 0;
	float RunRewardReflectedSpeedBonus = 0.0f;
	float RunRewardCloseRangeBonus = 0.0f;
	float RunRewardMaxHealthBonus = 0.0f;
	int32 RunRewardPerfectRecoveryAmount = 0;
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
