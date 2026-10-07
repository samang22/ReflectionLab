#include "Combat/RLProjectileContactComponent.h"

#include "Combat/RLProjectile.h"
#include "Combat/RLProjectileSpecialComponent.h"
#include "Combat/RLProjectileRallyComponent.h"
#include "Components/SphereComponent.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/Components/RLBossOverloadComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

URLProjectileContactComponent::URLProjectileContactComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URLProjectileContactComponent::ResolveProjectileContact(AActor* OtherActor)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->IsPoolActive() || Projectile->IsFadingOut() || !IsValid(OtherActor))
	{
		return;
	}

	if (TryDetonateOnPlayerContact(OtherActor))
	{
		return;
	}
	if (auto* Overload = OtherActor->FindComponentByClass<URLBossOverloadComponent>())
	{
		if (Overload->TryAbsorb(Projectile)) { return; }
	}

	if (Projectile->GetRallyComponent()->IsRallyProjectile() && !Projectile->IsReflected() &&
		Projectile->GetRallyComponent()->IsRelayTarget(OtherActor) && OtherActor->IsA<ARLEnemyCharacter>())
	{
		Projectile->GetRallyComponent()->AdvanceRally();
		return;
	}

	if (ShouldIgnoreActor(OtherActor))
	{
		return;
	}

	if (TryExplodeOnEnemyContact(OtherActor))
	{
		return;
	}

	if (TryStopReflectedExplosiveOnContact(OtherActor)) { return; }

	if (Projectile->GetSpecialComponent()->IsExplosive())
	{
		Projectile->GetSpecialComponent()->Explode();
		return;
	}

	ApplyDamageAndReturn(OtherActor);
}

bool URLProjectileContactComponent::TryDetonateOnPlayerContact(AActor* OtherActor)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->GetSpecialComponent()->IsExplosive() || Projectile->IsReflected() || !IsValid(OtherActor))
	{
		return false;
	}

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(Projectile, 0);
	if (OtherActor != PlayerPawn)
	{
		return false;
	}

	return Projectile->GetSpecialComponent()->Detonate();
}

bool URLProjectileContactComponent::TryStopReflectedExplosiveOnContact(AActor* OtherActor)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->GetSpecialComponent()->IsExplosive() || !Projectile->IsReflected() || !IsValid(OtherActor)) { return false; }
	// Enemy contact detonates first; walls keep the restarted fuse running.
	Projectile->PauseMotion();
	Projectile->GetCollisionSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return true;
}

bool URLProjectileContactComponent::TryExplodeOnEnemyContact(AActor* OtherActor)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if ((!Projectile->GetSpecialComponent()->IsExplosive() && !Projectile->GetSpecialComponent()->ExplodesOnEnemyImpact()) || !Projectile->IsReflected() ||
		!IsValid(OtherActor) || !OtherActor->IsA<ARLEnemyCharacter>())
	{
		return false;
	}

	Projectile->GetSpecialComponent()->TriggerExplosion(false, true);
	return true;
}

bool URLProjectileContactComponent::ShouldIgnoreActor(const AActor* OtherActor) const
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!OtherActor || OtherActor == Projectile || OtherActor == Projectile->GetOwner())
	{
		return true;
	}

	const APawn* ProjectileInstigator = Projectile->GetInstigator();
	return ProjectileInstigator &&
		ProjectileInstigator->IsA<ARLEnemyCharacter>() &&
		OtherActor->IsA<ARLEnemyCharacter>();
}

void URLProjectileContactComponent::ApplyDamageAndReturn(AActor* OtherActor)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	ARLEnemyCharacter* HitEnemy = Cast<ARLEnemyCharacter>(OtherActor);
	// Collision callbacks and the arrival fallback may resolve the same target.
	if (Projectile->IsReflected() && Projectile->GetRallyComponent()->IsRallyProjectile() && HitEnemy && Projectile->GetRallyComponent()->HasDamagedEnemy(HitEnemy)) { return; }
	if (Projectile->GetSpecialComponent()->IsGuardProjectile() && Projectile->IsReflected() && HitEnemy)
	{
		// Guard rounds break shields without dealing health damage. All damaging
		// rounds resolve protection once, inside the enemy's TakeDamage path.
		HitEnemy->TryAbsorbReflectedProjectile();
		Projectile->ReturnToPool();
		return;
	}

	const float AppliedDamage = ApplyDamageToActor(OtherActor, Projectile->GetDamageAmount());
	if (Projectile->IsReflected() && Projectile->GetRallyComponent()->IsRallyProjectile() && HitEnemy && AppliedDamage > 0.0f)
	{
		Projectile->GetRallyComponent()->RecordEnemyHit(HitEnemy);
	}

	if (!TryContinueAfterEnemyHit(HitEnemy, AppliedDamage))
	{
		Projectile->ReturnToPool();
	}
}

float URLProjectileContactComponent::ApplyDamageToActor(AActor* TargetActor, float RequestedDamage)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!IsValid(TargetActor)) { return 0.0f; }
	const float AppliedDamage = UGameplayStatics::ApplyDamage(
		TargetActor,
		FMath::Max(0.0f, RequestedDamage),
		Projectile->GetInstigatorController(),
		Projectile,
		UDamageType::StaticClass());
	if (Projectile->IsReflected() && Projectile->GetRallyComponent()->IsRallyProjectile())
	{
		// Keep the actual contact actor: a wall/floor hit can look like an enemy
		// hit on screen, but never enters the enemy damage handler.
		UE_LOG(LogTemp, Display,
			TEXT("[RallyImpact] projectile=%s contact=%s class=%s target=%s definition=%s requested=%.3f applied=%.3f position=%s contactPosition=%s"),
			*Projectile->GetName(), *GetNameSafe(TargetActor), *GetNameSafe(TargetActor->GetClass()),
			*GetNameSafe(Projectile->GetRallyComponent()->GetTargetActor()), *GetNameSafe(Projectile->GetProjectileDefinition()),
			RequestedDamage, AppliedDamage, *Projectile->GetActorLocation().ToString(), *TargetActor->GetActorLocation().ToString());
	}
	return AppliedDamage;
}

bool URLProjectileContactComponent::TryContinueAfterEnemyHit(ARLEnemyCharacter* HitEnemy, float AppliedDamage)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!HitEnemy || !Projectile->TryConsumePierce(AppliedDamage))
	{
		return false;
	}
	if (Projectile->GetRallyComponent()->IsRallyProjectile())
	{
		Projectile->GetRallyComponent()->AdvanceAfterEnemyHit();
	}
	return true;
}
