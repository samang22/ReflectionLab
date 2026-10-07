#include "Enemies/Components/RLBossSummonVisualComponent.h"

#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"

URLBossSummonVisualComponent::URLBossSummonVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

int32 URLBossSummonVisualComponent::ShowMarker(const FVector& GroundLocation, float Duration,
	float Radius, UMaterialInterface* Material, bool bPulseOnly)
{
	if (!Material || !GetOwner()) { return INDEX_NONE; }
	int32 Index = Markers.IndexOfByPredicate([](const FRLSummonMarker& Entry) { return !Entry.bActive; });
	if (Index == INDEX_NONE)
	{
		// At most ten reserved minions and one boss pulse.
		if (Markers.Num() >= 11) { return INDEX_NONE; }
		Index = Markers.AddDefaulted();
		Markers[Index].Decal = NewObject<UDecalComponent>(GetOwner());
		Markers[Index].Decal->SetVisibility(false);
		Markers[Index].Decal->RegisterComponent();
	}
	FRLSummonMarker& Entry = Markers[Index];
	if (!Entry.Material || Entry.SourceMaterial != Material)
	{
		Entry.Material = UMaterialInstanceDynamic::Create(Material, this);
		Entry.SourceMaterial = Material;
	}
	if (!Entry.Material) { return INDEX_NONE; }
	Entry.Elapsed = 0.0f;
	Entry.Duration = FMath::Max(0.1f, Duration);
	Entry.FadeElapsed = 0.0f;
	Entry.bActive = true;
	Entry.bAttached = false;
	Entry.bPulseOnly = bPulseOnly;
	Entry.Minion.Reset();
	Entry.Decal->SetDecalMaterial(Entry.Material);
	Entry.Decal->SetWorldLocation(GroundLocation + FVector(0, 0, 30));
	Entry.Decal->SetWorldRotation(FRotator(-90, 0, 0));
	Entry.Decal->DecalSize = FVector(64, FMath::Max(1.0f, Radius), FMath::Max(1.0f, Radius));
	Entry.Decal->FadeScreenSize = 0.0f;
	Entry.Decal->SetSortOrder(18);
	Entry.Material->SetScalarParameterValue(TEXT("MarkerProgress"), 0.0f);
	Entry.Material->SetScalarParameterValue(TEXT("MarkerOpacity"), 1.0f);
	Entry.Decal->SetVisibility(true);
	SetComponentTickEnabled(true);
	return Index;
}

void URLBossSummonVisualComponent::AttachMinion(int32 MarkerIndex, ARLEnemyCharacter* Minion)
{
	if (!Markers.IsValidIndex(MarkerIndex)) { return; }
	Markers[MarkerIndex].Minion = Minion;
	Markers[MarkerIndex].bAttached = true;
}

void URLBossSummonVisualComponent::HideMarker(int32 MarkerIndex)
{
	if (!Markers.IsValidIndex(MarkerIndex)) { return; }
	FRLSummonMarker& Entry = Markers[MarkerIndex];
	Entry.bActive = false;
	Entry.Minion.Reset();
	if (Entry.Decal) { Entry.Decal->SetVisibility(false); }
}

void URLBossSummonVisualComponent::Reset()
{
	RestorePulse();
	for (int32 Index = 0; Index < Markers.Num(); ++Index) { HideMarker(Index); }
	SetComponentTickEnabled(false);
}

void URLBossSummonVisualComponent::PlayBossPulse(USkeletalMeshComponent* Mesh, UMaterialInterface* Material)
{
	RestorePulse();
	if (!Mesh || !Material) { return; }
	if (!PulseMaterial || PulseSourceMaterial != Material)
	{
		PulseMaterial = UMaterialInstanceDynamic::Create(Material, this);
		PulseSourceMaterial = Material;
	}
	if (!PulseMaterial) { return; }
	PulseMesh = Mesh;
	PulseElapsed = 0.0f;
	PulseMaterial->SetScalarParameterValue(TEXT("SpawnProgress"), 1.1f);
	PulseMaterial->SetScalarParameterValue(TEXT("SpawnBottom"), Mesh->Bounds.Origin.Z - Mesh->Bounds.BoxExtent.Z);
	PulseMaterial->SetScalarParameterValue(TEXT("SpawnHeight"), FMath::Max(1.0f, Mesh->Bounds.BoxExtent.Z * 2.0f));
	PulseMaterial->SetVectorParameterValue(TEXT("SpawnColor"), FLinearColor(0.02f, 0.8f, 1.0f));
	PulseMaterial->SetScalarParameterValue(TEXT("SpawnFlash"), 0.3f);
	for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
	{
		PulseOriginalMaterials.Add(Mesh->GetMaterial(Index));
		Mesh->SetMaterial(Index, PulseMaterial);
	}
	SetComponentTickEnabled(true);
}

void URLBossSummonVisualComponent::RestorePulse()
{
	if (IsValid(PulseMesh))
	{
		for (int32 Index = 0; Index < PulseOriginalMaterials.Num(); ++Index)
		{
			PulseMesh->SetMaterial(Index, PulseOriginalMaterials[Index]);
		}
	}
	PulseOriginalMaterials.Reset();
	PulseMesh = nullptr;
}

void URLBossSummonVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (PulseMesh)
	{
		PulseElapsed += FMath::Max(0.0f, DeltaTime);
		PulseMaterial->SetScalarParameterValue(TEXT("SpawnFlash"), 0.3f * FMath::Max(0.0f, 1.0f - PulseElapsed / 0.12f));
		if (PulseElapsed >= 0.12f) { RestorePulse(); }
	}
	bool bAnyActive = false;
	for (int32 Index = 0; Index < Markers.Num(); ++Index)
	{
		FRLSummonMarker& Entry = Markers[Index];
		if (!Entry.bActive) { continue; }
		Entry.Elapsed += FMath::Max(0.0f, DeltaTime);
		Entry.Material->SetScalarParameterValue(TEXT("MarkerProgress"), FMath::Clamp(Entry.Elapsed / Entry.Duration, 0.0f, 1.0f));
		const bool bFinished = Entry.bPulseOnly ? Entry.Elapsed >= Entry.Duration : Entry.bAttached &&
			(!Entry.Minion.IsValid() || !Entry.Minion->IsPoolActive() || Entry.Minion->GetOwner() != GetOwner() || !Entry.Minion->IsSpawning());
		if (bFinished)
		{
			Entry.FadeElapsed += FMath::Max(0.0f, DeltaTime);
			Entry.Material->SetScalarParameterValue(TEXT("MarkerOpacity"), FMath::Max(0.0f, 1.0f - Entry.FadeElapsed / 0.12f));
			if (Entry.FadeElapsed >= 0.12f) { HideMarker(Index); continue; }
		}
		bAnyActive = true;
	}
	SetComponentTickEnabled(bAnyActive || PulseMesh != nullptr);
}

void URLBossSummonVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Reset();
	Super::EndPlay(EndPlayReason);
}
