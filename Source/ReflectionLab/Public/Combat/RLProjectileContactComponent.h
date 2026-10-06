#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLProjectileContactComponent.generated.h"

class ARLEnemyCharacter;

// Centralizes contact priority, damage and piercing for ARLProjectile.
UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLProjectileContactComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLProjectileContactComponent();
	void ResolveProjectileContact(AActor* OtherActor);
	float ApplyDamageToActor(AActor* TargetActor, float RequestedDamage);

private:
	bool TryDetonateOnPlayerContact(AActor* OtherActor);
	bool TryStopReflectedExplosiveOnContact(AActor* OtherActor);
	bool TryExplodeOnEnemyContact(AActor* OtherActor);
	bool ShouldIgnoreActor(const AActor* OtherActor) const;
	void ApplyDamageAndReturn(AActor* OtherActor);
	bool TryContinueAfterEnemyHit(ARLEnemyCharacter* HitEnemy, float AppliedDamage);
};
