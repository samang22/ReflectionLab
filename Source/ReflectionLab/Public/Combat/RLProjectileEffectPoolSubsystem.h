#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RLProjectileEffectPoolSubsystem.generated.h"

class ARLExplosionVisual;
class UStaticMesh;
class UMaterialInterface;

// Only projectile-return shards belong here; explosion/death visuals retain their own lifecycle.
UCLASS()
class REFLECTIONLAB_API URLProjectileEffectPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	ARLExplosionVisual* PlayEffect(const FVector& Location, UStaticMesh* Mesh, UMaterialInterface* Material);
	void ReleaseEffect(ARLExplosionVisual* Effect);
private:
	UPROPERTY(Transient) TArray<TObjectPtr<ARLExplosionVisual>> Effects;
	UPROPERTY(Transient) TArray<TObjectPtr<ARLExplosionVisual>> InactiveEffects;
};
