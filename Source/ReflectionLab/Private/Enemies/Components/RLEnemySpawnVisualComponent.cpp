#include "Enemies/Components/RLEnemySpawnVisualComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

URLEnemySpawnVisualComponent::URLEnemySpawnVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool URLEnemySpawnVisualComponent::StartSpawn(USkeletalMeshComponent* Mesh)
{
	CancelSpawn();
	if (!Mesh || !SpawnMaterial || !FMath::IsFinite(SpawnDuration) || SpawnDuration <= 0.0f || Mesh->GetNumMaterials() == 0)
	{
		return false;
	}
	SpawnInstance = UMaterialInstanceDynamic::Create(SpawnMaterial, this);
	if (!SpawnInstance) { return false; }
	TargetMesh = Mesh;
	const FBoxSphereBounds Bounds = Mesh->CalcBounds(Mesh->GetComponentTransform());
	SpawnInstance->SetScalarParameterValue(TEXT("SpawnBottom"), Bounds.Origin.Z - Bounds.BoxExtent.Z);
	SpawnInstance->SetScalarParameterValue(TEXT("SpawnHeight"), FMath::Max(1.0f, Bounds.BoxExtent.Z * 2.0f));
	SpawnInstance->SetScalarParameterValue(TEXT("SpawnProgress"), 0.0f);
	if (bOverrideSpawnColor) { SpawnInstance->SetVectorParameterValue(TEXT("SpawnColor"), SpawnColor); }
	SpawnInstance->SetScalarParameterValue(TEXT("SpawnFlash"), 0.0f);
	for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
	{
		OriginalMaterials.Add(Mesh->GetMaterial(Index));
		Mesh->SetMaterial(Index, SpawnInstance);
	}
	Elapsed = 0.0f;
	bSpawning = true;
	SetComponentTickEnabled(true);
	return true;
}

void URLEnemySpawnVisualComponent::CancelSpawn()
{
	bSpawning = false;
	SetComponentTickEnabled(false);
	if (IsValid(TargetMesh))
	{
		for (int32 Index = 0; Index < OriginalMaterials.Num(); ++Index)
		{
			TargetMesh->SetMaterial(Index, OriginalMaterials[Index]);
		}
	}
	OriginalMaterials.Reset();
	TargetMesh = nullptr;
	SpawnInstance = nullptr;
}

void URLEnemySpawnVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bSpawning) { return; }
	Elapsed += FMath::Max(0.0f, DeltaTime);
	const float Progress = FMath::Clamp(Elapsed / FMath::Max(SpawnDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	if (SpawnInstance) { SpawnInstance->SetScalarParameterValue(TEXT("SpawnProgress"), Progress); }
	if (Progress >= 1.0f)
	{
		const float FlashDuration = FMath::IsFinite(CompletionFlashDuration) ? FMath::Max(0.0f, CompletionFlashDuration) : 0.0f;
		if (FlashDuration > 0.0f && Elapsed < SpawnDuration + FlashDuration)
		{
			if (SpawnInstance) { SpawnInstance->SetScalarParameterValue(TEXT("SpawnProgress"), 1.1f); }
			if (SpawnInstance) { SpawnInstance->SetScalarParameterValue(TEXT("SpawnFlash"),
				FMath::Clamp(1.0f - (Elapsed - SpawnDuration) / FlashDuration, 0.0f, 1.0f)); }
			return;
		}
		CancelSpawn();
		OnSpawnFinished.Broadcast();
	}
}
