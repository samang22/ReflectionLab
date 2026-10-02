#include "Player/Components/RLHealthComponent.h"

URLHealthComponent::URLHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URLHealthComponent::InitializeHealth(float NewMaxHealth)
{
	if (!FMath::IsFinite(NewMaxHealth))
	{
		return;
	}
	MaxHealth = FMath::Max(1.0f, NewMaxHealth);
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void URLHealthComponent::SetMaxHealth(float NewMaxHealth, bool bRestoreAddedHealth)
{
	if (!FMath::IsFinite(NewMaxHealth))
	{
		return;
	}
	const float PreviousMaxHealth = MaxHealth;
	MaxHealth = FMath::Max(1.0f, NewMaxHealth);
	if (bRestoreAddedHealth && !IsDead())
	{
		CurrentHealth += FMath::Max(0.0f, MaxHealth - PreviousMaxHealth);
	}
	CurrentHealth = FMath::Min(CurrentHealth, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

float URLHealthComponent::ApplyDamage(float Amount)
{
	if (!FMath::IsFinite(Amount) || Amount <= 0.0f || IsDead())
	{
		return 0.0f;
	}
	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - Amount);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	if (IsDead())
	{
		OnDeath.Broadcast();
	}
	return PreviousHealth - CurrentHealth;
}

float URLHealthComponent::RestoreHealth(float Amount)
{
	if (!FMath::IsFinite(Amount) || Amount <= 0.0f || IsDead() || CurrentHealth >= MaxHealth)
	{
		return 0.0f;
	}
	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + Amount);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	return CurrentHealth - PreviousHealth;
}
