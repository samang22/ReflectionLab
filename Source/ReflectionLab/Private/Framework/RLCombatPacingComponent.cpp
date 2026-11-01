#include "Framework/RLCombatPacingComponent.h"
#include "Framework/GameMode/RLGameModeBase.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/Components/RLEnemyAttackComponent.h"
#include "Combat/RLProjectile.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

URLCombatPacingComponent::URLCombatPacingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void URLCombatPacingComponent::Configure(const FRLCombatPacingSettings& NewSettings, bool bOrdinaryCombatRound)
{
	ResetPacing();
	Settings = NewSettings;
	Settings.bEnabled &= bOrdinaryCombatRound;
	SetComponentTickInterval(FMath::IsFinite(Settings.UpdateInterval) ? FMath::Clamp(Settings.UpdateInterval, 0.05f, 0.5f) : 0.15f);
	SetComponentTickEnabled(Settings.bEnabled);
}

void URLCombatPacingComponent::ResetPacing()
{
	for (const auto& Entry : AdjustedEnemies)
	{
		if (auto* Attack = Entry.Get()) { Attack->ClearCleanupPressure(); }
	}
	AdjustedEnemies.Empty();
	Settings.bEnabled = false;
	SetComponentTickEnabled(false);
}

void URLCombatPacingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function)
{
	Super::TickComponent(DeltaTime, TickType, Function);
	const auto* Mode = Cast<ARLGameModeBase>(GetOwner());
	if (!Mode || Mode->GetRunState() != ERLRunState::PlayingRound) { ResetPacing(); return; }
	UpdatePacing();
}

void URLCombatPacingComponent::UpdatePacing()
{
	if (!Settings.bEnabled || !GetWorld()) { return; }
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player) { return; }
	TArray<ARLEnemyCharacter*> Enemies;
	for (TActorIterator<ARLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (It->IsPoolActive()) { Enemies.Add(*It); }
	}
	const float Rate = Settings.GetAttackRate(Enemies.Num());
	if (Rate <= 1)
	{
		for (const auto& Entry : AdjustedEnemies) { if (auto* Attack = Entry.Get()) { Attack->ClearCleanupPressure(); } }
		AdjustedEnemies.Empty();
		return;
	}
	TSet<AActor*> IncomingOwners;
	for (TActorIterator<ARLProjectile> It(GetWorld()); It; ++It)
	{
		if (!It->IsPoolActive() || It->IsFadingOut() || It->IsReflected()) { continue; }
		const auto* Motion = It->FindComponentByClass<UProjectileMovementComponent>();
		if (Motion && Settings.IsApproaching(It->GetActorLocation(), Motion->Velocity, Player->GetActorLocation()))
		{
			IncomingOwners.Add(It->GetOwner());
		}
	}
	TArray<TWeakObjectPtr<URLEnemyAttackComponent>> Current;
	for (auto* Enemy : Enemies)
	{
		if (auto* Attack = Enemy->FindComponentByClass<URLEnemyAttackComponent>())
		{
			Attack->SetCleanupPressure(Rate, IncomingOwners.Contains(Enemy), Settings.NoThreatMaxWait, Settings.UpdateInterval);
			Current.Add(Attack);
		}
	}
	for (const auto& Entry : AdjustedEnemies)
	{
		if (!Current.Contains(Entry)) { if (auto* Attack = Entry.Get()) { Attack->ClearCleanupPressure(); } }
	}
	AdjustedEnemies = MoveTemp(Current);
}

void URLCombatPacingComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	ResetPacing();
	Super::EndPlay(Reason);
}
