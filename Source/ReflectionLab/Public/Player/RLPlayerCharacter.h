#pragma once

#include "CoreMinimal.h"
#include "Data/RLRunRewardTypes.h"
#include "GameFramework/Character.h"
#include "RLPlayerCharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class URLPlayerStatsDataAsset;
class URLHealthComponent;
class URLHitRecoveryComponent;
class URLDodgeRollComponent;
class URLParryComponent;
class URLParryProgressionComponent;
class URLRunRewardComponent;
class URLParryFeedbackComponent;

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
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintPure, Category = "Player|Health")
	float GetMaxHealth() const;
	URLHealthComponent* GetHealthComponent() const { return HealthComponent; }
	void RestoreHealth(float Amount);
	bool HasPerfectRecoveryReward() const;

	UFUNCTION(BlueprintPure, Category = "Player|Health")
	bool IsDead() const;
	bool IsInvincibilityEnabled() const { return bInvincibilityEnabled; }
	void ToggleInvincibility() { bInvincibilityEnabled = !bInvincibilityEnabled; }

	UFUNCTION(BlueprintPure, Category = "Player|Hit Recovery")
	bool IsInHitRecovery() const;

	UPROPERTY(BlueprintAssignable, Category = "Player|Health")
	FRLPlayerHealthChangedSignature OnHealthChanged;

	UFUNCTION(BlueprintCallable, Category = "Parry")
	void StartParry();
	void StartRoll(const FVector& Direction);
	bool IsRolling() const;
	URLDodgeRollComponent* GetDodgeRollComponent() const { return DodgeRollComponent; }

	UFUNCTION(BlueprintCallable, Category = "Parry|Animation")
	void BeginParryWindow();

	UFUNCTION(BlueprintCallable, Category = "Parry|Animation")
	void EndParryWindow();

	UFUNCTION(BlueprintPure, Category = "Parry")
	bool IsParryActive() const;

	UFUNCTION(BlueprintPure, Category = "Parry")
	bool IsParryOnCooldown() const;

	UFUNCTION(BlueprintPure, Category = "Parry")
	int32 GetParryChainCount() const;

	UFUNCTION(BlueprintPure, Category = "Parry|Combo")
	int32 GetParryComboCount() const { return GetParryChainCount(); }

	UFUNCTION(BlueprintPure, Category = "Parry|Enhancement")
	int32 GetParryEnhancementLevel() const;

	UFUNCTION(BlueprintCallable, Category = "Run|Rewards")
	void ApplyRunReward(ERLRunRewardType RewardType);

	UFUNCTION(BlueprintCallable, Category = "Run|Rewards")
	void ResetRunRewards();

	UPROPERTY(BlueprintAssignable, Category = "Parry")
	FRLParryChainChangedSignature OnParryChainChanged;

	UPROPERTY(BlueprintAssignable, Category = "Parry|Combo")
	FRLParryComboChangedSignature OnParryComboChanged;

	URLParryComponent* GetParryComponent() const { return ParryComponent; }
	URLParryProgressionComponent* GetParryProgressionComponent() const { return ParryProgressionComponent; }
	URLRunRewardComponent* GetRunRewardComponent() const { return RunRewardComponent; }
	URLPlayerStatsDataAsset* GetPlayerStatsData() const { return PlayerStatsData; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Weapon")
	TObjectPtr<UStaticMeshComponent> BatMesh;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Player|Health")
	void Die();
	virtual void Die_Implementation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Health")
	TObjectPtr<URLHealthComponent> HealthComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Hit Recovery")
	TObjectPtr<URLHitRecoveryComponent> HitRecoveryComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Roll")
	TObjectPtr<URLDodgeRollComponent> DodgeRollComponent;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Config")
	TObjectPtr<URLPlayerStatsDataAsset> PlayerStatsData;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parry")
	TObjectPtr<URLParryComponent> ParryComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parry|Enhancement")
	TObjectPtr<URLParryProgressionComponent> ParryProgressionComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Run|Rewards")
	TObjectPtr<URLRunRewardComponent> RunRewardComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parry|Feedback")
	TObjectPtr<URLParryFeedbackComponent> ParryFeedbackComponent;

private:
	UPROPERTY(Transient)
	bool bInvincibilityEnabled = false;
	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);
	UFUNCTION()
	void HandleDeath();
	void ApplyPlayerStats();
	bool CanPerformParry() const;
	void RefreshParryFeedback();
	void HandleRewardsChanged();
	void HandleParryChainChanged(int32 ChainCount);
	void HandleParryComboChanged(int32 ComboCount, int32 MultiParryCount,
		int32 EnhancementLevel, bool bPerfectParry, bool bCloseRangeParry);
	void HandleHitRecoveryStarted();
	void HandleHitRecoveryEnded();
};
