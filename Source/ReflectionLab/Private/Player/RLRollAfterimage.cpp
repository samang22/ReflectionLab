#include "Player/RLRollAfterimage.h"

#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Player/RLRollAfterimagePoolSubsystem.h"

ARLRollAfterimage::ARLRollAfterimage()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetCanBeDamaged(false);
	PoseMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("PoseMesh"));
	SetRootComponent(PoseMesh);
	PoseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PoseMesh->SetGenerateOverlapEvents(false);
	PoseMesh->SetCanEverAffectNavigation(false);
	PoseMesh->SetCastShadow(false);
	PoseMesh->SetReceivesDecals(false);
	// Translucent ghosts require the fallback skeletal renderer.
	PoseMesh->bDisallowNanite = true;
	PoseMesh->SetComponentTickEnabled(false);
	PoseMesh->PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool ARLRollAfterimage::PrepareResources(USkeletalMeshComponent* Source, UMaterialInterface* Material)
{
	if (!IsValid(Source) || !Source->GetSkeletalMeshAsset() || !IsValid(Material))
	{
		return false;
	}
	const bool bMeshChanged = PoseMesh->GetSkinnedAsset() != Source->GetSkeletalMeshAsset();
	if (bMeshChanged)
	{
		PoseMesh->EmptyOverrideMaterials();
		PoseMesh->SetSkinnedAssetAndUpdate(Source->GetSkeletalMeshAsset());
	}
	const bool bMaterialChanged = !FadeMaterial || CachedMaterial != Material;
	if (bMaterialChanged)
	{
		FadeMaterial = UMaterialInstanceDynamic::Create(Material, this);
		if (!FadeMaterial) { return false; }
		CachedMaterial = Material;
	}
	if (bMeshChanged || bMaterialChanged)
	{
		for (int32 Index = 0; Index < PoseMesh->GetNumMaterials(); ++Index)
		{
			PoseMesh->SetMaterial(Index, FadeMaterial);
		}
	}
	return true;
}

bool ARLRollAfterimage::ActivateFromPool(USkeletalMeshComponent* Source, UMaterialInterface* Material,
	const FLinearColor& Color, float Duration, float Opacity)
{
	if (!FMath::IsFinite(Duration) || Duration <= 0.0f || !FMath::IsFinite(Opacity) ||
		!FMath::IsFinite(Color.R) || !FMath::IsFinite(Color.G) || !FMath::IsFinite(Color.B) ||
		!FMath::IsFinite(Color.A) || !PrepareResources(Source, Material))
	{
		return false;
	}
	SetLifeSpan(0.0f);
	Elapsed = 0.0f;
	Lifetime = FMath::Clamp(Duration, 0.02f, 1.0f);
	InitialOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
	SetOwner(Source->GetOwner());
	SetActorTransform(Source->GetComponentTransform(), false, nullptr, ETeleportType::TeleportPhysics);
	PoseMesh->CopyPoseFromSkeletalComponent(Source);
	PoseMesh->RefreshBoneTransforms();
	FadeMaterial->SetVectorParameterValue(TEXT("ProjectileColor"), Color);
	FadeMaterial->SetScalarParameterValue(TEXT("EmissiveIntensity"), 2.0f);
	FadeMaterial->SetScalarParameterValue(TEXT("FadeOpacity"), InitialOpacity);
	bIsPoolActive = true;
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
	return true;
}

void ARLRollAfterimage::DeactivateForPool()
{
	bIsPoolActive = false;
	SetLifeSpan(0.0f);
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	PoseMesh->SetComponentTickEnabled(false);
	SetOwner(nullptr);
	Elapsed = 0.0f;
	if (FadeMaterial)
	{
		FadeMaterial->SetScalarParameterValue(TEXT("FadeOpacity"), 0.0f);
		FadeMaterial->SetScalarParameterValue(TEXT("EmissiveIntensity"), 0.0f);
	}
}

void ARLRollAfterimage::ReturnToPool()
{
	if (!bIsPoolActive) { return; }
	if (UWorld* World = GetWorld())
	{
		if (URLRollAfterimagePoolSubsystem* Pool = World->GetSubsystem<URLRollAfterimagePoolSubsystem>())
		{
			Pool->ReleaseAfterimage(this);
			return;
		}
	}
	Destroy();
}

void ARLRollAfterimage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bIsPoolActive) { return; }
	Elapsed += FMath::Max(0.0f, DeltaSeconds);
	const float Remaining = 1.0f - FMath::Clamp(Elapsed / Lifetime, 0.0f, 1.0f);
	if (FadeMaterial)
	{
		FadeMaterial->SetScalarParameterValue(TEXT("FadeOpacity"), InitialOpacity * Remaining);
		FadeMaterial->SetScalarParameterValue(TEXT("EmissiveIntensity"), 2.0f * Remaining);
	}
	if (Remaining <= 0.0f) { ReturnToPool(); }
}
