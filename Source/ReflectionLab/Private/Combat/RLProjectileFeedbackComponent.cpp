#include "Combat/RLProjectileFeedbackComponent.h"

#include "Combat/RLExplosionVisual.h"
#include "Combat/RLProjectileEffectPoolSubsystem.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

URLProjectileFeedbackComponent::URLProjectileFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	ShardMesh = MeshFinder.Object;
}

void URLProjectileFeedbackComponent::ApplyDefinitionSettings(const URLProjectileDefinitionDataAsset& Definition)
{
	PoolReturnSound = Definition.PoolReturnSound;
	PoolReturnSoundVolume = FMath::IsFinite(Definition.PoolReturnSoundVolume)
		? FMath::Clamp(Definition.PoolReturnSoundVolume, 0.0f, 1.0f) : 0.0f;
}

void URLProjectileFeedbackComponent::PlayPoolReturnEffect(UStaticMeshComponent* Visual)
{
	if (bReturnFeedbackHandled) { return; }
	bReturnFeedbackHandled = true;
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown) { return; }
	if (PoolReturnSound && PoolReturnSoundVolume > 0.0f)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PoolReturnSound, GetOwner()->GetActorLocation(),
			PoolReturnSoundVolume, FMath::FRandRange(0.95f, 1.05f));
	}
	if (!Visual || !Visual->GetStaticMesh() || !ShardMesh) { return; }
	UMaterialInterface* SourceMaterial = Visual->GetMaterial(0);
	if (!SourceMaterial) { return; }
	const FVector Center = Visual->GetComponentTransform().TransformPosition(Visual->GetStaticMesh()->GetBounds().Origin);
	auto* Pool = World->GetSubsystem<URLProjectileEffectPoolSubsystem>();
	if (!Pool || !Pool->PlayEffect(Center - FVector(0.0f, 0.0f, 18.0f), ShardMesh, SourceMaterial)) { return; }
	UE_LOG(LogTemp, Display, TEXT("[ProjectilePoolReturnEffect] %s at %s"), *GetNameSafe(GetOwner()), *Center.ToCompactString());
}
