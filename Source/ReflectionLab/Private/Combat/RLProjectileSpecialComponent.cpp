#include "Combat/RLProjectileSpecialComponent.h"
#include "Combat/RLProjectileExplosionComponent.h"
#include "Combat/RLProjectile.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Combat/RLProjectileVisualComponent.h"
#include "Combat/RLProjectileRallyComponent.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

URLProjectileSpecialComponent::URLProjectileSpecialComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<USoundBase> ExplosionSoundFinder(
		TEXT("/Game/ReflectionLab/Audio/SFX/Combat/Explosion/"
			 "SFX_ExplosiveDetonation.SFX_ExplosiveDetonation"));
	if (ExplosionSoundFinder.Succeeded())
	{
		Settings.ExplosionSound = ExplosionSoundFinder.Object;
	}
}

void URLProjectileSpecialComponent::ApplyDefinitionSettings(const URLProjectileDefinitionDataAsset& Definition)
{
	Explosion()->ApplyDefinitionSettings(Definition);

	Settings.FakeTriggerDistance = FMath::Max(1.0f, Definition.FakeTriggerDistance);
	Settings.FakeRevealDelay = FMath::Max(0.0f, Definition.FakeRevealDelay);
	Settings.FakeRealSpeedMultiplier = FMath::Max(0.1f, Definition.FakeRealSpeedMultiplier);
	Settings.FakeColor = Definition.FakeColor;
	Settings.FakeOpacity = FMath::Clamp(Definition.FakeOpacity, 0.0f, 1.0f);
	Settings.GuardColor = Definition.GuardColor;
	Settings.ParrySplitColor = Definition.ParrySplitColor;
	Settings.ParrySplitFragmentCount = FMath::Clamp(Definition.ParrySplitFragmentCount, 1, 6);
	Settings.ParrySplitSpreadAngle = FMath::Clamp(Definition.ParrySplitSpreadAngle, 0.0f, 180.0f);
	Settings.ParrySplitFragmentSpeedMultiplier = FMath::Max(0.1f, Definition.ParrySplitFragmentSpeedMultiplier);
	Settings.ParrySplitFragmentScale = FMath::Clamp(Definition.ParrySplitFragmentScale, 0.1f, 1.0f);
}

void URLProjectileSpecialComponent::InitializeFeedback()
{
	Explosion()->SetDefaults(Settings);
	Explosion()->InitializeFeedback();
}

void URLProjectileSpecialComponent::ResetRuntimeState()
{
	Explosion()->ResetRuntimeState();
	bIsFakeProjectile = false;
	bFakeDormant = false;
	bIsGuardProjectile = false;
	bSplitsOnParry = false;
	FakeDormantElapsedTime = 0;
	FakeTarget.Reset();
}

void URLProjectileSpecialComponent::ConfigureAsExplosive()
{
	if (!CastChecked<ARLProjectile>(GetOwner())->IsPoolActive()) { return; }
	bIsFakeProjectile = false; bIsGuardProjectile = false; bSplitsOnParry = false;
	Explosion()->ConfigureAsExplosive();
}

void URLProjectileSpecialComponent::ConfigureAsDelayedExplosive(AActor* TargetActor)
{
	if (!CastChecked<ARLProjectile>(GetOwner())->IsPoolActive() || !IsValid(TargetActor)) { return; }
	bIsFakeProjectile = false; bIsGuardProjectile = false; bSplitsOnParry = false;
	Explosion()->ConfigureAsDelayedExplosive(TargetActor);
}

void URLProjectileSpecialComponent::ConfigureAsFake(AActor* TargetActor)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->IsPoolActive() || !IsValid(TargetActor))
	{
		return;
	}

	Projectile->SetParryEnabled(false);
	Explosion()->DisableBehavior();
	bIsFakeProjectile = true;
	bFakeDormant = false;
	bIsGuardProjectile = false;
	bSplitsOnParry = false;
	Projectile->GetRallyComponent()->DisableRally();
	FakeDormantElapsedTime = 0.0f;
	FakeTarget = TargetActor;
	Projectile->GetVisualComponent()->UpdateProjectileMaterial();
	Projectile->SetActorTickEnabled(true);
}

void URLProjectileSpecialComponent::ConfigureAsGuard()
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->IsPoolActive())
	{
		return;
	}

	Projectile->SetParryEnabled(true);
	Explosion()->DisableBehavior();
	bIsFakeProjectile = false;
	bIsGuardProjectile = true;
	bSplitsOnParry = false;
	Projectile->GetRallyComponent()->DisableRally();
	Projectile->GetVisualComponent()->UpdateProjectileMaterial();
}

void URLProjectileSpecialComponent::ConfigureAsParrySplit()
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->IsPoolActive())
	{
		return;
	}

	Projectile->SetParryEnabled(true);
	Explosion()->DisableBehavior();
	bIsFakeProjectile = false;
	bIsGuardProjectile = false;
	bSplitsOnParry = true;
	Projectile->GetRallyComponent()->DisableRally();
	Projectile->GetVisualComponent()->UpdateProjectileMaterial();
}

bool URLProjectileSpecialComponent::Detonate()
{
	return Explosion()->Detonate();
}

void URLProjectileSpecialComponent::UpdateExplosive(float DeltaTime)
{
	Explosion()->UpdateExplosive(DeltaTime);
}

void URLProjectileSpecialComponent::UpdateDelayedExplosive(float DeltaTime)
{
	Explosion()->UpdateDelayedExplosive(DeltaTime);
}

void URLProjectileSpecialComponent::UpdateFake(float DeltaTime)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	AActor* TargetActor = FakeTarget.Get();
	if (!IsValid(TargetActor))
	{
		Projectile->ReturnToPool();
		return;
	}

	if (!bFakeDormant)
	{
		if (FVector::DistSquared2D(Projectile->GetActorLocation(), TargetActor->GetActorLocation()) >
			FMath::Square(FMath::Max(1.0f, Settings.FakeTriggerDistance)))
		{
			return;
		}

		bFakeDormant = true;
		FakeDormantElapsedTime = 0.0f;
		Projectile->PauseMotion();
		Projectile->GetCollisionSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Projectile->GetVisualMesh()->SetVisibility(false, true);
		return;
	}

	FakeDormantElapsedTime += FMath::Max(0.0f, DeltaTime);
	if (FakeDormantElapsedTime >= FMath::Max(0.0f, Settings.FakeRevealDelay))
	{
		RevealFakeProjectile();
	}
}

void URLProjectileSpecialComponent::RevealFakeProjectile()
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	AActor* TargetActor = FakeTarget.Get();
	if (!IsValid(TargetActor))
	{
		Projectile->ReturnToPool();
		return;
	}

	bIsFakeProjectile = false;
	bFakeDormant = false;
	Projectile->SetParryEnabled(true);
	FakeDormantElapsedTime = 0.0f;
	Projectile->GetVisualMesh()->SetVisibility(true, true);
	Projectile->GetCollisionSphere()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Projectile->GetVisualComponent()->UpdateProjectileMaterial();
	const FVector RealShotDirection =
		(TargetActor->GetActorLocation() - Projectile->GetActorLocation()).GetSafeNormal();
	Projectile->SetProjectileSpeed(
		Projectile->GetBaseSpeed() * FMath::Max(0.1f, Settings.FakeRealSpeedMultiplier),
		RealShotDirection);
	Projectile->RestartLifetime(FMath::Max(0.1f, Projectile->GetLifetimeSeconds()));
	FakeTarget.Reset();
}

void URLProjectileSpecialComponent::SpawnParrySplitFragments(
	AActor* OriginalOwner,
	APawn* OriginalInstigator,
	AActor* PlayerActor, const FGuid& ParryAttemptId)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	UWorld* World = Projectile->GetWorld();
	if (!World || !IsValid(PlayerActor))
	{
		return;
	}

	URLProjectilePoolSubsystem* PoolSubsystem =
		World->GetSubsystem<URLProjectilePoolSubsystem>();
	if (!PoolSubsystem)
	{
		return;
	}

	const int32 FragmentCount = FMath::Clamp(Settings.ParrySplitFragmentCount, 1, 6);
	const FVector DirectionToPlayer =
		(PlayerActor->GetActorLocation() - Projectile->GetActorLocation()).GetSafeNormal2D();
	if (DirectionToPlayer.IsNearlyZero())
	{
		return;
	}

	for (int32 FragmentIndex = 0; FragmentIndex < FragmentCount; ++FragmentIndex)
	{
		const float FragmentAlpha = FragmentCount > 1
			? static_cast<float>(FragmentIndex) / static_cast<float>(FragmentCount - 1)
			: 0.5f;
		const float FragmentAngle = FMath::Lerp(
			-Settings.ParrySplitSpreadAngle * 0.5f,
			Settings.ParrySplitSpreadAngle * 0.5f,
			FragmentAlpha);
		const FVector FragmentDirection = DirectionToPlayer.RotateAngleAxis(
			FragmentAngle,
			FVector::UpVector);
		const FTransform FragmentTransform(
			FragmentDirection.Rotation(),
			Projectile->GetActorLocation() + FragmentDirection * (Projectile->GetDefaultCollisionRadius() + 4.0f));
		if (ARLProjectile* Fragment = PoolSubsystem->AcquireProjectile(
			Projectile->GetClass(),
			FragmentTransform,
			OriginalOwner,
			OriginalInstigator))
		{
			Fragment->InitializeFromDefinition(Projectile->GetProjectileDefinition(), nullptr, false);
			Fragment->ExcludeParryAttempt(ParryAttemptId);
			Fragment->GetSpecialComponent()->ConfigureAsSplitFragment(
				Settings.ParrySplitFragmentSpeedMultiplier,
				Settings.ParrySplitFragmentScale);
		}
	}
}

void URLProjectileSpecialComponent::ConfigureAsSplitFragment(float SpeedMultiplier, float VisualScale)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	const float SafeScale = FMath::Clamp(VisualScale, 0.1f, 1.0f);
	Projectile->GetVisualMesh()->SetRelativeScale3D(Projectile->GetDefaultVisualScale() * SafeScale);
	Projectile->GetCollisionSphere()->SetSphereRadius(Projectile->GetDefaultCollisionRadius() * SafeScale, true);
	Projectile->SetProjectileSpeed(
		Projectile->GetBaseSpeed() * FMath::Max(0.1f, SpeedMultiplier),
		Projectile->GetActorForwardVector());
}

void URLProjectileSpecialComponent::Explode()
{
	Explosion()->Explode();
}

void URLProjectileSpecialComponent::TriggerExplosion(bool bDamagePlayer, bool bDamageEnemies)
{
	Explosion()->TriggerExplosion(bDamagePlayer, bDamageEnemies);
}

float URLProjectileSpecialComponent::GetFuseRemainingSeconds() const
{
	return Explosion()->GetFuseRemainingSeconds();
}

float URLProjectileSpecialComponent::CalculateReturnSpeed(float RequestedMultiplier, float DefaultMultiplier) const
{
	return Explosion()->CalculateReturnSpeed(RequestedMultiplier, DefaultMultiplier);
}

void URLProjectileSpecialComponent::CalculateReturnScales(
	float RequestedScale, float& VisualScale, float& CollisionScale) const
{
	Explosion()->CalculateReturnScales(RequestedScale, VisualScale, CollisionScale);
}

bool URLProjectileSpecialComponent::GetVisualStyle(FLinearColor& Color, float& Emissive, float& Opacity) const
{
	Opacity = 1.0f;
	if (bIsGuardProjectile)
	{
		Color = Settings.GuardColor;
		Emissive = 18.0f;
	}
	else if (bSplitsOnParry)
	{
		Color = Settings.ParrySplitColor;
		Emissive = 22.0f;
	}
	else if (bIsFakeProjectile)
	{
		Color = Settings.FakeColor;
		Emissive = 5.0f;
		Opacity = Settings.FakeOpacity;
	}
	else
	{
		return false;
	}
	return true;
}

void URLProjectileSpecialComponent::DisableBehavior()
{
	Explosion()->DisableBehavior();
	bIsFakeProjectile = false;
	bIsGuardProjectile = false;
	bSplitsOnParry = false;
}

void URLProjectileSpecialComponent::PrepareForReflection()
{
	Explosion()->PrepareForReflection();
	bSplitsOnParry = false;
}

void URLProjectileSpecialComponent::RestartReflectedFuse()
{
	Explosion()->RestartReflectedFuse();
}

URLProjectileExplosionComponent* URLProjectileSpecialComponent::Explosion() const
{
	return CastChecked<ARLProjectile>(GetOwner())->GetExplosionComponent();
}
bool URLProjectileSpecialComponent::IsExplosive() const { return Explosion()->IsExplosive(); }
bool URLProjectileSpecialComponent::IsDelayedExplosive() const { return Explosion()->IsDelayedExplosive(); }
bool URLProjectileSpecialComponent::ExplodesOnEnemyImpact() const { return Explosion()->ExplodesOnEnemyImpact(); }
float URLProjectileSpecialComponent::GetExplosionRadius() const { return Explosion()->GetExplosionRadius(); }
