#include "Combat/RLProjectilePoolSubsystem.h"

#include "Combat/RLProjectile.h"
#include "Engine/World.h"

void URLProjectilePoolSubsystem::PrewarmPool(
	TSubclassOf<ARLProjectile> ProjectileClass,
	int32 DesiredInactiveCount)
{
	if (!ProjectileClass || DesiredInactiveCount <= 0)
	{
		return;
	}

	FRLProjectilePoolBucket& Pool = ProjectilePools.FindOrAdd(ProjectileClass);
	Pool.InactiveProjectiles.RemoveAll(
		[](const TObjectPtr<ARLProjectile>& Projectile)
		{
			return !IsValid(Projectile);
		});

	while (Pool.InactiveProjectiles.Num() < DesiredInactiveCount)
	{
		ARLProjectile* Projectile = SpawnProjectile(ProjectileClass);
		if (!Projectile)
		{
			break;
		}

		Pool.InactiveProjectiles.Add(Projectile);
	}
}

ARLProjectile* URLProjectilePoolSubsystem::AcquireProjectile(
	TSubclassOf<ARLProjectile> ProjectileClass,
	const FTransform& SpawnTransform,
	AActor* NewOwner,
	APawn* NewInstigator)
{
	if (!ProjectileClass)
	{
		return nullptr;
	}

	FRLProjectilePoolBucket& Pool = ProjectilePools.FindOrAdd(ProjectileClass);
	ARLProjectile* Projectile = nullptr;

	while (Pool.InactiveProjectiles.Num() > 0 && !IsValid(Projectile))
	{
		Projectile = Pool.InactiveProjectiles.Pop(EAllowShrinking::No);
	}

	if (!Projectile)
	{
		Projectile = SpawnProjectile(ProjectileClass);
	}

	if (Projectile)
	{
		Projectile->ActivateProjectile(SpawnTransform, NewOwner, NewInstigator);
	}

	return Projectile;
}

void URLProjectilePoolSubsystem::ReleaseProjectile(ARLProjectile* Projectile)
{
	if (!IsValid(Projectile))
	{
		return;
	}

	Projectile->DeactivateForPool();
	ProjectilePools.FindOrAdd(Projectile->GetClass()).InactiveProjectiles.AddUnique(Projectile);
}

ARLProjectile* URLProjectilePoolSubsystem::SpawnProjectile(
	TSubclassOf<ARLProjectile> ProjectileClass)
{
	UWorld* World = GetWorld();
	if (!World || !ProjectileClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ARLProjectile* Projectile = World->SpawnActor<ARLProjectile>(
		ProjectileClass,
		FTransform::Identity,
		SpawnParameters);

	if (Projectile)
	{
		Projectile->DeactivateForPool();
	}

	return Projectile;
}
