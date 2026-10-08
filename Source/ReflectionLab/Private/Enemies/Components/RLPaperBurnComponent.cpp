#include "Enemies/Components/RLPaperBurnComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Data/RLPaperBurnDataAsset.h"
#include "Materials/MaterialInstanceDynamic.h"

URLPaperBurnComponent::URLPaperBurnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool URLPaperBurnComponent::Start(USkeletalMeshComponent* Mesh, const TArray<UActorComponent*>& ComponentsToSuspend)
{
	if (bPlaying) { return true; }
	if (!Mesh || !Settings || !Settings->Material || !FMath::IsFinite(Settings->Duration) || Settings->Duration <= 0.0f ||
		!FMath::IsFinite(Settings->EdgeWidth) || Mesh->GetNumMaterials() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Paper burn settings invalid on %s; using immediate death."), *GetNameSafe(GetOwner()));
		return false;
	}
	BurnMaterial = UMaterialInstanceDynamic::Create(Settings->Material, this);
	if (!BurnMaterial) { return false; }
	TargetMesh = Mesh;
	const FBoxSphereBounds Bounds = Mesh->CalcBounds(Mesh->GetComponentTransform());
	BurnMaterial->SetScalarParameterValue(TEXT("BurnBottom"), Bounds.Origin.Z - Bounds.BoxExtent.Z);
	BurnMaterial->SetScalarParameterValue(TEXT("BurnHeight"), FMath::Max(1.0f, Bounds.BoxExtent.Z * 2.0f));
	BurnMaterial->SetScalarParameterValue(TEXT("BurnProgress"), 0.0f);
	BurnMaterial->SetScalarParameterValue(TEXT("BurnEdgeWidth"), FMath::Clamp(Settings->EdgeWidth, 0.001f, 0.2f));
	BurnMaterial->SetVectorParameterValue(TEXT("BurnEdgeColor"), Settings->EdgeColor);
	for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
	{
		OriginalMaterials.Add(Mesh->GetMaterial(Index));
		Mesh->SetMaterial(Index, BurnMaterial);
	}
	bPreviousPauseAnims = Mesh->bPauseAnims;
	Mesh->bPauseAnims = true;
	for (UActorComponent* Component : ComponentsToSuspend)
	{
		if (IsValid(Component) && Component != this && Component->IsComponentTickEnabled())
		{
			SuspendedComponents.AddUnique(Component);
			Component->SetComponentTickEnabled(false);
		}
	}
	Elapsed = 0.0f;
	bPlaying = true;
	SetComponentTickEnabled(true);
	return true;
}

void URLPaperBurnComponent::Cancel()
{
	bPlaying = false;
	SetComponentTickEnabled(false);
	if (IsValid(TargetMesh))
	{
		for (int32 Index = 0; Index < OriginalMaterials.Num(); ++Index) { TargetMesh->SetMaterial(Index, OriginalMaterials[Index]); }
		TargetMesh->bPauseAnims = bPreviousPauseAnims;
	}
	for (const auto& Component : SuspendedComponents)
	{
		if (Component.IsValid()) { Component->SetComponentTickEnabled(true); }
	}
	SuspendedComponents.Reset();
	OriginalMaterials.Reset();
	TargetMesh = nullptr;
	BurnMaterial = nullptr;
	Elapsed = 0.0f;
}

void URLPaperBurnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bPlaying) { return; }
	Elapsed += FMath::Max(0.0f, DeltaTime);
	const float Progress = FMath::Clamp(Elapsed / FMath::Max(0.1f, Settings->Duration), 0.0f, 1.0f);
	BurnMaterial->SetScalarParameterValue(TEXT("BurnProgress"), Progress);
	if (Progress >= 1.0f)
	{
		Cancel();
		OnFinished.ExecuteIfBound();
	}
}

void URLPaperBurnComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Cancel();
	OnFinished.Unbind();
	Super::EndPlay(EndPlayReason);
}
