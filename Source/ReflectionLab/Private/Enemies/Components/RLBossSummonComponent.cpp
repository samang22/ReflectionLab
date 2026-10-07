#include "Enemies/Components/RLBossSummonComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/RLBossSummonDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/RLEnemyPoolSubsystem.h"
#include "Enemies/RLRobotMinionCharacter.h"
#include "Enemies/Components/RLBossSummonMath.h"
#include "Enemies/Components/RLBossSummonVisualComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Player/Components/RLHealthComponent.h"

URLBossSummonComponent::URLBossSummonComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URLBossSummonComponent::Initialize(URLHealthComponent* InHealth)
{
	Health = InHealth;
	Visual = GetOwner()->FindComponentByClass<URLBossSummonVisualComponent>();
	if (!Settings || !Health || !Settings->MinionClass || !Visual || !Settings->MarkerMaterial ||
		!FMath::IsFinite(Settings->InitialDelay) || !FMath::IsFinite(Settings->Interval) ||
		!FMath::IsFinite(Settings->EnragedInterval) || !FMath::IsFinite(Settings->EnragedHealthRatio) ||
		!FMath::IsFinite(Settings->SpawnRadius) || !FMath::IsFinite(Settings->MinPlayerDistance) ||
		!FMath::IsFinite(Settings->TelegraphDuration) || !FMath::IsFinite(Settings->SpawnStagger) || !FMath::IsFinite(Settings->MarkerRadius))
	{
		UE_LOG(LogTemp, Error, TEXT("Boss %s summon settings incomplete; summoning disabled."), *GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
		return;
	}
	Reset();
}

bool URLBossSummonComponent::FindSpawnTransform(FTransform& OutTransform) const
{
	const ARLRobotMinionCharacter* Defaults = Settings->MinionClass.GetDefaultObject();
	if (!Defaults || !GetWorld()) { return false; }
	const UCapsuleComponent* Capsule = Defaults->GetCapsuleComponent();
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FCollisionQueryParams Query(SCENE_QUERY_STAT(BossMinionSpawn), false);
	for (int32 Attempt = 0; Attempt < 12; ++Attempt)
	{
		const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
		FVector Location = GetOwner()->GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) *
			FMath::Max(1.0f, Settings->SpawnRadius);
		if (Navigation)
		{
			FNavLocation Ground;
			if (!Navigation->ProjectPointToNavigation(Location, Ground, FVector(150, 150, 1000))) { continue; }
			Location = Ground.Location;
		}
		else
		{
			FHitResult Ground;
			if (!GetWorld()->LineTraceSingleByChannel(Ground, Location + FVector(0,0,300),
				Location - FVector(0,0,2000), ECC_WorldStatic, Query)) { continue; }
			Location = Ground.ImpactPoint;
		}
		Location.Z += Capsule->GetScaledCapsuleHalfHeight() + 2.0f;
		if (PlayerPawn && FVector::DistSquared2D(Location, PlayerPawn->GetActorLocation()) <
			FMath::Square(FMath::Max(0.0f, Settings->MinPlayerDistance))) { continue; }
		if (GetWorld()->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_Pawn,
			Capsule->GetCollisionShape(), Query)) { continue; }
		const float Separation = Capsule->GetScaledCapsuleRadius() * 2.0f + 20.0f;
		if (PendingMinions.ContainsByPredicate([&Location, Separation](const FPendingMinion& Pending)
		{
			return FVector::DistSquared2D(Location, Pending.Transform.GetLocation()) < FMath::Square(Separation);
		})) { continue; }
		OutTransform = FTransform(FRotator::ZeroRotator, Location);
		return true;
	}
	return false;
}

void URLBossSummonComponent::Summon()
{
	const int32 Count = RLBossSummon::GetSpawnCount(Minions.Num() + PendingMinions.Num(), Settings->MinionsPerSummon, Settings->MaxAliveMinions);
	int32 ReservedCount = 0;
	const auto* Defaults = Settings->MinionClass.GetDefaultObject();
	if (!Defaults) { return; }
	const float HalfHeight = Defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FTransform Transform;
		if (!FindSpawnTransform(Transform)) { continue; }
		FPendingMinion Pending;
		Pending.Transform = Transform;
		Pending.Remaining = RLBossSummon::GetTelegraphDelay(ReservedCount, Settings->TelegraphDuration, Settings->SpawnStagger);
		if (Visual)
		{
			Pending.MarkerIndex = Visual->ShowMarker(Transform.GetLocation() - FVector(0,0,HalfHeight + 2.0f),
				Pending.Remaining, Settings->MarkerRadius, Settings->MarkerMaterial);
		}
		PendingMinions.Add(Pending);
		++ReservedCount;
	}
	if (ReservedCount > 0)
	{
		if (Settings->SummonSound) { UGameplayStatics::PlaySoundAtLocation(this, Settings->SummonSound, GetOwner()->GetActorLocation(), 0.25f, 1.6f); }
		if (Visual)
		{
			const auto* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
			Visual->PlayBossPulse(Enemy->GetMesh(), Settings->PulseMaterial);
			const FVector Ground = GetOwner()->GetActorLocation() - FVector(0,0,Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			Visual->ShowMarker(Ground, 0.25f, Settings->MarkerRadius * 1.5f, Settings->MarkerMaterial, true);
		}
	}
	if (ReservedCount < Count)
	{
		UE_LOG(LogTemp, Warning, TEXT("Boss %s reserved %d/%d minion positions; check navigation and free spawn space."),
			*GetNameSafe(GetOwner()), ReservedCount, Count);
	}
}

void URLBossSummonComponent::UpdatePendingMinions(float DeltaTime)
{
	for (FPendingMinion& Pending : PendingMinions) { Pending.Remaining -= FMath::Max(0.0f, DeltaTime); }
	for (int32 Index = 0; Index < PendingMinions.Num();)
	{
		if (PendingMinions[Index].Remaining > 0.0f) { ++Index; continue; }
		const FPendingMinion Pending = PendingMinions[Index];
		PendingMinions.RemoveAt(Index);
		const auto* Defaults = Settings->MinionClass.GetDefaultObject();
		FCollisionQueryParams Query(SCENE_QUERY_STAT(BossMinionSpawnRecheck), false);
		// Do not silently relocate after the telegraph: cancel an obstructed position.
		if (GetWorld()->OverlapBlockingTestByChannel(Pending.Transform.GetLocation(), FQuat::Identity,
			ECC_Pawn, Defaults->GetCapsuleComponent()->GetCollisionShape(), Query))
		{
			if (Visual) { Visual->HideMarker(Pending.MarkerIndex); }
			continue;
		}
		auto* Pool = GetWorld()->GetSubsystem<URLEnemyPoolSubsystem>();
		auto* Minion = Pool ? Cast<ARLRobotMinionCharacter>(Pool->AcquireEnemy(Settings->MinionClass, Pending.Transform)) : nullptr;
		if (Minion && Minion->IsPoolActive())
		{
			Minion->SetOwner(GetOwner());
			Minions.Add(Minion);
			if (Visual) { Visual->AttachMinion(Pending.MarkerIndex, Minion); }
		}
		else if (Visual) { Visual->HideMarker(Pending.MarkerIndex); }
		// Acquisition can run gameplay callbacks; Reset may invalidate remaining jobs.
		if (!Health || Health->IsDead() || !Cast<ARLEnemyCharacter>(GetOwner())->IsPoolActive()) { Reset(); return; }
	}
}

void URLBossSummonComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const auto* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy || !Enemy->IsPoolActive() || Enemy->IsSpawning() || !Health || Health->IsDead()) { Reset(); return; }
	Minions.RemoveAll([this](const TWeakObjectPtr<ARLRobotMinionCharacter>& Minion)
	{
		return !Minion.IsValid() || !Minion->IsPoolActive() || Minion->GetOwner() != GetOwner();
	});
	UpdatePendingMinions(DeltaTime);
	if (!Enemy->IsPoolActive() || Health->IsDead()) { return; }
	const bool bNowEnraged = RLBossSummon::IsEnraged(Health->GetCurrentHealth(), Health->GetMaxHealth(), Settings->EnragedHealthRatio);
	if (bNowEnraged && !bEnraged) { Remaining = FMath::Min(Remaining, FMath::Max(0.1f, Settings->EnragedInterval)); }
	bEnraged = bNowEnraged;
	Remaining -= FMath::Max(0.0f, DeltaTime);
	if (Remaining <= 0.0f)
	{
		Summon();
		Remaining = FMath::Max(0.1f, bEnraged ? Settings->EnragedInterval : Settings->Interval);
	}
}

void URLBossSummonComponent::Reset()
{
	PendingMinions.Reset();
	if (Visual) { Visual->Reset(); }
	// Clear tracking first: returning enemies can invoke other gameplay callbacks.
	TArray<TWeakObjectPtr<ARLRobotMinionCharacter>> Previous = MoveTemp(Minions);
	Minions.Reset();
	for (const auto& Minion : Previous)
	{
		if (Minion.IsValid() && Minion->IsPoolActive() && Minion->GetOwner() == GetOwner())
		{
			Minion->SetOwner(nullptr);
			Minion->ReturnToPool();
		}
	}
	Remaining = Settings ? FMath::Max(0.1f, Settings->InitialDelay) : 5.0f;
	bEnraged = false;
}

void URLBossSummonComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Reset();
	Super::EndPlay(EndPlayReason);
}
