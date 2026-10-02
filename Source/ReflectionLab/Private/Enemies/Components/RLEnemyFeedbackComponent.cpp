#include "Enemies/Components/RLEnemyFeedbackComponent.h"

#include "Animation/RLEnemyAnimInstance.h"
#include "Combat/RLExplosionVisual.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"

URLEnemyFeedbackComponent::URLEnemyFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URLEnemyFeedbackComponent::Configure(const FRLEnemyFeedbackSettings& NewSettings)
{
	Settings = NewSettings;
}

void URLEnemyFeedbackComponent::PlayHitSound() const
{
	if (!GetOwner() || !Settings.HitSound || !FMath::IsFinite(Settings.HitSoundVolume) ||
		!FMath::IsFinite(Settings.HitSoundPitchMin) || !FMath::IsFinite(Settings.HitSoundPitchMax))
	{
		return;
	}
	UGameplayStatics::PlaySoundAtLocation(this, Settings.HitSound, GetOwner()->GetActorLocation(),
		FMath::Max(0.0f, Settings.HitSoundVolume), FMath::FRandRange(
			FMath::Max(0.1f, FMath::Min(Settings.HitSoundPitchMin, Settings.HitSoundPitchMax)),
			FMath::Max(0.1f, FMath::Max(Settings.HitSoundPitchMin, Settings.HitSoundPitchMax))));
}

void URLEnemyFeedbackComponent::PlayShootAnimation() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Character && Character->GetMesh())
	{
		if (URLEnemyAnimInstance* Anim = Cast<URLEnemyAnimInstance>(Character->GetMesh()->GetAnimInstance()))
		{
			Anim->PlayShootAnimation();
		}
	}
}

void URLEnemyFeedbackComponent::ResetCombatAnimation() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Character && Character->GetMesh())
	{
		if (URLEnemyAnimInstance* Anim = Cast<URLEnemyAnimInstance>(Character->GetMesh()->GetAnimInstance()))
		{
			Anim->ResetCombatAnimation();
		}
	}
}

void URLEnemyFeedbackComponent::SpawnDeathEffect() const
{
	UWorld* World = GetWorld();
	if (!World || !GetOwner() || !Settings.DeathEffectShardMesh || !Settings.DeathEffectShardMaterial)
	{
		return;
	}

	FVector GroundLocation = GetOwner()->GetActorLocation();
	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyDeathEffectGroundTrace), false, GetOwner());
	const FVector TraceStart = GroundLocation + FVector(0.0f, 0.0f, 200.0f);
	const FVector TraceEnd = GroundLocation - FVector(0.0f, 0.0f, 600.0f);
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
	ARLExplosionVisual* DeathEffect = World->SpawnActor<ARLExplosionVisual>(
		ARLExplosionVisual::StaticClass(),
		GroundLocation,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (DeathEffect)
	{
		DeathEffect->Initialize(
			Settings.DeathEffectShardMesh,
			Settings.DeathEffectShardMaterial,
			Settings.DeathEffectShardScale,
			FMath::Max(1.0f, Settings.DeathEffectRadius),
			Settings.DeathEffectColor,
			false);
	}
}

