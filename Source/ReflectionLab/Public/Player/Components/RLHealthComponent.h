#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRLHealthChangedSignature, float, CurrentHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRLDeathSignature);

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLHealthComponent();

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return CurrentHealth <= 0.0f; }

	// Explicit initialization is the only operation allowed to revive the owner.
	void InitializeHealth(float NewMaxHealth);
	// Pool deactivation clears health without emitting a second death event.
	void ClearHealth();
	void SetMaxHealth(float NewMaxHealth, bool bRestoreAddedHealth = false);
	float ApplyDamage(float Amount);
	float RestoreHealth(float Amount);

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FRLHealthChangedSignature OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FRLDeathSignature OnDeath;

private:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float MaxHealth = 10.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float CurrentHealth = 0.0f;
};
