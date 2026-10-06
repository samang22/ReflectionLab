#include "Combat/RLProjectileSpecialComponent.h"

#include "Combat/RLProjectile.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Combat/RLProjectileVisualComponent.h"
#include "Combat/RLProjectileRallyComponent.h"
#include "Combat/RLProjectileContactComponent.h"
#include "Combat/RLExplosionVisual.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Combat/RLVFXPoolSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundConcurrency.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName SpecialProjectileColorParameterName(TEXT("ProjectileColor"));
	const FName SpecialEmissiveIntensityParameterName(TEXT("EmissiveIntensity"));
}

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
	bExplodesOnEnemyImpact = Definition.bExplodesOnEnemyImpact;

	Settings.ExplosiveSpeedMultiplier = FMath::Clamp(Definition.ExplosiveSpeedMultiplier, 0.1f, 1.0f);
	Settings.ExplosiveVisualScale = FMath::Max(1.0f, Definition.ExplosiveVisualScale);
	Settings.ExplosiveFuseDuration = FMath::Max(0.1f, Definition.ExplosiveFuseDuration);
	const URLProjectileDefinitionDataAsset& ExplosionDefinition =
		Definition.EnemyImpactExplosionDefinition
			? *Definition.EnemyImpactExplosionDefinition
			: Definition;
	Settings.ExplosionRadius = FMath::Max(1.0f, ExplosionDefinition.ExplosionRadius);
	ReflectedExplosionRadius = FMath::Max(1.0f, ExplosionDefinition.ReflectedExplosionRadius);
	Settings.ExplosionDamage = FMath::Max(0.0f, ExplosionDefinition.ExplosionDamage);
	Settings.ExplosionSound = ExplosionDefinition.ExplosionSound;
	Settings.ExplosionSoundVolume = FMath::Clamp(
		ExplosionDefinition.ExplosionSoundVolume,
		0.0f,
		2.0f);
	ExplosionVFX = ExplosionDefinition.ExplosionVFX;
	ExplosionVFXScale = FMath::Clamp(ExplosionDefinition.ExplosionVFXScale, 0.1f, 5.0f);
	Settings.ExplosiveBlinkStartInterval = FMath::Max(0.01f, Definition.ExplosiveBlinkStartInterval);
	Settings.ExplosiveBlinkEndInterval = FMath::Max(0.01f, Definition.ExplosiveBlinkEndInterval);
	Settings.ExplosiveBaseColor = Definition.ExplosiveBaseColor;
	Settings.ExplosiveDecalColor = ExplosionDefinition.ExplosiveDecalColor;
	Settings.ExplosiveWarningColor = Definition.ExplosiveWarningColor;
	Settings.ExplosiveBaseEmissiveIntensity = FMath::Max(0.0f, Definition.ExplosiveBaseEmissiveIntensity);
	Settings.ExplosiveWarningEmissiveIntensity = FMath::Max(0.0f, Definition.ExplosiveWarningEmissiveIntensity);

	Settings.DelayedExplosiveTriggerDistance = FMath::Max(1.0f, Definition.DelayedExplosiveTriggerDistance);
	Settings.DelayedExplosivePauseDuration = FMath::Max(0.0f, Definition.DelayedExplosivePauseDuration);
	Settings.DelayedExplosiveResumeSpeedMultiplier = FMath::Max(0.1f, Definition.DelayedExplosiveResumeSpeedMultiplier);
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
	static USoundConcurrency* SharedExplosionSoundConcurrency = nullptr;
	if (!SharedExplosionSoundConcurrency)
	{
		SharedExplosionSoundConcurrency = NewObject<USoundConcurrency>(
			GetTransientPackage(), TEXT("RLExplosionSoundConcurrency"));
		if (!SharedExplosionSoundConcurrency)
		{
			return;
		}
		SharedExplosionSoundConcurrency->AddToRoot();
		SharedExplosionSoundConcurrency->Concurrency.MaxCount = 3;
		SharedExplosionSoundConcurrency->Concurrency.bLimitToOwner = false;
		SharedExplosionSoundConcurrency->Concurrency.ResolutionRule =
			EMaxConcurrentResolutionRule::StopQuietest;
		SharedExplosionSoundConcurrency->Concurrency.RetriggerTime = 0.06f;
	}
	ExplosionSoundConcurrency = SharedExplosionSoundConcurrency;
}

void URLProjectileSpecialComponent::ResetRuntimeState()
{
	bIsExplosive = false;
	bExplosiveBlinkWarning = false;
	bIsDelayedExplosive = false;
	bDelayedExplosivePaused = false;
	bDelayedExplosiveResumed = false;
	bIsFakeProjectile = false;
	bFakeDormant = false;
	bExplodesOnEnemyImpact = false;
	bIsGuardProjectile = false;
	bSplitsOnParry = false;
	ExplosiveElapsedTime = 0.0f;
	ExplosiveNextBlinkTime = 0.0f;
	DelayedExplosivePauseElapsedTime = 0.0f;
	FakeDormantElapsedTime = 0.0f;
	DelayedExplosiveTarget.Reset();
	FakeTarget.Reset();
	ExplosiveMaterialInstance = nullptr;
	ExplosionVFX = nullptr;
	ExplosionVFXScale = 1.0f;
}

void URLProjectileSpecialComponent::ConfigureAsExplosive()
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->IsPoolActive())
	{
		return;
	}

	Projectile->SetParryEnabled(true);
	bIsExplosive = true;
	bIsDelayedExplosive = false;
	bIsFakeProjectile = false;
	bIsGuardProjectile = false;
	bSplitsOnParry = false;
	Projectile->GetRallyComponent()->DisableRally(true);
	ExplosiveElapsedTime = 0.0f;
	ExplosiveNextBlinkTime = FMath::Max(0.01f, Settings.ExplosiveBlinkStartInterval);
	bExplosiveBlinkWarning = false;
	Projectile->GetVisualMesh()->SetVisibility(true, true);
	Projectile->GetVisualMesh()->SetRelativeScale3D(
		Projectile->GetDefaultVisualScale() * FMath::Max(1.0f, Settings.ExplosiveVisualScale));
	Projectile->GetCollisionSphere()->SetGenerateOverlapEvents(true);
	Projectile->GetCollisionSphere()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Projectile->GetCollisionSphere()->SetSphereRadius(
		Projectile->GetDefaultCollisionRadius() * FMath::Sqrt(FMath::Max(1.0f, Settings.ExplosiveVisualScale)),
		false);
	ExplosiveMaterialInstance = Projectile->GetVisualMesh()->CreateDynamicMaterialInstance(0);
	ApplyExplosiveBlinkColor(false);
	Projectile->SetProjectileSpeed(
		Projectile->GetBaseSpeed() * FMath::Clamp(Settings.ExplosiveSpeedMultiplier, 0.1f, 1.0f),
		Projectile->GetActorForwardVector());
	Projectile->RestartLifetime(FMath::Max(Projectile->GetLifetimeSeconds(), Settings.ExplosiveFuseDuration + 0.5f));
	Projectile->SetActorTickEnabled(true);
}

void URLProjectileSpecialComponent::ConfigureAsDelayedExplosive(AActor* TargetActor)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->IsPoolActive() || !IsValid(TargetActor))
	{
		return;
	}

	ConfigureAsExplosive();
	bIsDelayedExplosive = true;
	bDelayedExplosivePaused = false;
	bDelayedExplosiveResumed = false;
	DelayedExplosivePauseElapsedTime = 0.0f;
	DelayedExplosiveTarget = TargetActor;
}

void URLProjectileSpecialComponent::ConfigureAsFake(AActor* TargetActor)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->IsPoolActive() || !IsValid(TargetActor))
	{
		return;
	}

	Projectile->SetParryEnabled(false);
	bIsExplosive = false;
	bIsDelayedExplosive = false;
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
	bIsExplosive = false;
	bIsDelayedExplosive = false;
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
	bIsExplosive = false;
	bIsDelayedExplosive = false;
	bIsFakeProjectile = false;
	bIsGuardProjectile = false;
	bSplitsOnParry = true;
	Projectile->GetRallyComponent()->DisableRally();
	Projectile->GetVisualComponent()->UpdateProjectileMaterial();
}

bool URLProjectileSpecialComponent::Detonate()
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->IsPoolActive() || !bIsExplosive)
	{
		return false;
	}

	Explode();
	return true;
}

void URLProjectileSpecialComponent::UpdateExplosive(float DeltaTime)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	ExplosiveElapsedTime += FMath::Max(0.0f, DeltaTime);
	const float FuseDuration = FMath::Max(0.1f, Settings.ExplosiveFuseDuration);
	if (ExplosiveElapsedTime >= FuseDuration)
	{
		Explode();
		return;
	}

	if (ExplosiveElapsedTime < ExplosiveNextBlinkTime)
	{
		return;
	}

	bExplosiveBlinkWarning = !bExplosiveBlinkWarning;
	ApplyExplosiveBlinkColor(bExplosiveBlinkWarning);
	const float FuseAlpha = FMath::Clamp(ExplosiveElapsedTime / FuseDuration, 0.0f, 1.0f);
	const float BlinkInterval = FMath::Lerp(
		FMath::Max(0.01f, Settings.ExplosiveBlinkStartInterval),
		FMath::Max(0.01f, Settings.ExplosiveBlinkEndInterval),
		FuseAlpha * FuseAlpha);
	ExplosiveNextBlinkTime = ExplosiveElapsedTime + BlinkInterval;
}

void URLProjectileSpecialComponent::ApplyExplosiveBlinkColor(bool bUseWarningColor)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!ExplosiveMaterialInstance)
	{
		return;
	}

	ExplosiveMaterialInstance->SetVectorParameterValue(
		SpecialProjectileColorParameterName,
		Projectile->IsReflected()
			? (bUseWarningColor ? FLinearColor::White : FLinearColor(0.1f, 0.85f, 1.0f))
			: (bUseWarningColor ? Settings.ExplosiveWarningColor : Settings.ExplosiveBaseColor));
	ExplosiveMaterialInstance->SetScalarParameterValue(
		SpecialEmissiveIntensityParameterName,
		bUseWarningColor
			? FMath::Max(0.0f, Settings.ExplosiveWarningEmissiveIntensity)
			: FMath::Max(0.0f, Settings.ExplosiveBaseEmissiveIntensity));
}

void URLProjectileSpecialComponent::UpdateDelayedExplosive(float DeltaTime)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!bIsDelayedExplosive || bDelayedExplosiveResumed)
	{
		return;
	}

	AActor* TargetActor = DelayedExplosiveTarget.Get();
	if (!IsValid(TargetActor))
	{
		bIsDelayedExplosive = false;
		return;
	}

	if (!bDelayedExplosivePaused)
	{
		if (FVector::DistSquared2D(Projectile->GetActorLocation(), TargetActor->GetActorLocation()) >
			FMath::Square(FMath::Max(1.0f, Settings.DelayedExplosiveTriggerDistance)))
		{
			return;
		}

		bDelayedExplosivePaused = true;
		DelayedExplosivePauseElapsedTime = 0.0f;
		Projectile->PauseMotion();
		return;
	}

	DelayedExplosivePauseElapsedTime += FMath::Max(0.0f, DeltaTime);
	if (DelayedExplosivePauseElapsedTime < FMath::Max(0.0f, Settings.DelayedExplosivePauseDuration))
	{
		return;
	}

	bDelayedExplosivePaused = false;
	bDelayedExplosiveResumed = true;
	const FVector ResumeDirection =
		(TargetActor->GetActorLocation() - Projectile->GetActorLocation()).GetSafeNormal();
	Projectile->SetProjectileSpeed(
		Projectile->GetBaseSpeed() * FMath::Max(0.1f, Settings.DelayedExplosiveResumeSpeedMultiplier),
		ResumeDirection);
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
	AActor* PlayerActor)
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
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	TriggerExplosion(!Projectile->IsReflected(), Projectile->IsReflected());
}

void URLProjectileSpecialComponent::TriggerExplosion(bool bDamagePlayer, bool bDamageEnemies)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->IsPoolActive() || Projectile->IsFadingOut())
	{
		return;
	}

	const FVector ExplosionLocation = Projectile->GetActorLocation();
	const float BlastRadius = FMath::Max(1.0f, Settings.ExplosionRadius);
	if (Settings.ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			Projectile,
			Settings.ExplosionSound,
			ExplosionLocation,
			FRotator::ZeroRotator,
			FMath::Max(0.0f, Settings.ExplosionSoundVolume),
			1.0f,
			0.0f,
			nullptr,
			ExplosionSoundConcurrency,
			Projectile);
	}
	if (ExplosionVFX)
	{
		if (URLVFXPoolSubsystem* VFXPool = Projectile->GetWorld()->GetSubsystem<URLVFXPoolSubsystem>())
		{
			VFXPool->PlaySystemAtLocation(
				ExplosionVFX,
				ExplosionLocation,
				FRotator::ZeroRotator,
				FVector(ExplosionVFXScale));
		}
	}
	SpawnExplosionVisual(ExplosionLocation, BlastRadius);
	Projectile->NotifyExplosion(ExplosionLocation, BlastRadius);
	ApplyExplosionDamage(ExplosionLocation, BlastRadius, bDamagePlayer, bDamageEnemies);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Projectile explosion at %s with radius %.1f. PlayerDamage=%s EnemyDamage=%s"),
		*ExplosionLocation.ToCompactString(),
		BlastRadius,
		bDamagePlayer ? TEXT("true") : TEXT("false"),
		bDamageEnemies ? TEXT("true") : TEXT("false"));
	Projectile->ReturnToPool();
}

void URLProjectileSpecialComponent::ApplyExplosionDamage(
	const FVector& ExplosionLocation,
	float BlastRadius,
	bool bDamagePlayer,
	bool bDamageEnemies)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (bDamagePlayer)
	{
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(Projectile, 0))
		{
			if (FVector::DistSquared(PlayerPawn->GetActorLocation(), ExplosionLocation) <=
				FMath::Square(BlastRadius))
			{
				Projectile->GetContactComponent()->ApplyDamageToActor(PlayerPawn, Settings.ExplosionDamage);
			}
		}
	}

	if (bDamageEnemies)
	{
		for (TActorIterator<ARLEnemyCharacter> Iterator(Projectile->GetWorld()); Iterator; ++Iterator)
		{
			ARLEnemyCharacter* Enemy = *Iterator;
			if (!IsValid(Enemy) || !Enemy->IsPoolActive() ||
				FVector::DistSquared(Enemy->GetActorLocation(), ExplosionLocation) >
				FMath::Square(BlastRadius))
			{
				continue;
			}

			Projectile->GetContactComponent()->ApplyDamageToActor(Enemy, Settings.ExplosionDamage);
		}
	}
}

void URLProjectileSpecialComponent::SpawnExplosionVisual(
	const FVector& ExplosionLocation,
	float BlastRadius)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	UWorld* World = Projectile->GetWorld();
	if (!World || !Projectile->GetVisualMesh() || !Projectile->GetVisualMesh()->GetStaticMesh())
	{
		return;
	}

	FVector GroundLocation = ExplosionLocation;
	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ExplosionVisualGroundTrace), false, Projectile);
	const FVector TraceStart = ExplosionLocation + FVector(0.0f, 0.0f, 200.0f);
	const FVector TraceEnd = ExplosionLocation - FVector(0.0f, 0.0f, 600.0f);
	if (World->LineTraceSingleByChannel(
		GroundHit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams))
	{
		GroundLocation = GroundHit.ImpactPoint;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARLExplosionVisual* ExplosionVisual = World->SpawnActor<ARLExplosionVisual>(
		ARLExplosionVisual::StaticClass(),
		GroundLocation,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!ExplosionVisual)
	{
		return;
	}

	UMaterialInterface* ShardMaterial = Projectile->GetVisualComponent()->GetReflectedMaterial()
		? Projectile->GetVisualComponent()->GetReflectedMaterial()
		: Projectile->GetVisualMesh()->GetMaterial(0);
	ExplosionVisual->Initialize(
		Projectile->GetVisualMesh()->GetStaticMesh(),
		ShardMaterial,
		Projectile->GetDefaultVisualScale(),
		BlastRadius,
		Projectile->IsReflected() ? FLinearColor(0.1f, 0.85f, 1.0f) : Settings.ExplosiveDecalColor);
}

float URLProjectileSpecialComponent::GetFuseRemainingSeconds() const
{
	return bIsExplosive ? FMath::Max(0.0f, Settings.ExplosiveFuseDuration - ExplosiveElapsedTime) : 0.0f;
}

float URLProjectileSpecialComponent::CalculateReturnSpeed(float RequestedMultiplier, float DefaultMultiplier) const
{
	const ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	const float Multiplier = RequestedMultiplier > 0.0f
		? RequestedMultiplier : (bIsExplosive ? 1.0f : DefaultMultiplier);
	const float BaseSpeed = bIsExplosive
		? Projectile->GetBaseSpeed() * FMath::Clamp(Settings.ExplosiveSpeedMultiplier, 0.1f, 1.0f)
		: Projectile->GetBaseSpeed();
	return BaseSpeed * FMath::Max(1.0f, Multiplier);
}

void URLProjectileSpecialComponent::CalculateReturnScales(
	float RequestedScale, float& VisualScale, float& CollisionScale) const
{
	// A returned bomb retains its large silhouette and smaller collision volume.
	VisualScale = bIsExplosive ? FMath::Max(RequestedScale, Settings.ExplosiveVisualScale) : RequestedScale;
	CollisionScale = bIsExplosive
		? FMath::Max(RequestedScale, FMath::Sqrt(FMath::Max(1.0f, Settings.ExplosiveVisualScale)))
		: RequestedScale;
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
	bIsExplosive = false;
	bIsDelayedExplosive = false;
	bIsFakeProjectile = false;
	bIsGuardProjectile = false;
	bSplitsOnParry = false;
}

void URLProjectileSpecialComponent::PrepareForReflection()
{
	if (bIsExplosive)
	{
		Settings.ExplosionRadius = ReflectedExplosionRadius;
	}
	bSplitsOnParry = false;
}

void URLProjectileSpecialComponent::RestartReflectedFuse()
{
	if (!bIsExplosive)
	{
		return;
	}
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	// Delayed variants must not resume toward the old player target after a parry.
	bIsDelayedExplosive = false;
	bDelayedExplosivePaused = false;
	bDelayedExplosiveResumed = false;
	DelayedExplosiveTarget.Reset();
	DelayedExplosivePauseElapsedTime = 0.0f;
	ExplosiveElapsedTime = 0.0f;
	ExplosiveNextBlinkTime = FMath::Max(0.01f, Settings.ExplosiveBlinkStartInterval);
	bExplosiveBlinkWarning = false;
	ExplosiveMaterialInstance = Projectile->GetVisualMesh()->CreateDynamicMaterialInstance(0);
	ApplyExplosiveBlinkColor(false);
}
