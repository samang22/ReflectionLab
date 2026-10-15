#include "Combat/RLRingAttackPoolSubsystem.h"
#include "Combat/RLExpandingRingAttack.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ARLExpandingRingAttack* URLRingAttackPoolSubsystem::AcquireRing(const FTransform& Transform,
	AActor* NewOwner, APawn* NewInstigator, URLRingAttackDataAsset* Data, bool bAutoStart)
{
	ARLExpandingRingAttack* Ring = nullptr;
	while (!InactiveRings.IsEmpty() && !IsValid(Ring))
	{
		Ring = InactiveRings.Pop(EAllowShrinking::No);
	}
	if (!Ring)
	{
		Ring = GetWorld()->SpawnActorDeferred<ARLExpandingRingAttack>(ARLExpandingRingAttack::StaticClass(),
			Transform, NewOwner, NewInstigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Ring) { return nullptr; }
		Ring->bAutoStart = false;
		UGameplayStatics::FinishSpawningActor(Ring, Transform);
	}
	Ring->ActivateFromPool(Transform, NewOwner, NewInstigator, Data, bAutoStart);
	return Ring->IsPoolActive() ? Ring : nullptr;
}

void URLRingAttackPoolSubsystem::ReleaseRing(ARLExpandingRingAttack* Ring)
{
	if (!IsValid(Ring) || !Ring->IsPoolActive()) { return; }
	Ring->DeactivateForPool();
	InactiveRings.AddUnique(Ring);
}
