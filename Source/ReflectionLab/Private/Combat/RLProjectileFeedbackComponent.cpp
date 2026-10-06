#include "Combat/RLProjectileFeedbackComponent.h"

#include "Combat/RLExplosionVisual.h"
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

void URLProjectileFeedbackComponent::PlayPoolReturnEffect(UStaticMeshComponent* Visual)
{
	if (bReturnFeedbackHandled) { return; }
	bReturnFeedbackHandled = true;
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || !Visual || !Visual->GetStaticMesh() || !ShardMesh) { return; }
	UMaterialInterface* SourceMaterial = Visual->GetMaterial(0);
	if (!SourceMaterial) { return; }
	const FVector Center = Visual->GetComponentTransform().TransformPosition(Visual->GetStaticMesh()->GetBounds().Origin);
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARLExplosionVisual* Effect = World->SpawnActor<ARLExplosionVisual>(
		ARLExplosionVisual::StaticClass(), Center - FVector(0.0f, 0.0f, 18.0f), FRotator::ZeroRotator, Parameters);
	if (!Effect) { return; }
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SourceMaterial->GetMaterial(), Effect);
	if (!Material) { Effect->Destroy(); return; }
	Material->CopyMaterialUniformParameters(SourceMaterial);
	Material->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
	Material->SetScalarParameterValue(TEXT("FadeOpacity"), 1.0f);
	Material->SetScalarParameterValue(TEXT("EmissiveIntensity"), 18.0f);
	Effect->ConfigureProjectileDissolve();
	Effect->Initialize(ShardMesh, Material, FVector(0.22f, 0.07f, 0.07f), 50.0f, FLinearColor::White, false);
	UE_LOG(LogTemp, Display, TEXT("[ProjectilePoolReturnEffect] %s at %s"), *GetNameSafe(GetOwner()), *Center.ToCompactString());
}
