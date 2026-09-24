#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RLProjectilePoolSubsystem.generated.h"

class ARLProjectile;

USTRUCT()
struct FRLProjectilePoolBucket
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<ARLProjectile>> InactiveProjectiles;
};

UCLASS()
class REFLECTIONLAB_API URLProjectilePoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Projectile Pool")
	void PrewarmPool(TSubclassOf<ARLProjectile> ProjectileClass, int32 DesiredInactiveCount);

	UFUNCTION(BlueprintCallable, Category = "Projectile Pool")
	ARLProjectile* AcquireProjectile(
		TSubclassOf<ARLProjectile> ProjectileClass,
		const FTransform& SpawnTransform,
		AActor* NewOwner,
		APawn* NewInstigator);

	void ReleaseProjectile(ARLProjectile* Projectile);

private:
	ARLProjectile* SpawnProjectile(TSubclassOf<ARLProjectile> ProjectileClass);

	UPROPERTY(Transient)
	TMap<TSubclassOf<ARLProjectile>, FRLProjectilePoolBucket> ProjectilePools;
};
