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

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLHealthComponentTest, "ReflectionLab.Player.Health",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLHealthComponentTest::RunTest(const FString& Parameters)
{
	URLHealthComponent* Health = NewObject<URLHealthComponent>();
	Health->InitializeHealth(10.0f);
	TestEqual(TEXT("Full initial HP"), Health->GetCurrentHealth(), 10.0f);
	TestEqual(TEXT("Damage"), Health->ApplyDamage(3.0f), 3.0f);
	TestEqual(TEXT("Negative damage rejected"), Health->ApplyDamage(-1.0f), 0.0f);
	TestEqual(TEXT("Recovery"), Health->RestoreHealth(1.0f), 1.0f);
	Health->SetMaxHealth(12.0f, true);
	TestEqual(TEXT("Vitality preserves missing HP"), Health->GetCurrentHealth(), 10.0f);
	TestEqual(TEXT("Healing capped"), Health->RestoreHealth(20.0f), 2.0f);
	Health->SetMaxHealth(10.0f);
	TestEqual(TEXT("Max reduction clamps HP"), Health->GetCurrentHealth(), 10.0f);
	TestEqual(TEXT("Overkill clamped"), Health->ApplyDamage(100.0f), 10.0f);
	TestTrue(TEXT("Dead"), Health->IsDead());
	TestEqual(TEXT("Dead damage ignored"), Health->ApplyDamage(1.0f), 0.0f);
	TestEqual(TEXT("Healing cannot revive"), Health->RestoreHealth(10.0f), 0.0f);
	Health->SetMaxHealth(12.0f, true);
	TestTrue(TEXT("Vitality cannot revive"), Health->IsDead());
	Health->InitializeHealth(10.0f);
	TestFalse(TEXT("Explicit reset revives"), Health->IsDead());
	return true;
}
#endif
