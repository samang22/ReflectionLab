#include "Player/RLRollAfterimagePoolSubsystem.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Player/RLRollAfterimage.h"

void URLRollAfterimagePoolSubsystem::RemoveInvalidEntries()
{
	AllAfterimages.RemoveAll([](const ARLRollAfterimage* Image) { return !IsValid(Image); });
	InactiveAfterimages.RemoveAll([](const ARLRollAfterimage* Image) { return !IsValid(Image); });
}

void URLRollAfterimagePoolSubsystem::PrewarmPool(int32 DesiredCount,
	USkeletalMeshComponent* Source, UMaterialInterface* Material)
{
	if (bShuttingDown || !IsValid(Source) || Source->GetWorld() != GetWorld() ||
		!Source->GetSkeletalMeshAsset() || !IsValid(Material) || DesiredCount <= 0)
	{
		return;
	}
	RemoveInvalidEntries();
	for (ARLRollAfterimage* Image : InactiveAfterimages)
	{
		if (!Image->PrepareResources(Source, Material)) { return; }
	}
	while (AllAfterimages.Num() < DesiredCount)
	{
		ARLRollAfterimage* Image = SpawnAfterimage();
		if (!Image) { break; }
		InactiveAfterimages.Add(Image);
		if (!Image->PrepareResources(Source, Material)) { break; }
	}
}

ARLRollAfterimage* URLRollAfterimagePoolSubsystem::AcquireAfterimage(
	USkeletalMeshComponent* Source, UMaterialInterface* Material,
	const FLinearColor& Color, float Duration, float Opacity)
{
	if (bShuttingDown || !IsValid(Source) || Source->GetWorld() != GetWorld() ||
		!Source->GetSkeletalMeshAsset() || !IsValid(Material) ||
		!FMath::IsFinite(Duration) || Duration <= 0.0f || !FMath::IsFinite(Opacity))
	{
		return nullptr;
	}
	RemoveInvalidEntries();
	ARLRollAfterimage* Image = InactiveAfterimages.IsEmpty()
		? SpawnAfterimage() : InactiveAfterimages.Pop(EAllowShrinking::No).Get();
	if (!Image) { return nullptr; }
	if (!Image->ActivateFromPool(Source, Material, Color, Duration, Opacity))
	{
		Image->DeactivateForPool();
		InactiveAfterimages.AddUnique(Image);
		return nullptr;
	}
	return Image;
}

void URLRollAfterimagePoolSubsystem::ReleaseAfterimage(ARLRollAfterimage* Afterimage)
{
	if (bShuttingDown || !IsValid(Afterimage) || Afterimage->GetWorld() != GetWorld() ||
		!AllAfterimages.Contains(Afterimage) || !Afterimage->IsPoolActive())
	{
		return;
	}
	Afterimage->DeactivateForPool();
	InactiveAfterimages.AddUnique(Afterimage);
}

ARLRollAfterimage* URLRollAfterimagePoolSubsystem::SpawnAfterimage()
{
	UWorld* World = GetWorld();
	if (!World || bShuttingDown) { return nullptr; }
	FActorSpawnParameters Parameters;
	Parameters.ObjectFlags |= RF_Transient;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARLRollAfterimage* Image = World->SpawnActor<ARLRollAfterimage>(
		ARLRollAfterimage::StaticClass(), FTransform::Identity, Parameters);
	if (Image)
	{
		Image->DeactivateForPool();
		AllAfterimages.Add(Image);
	}
	return Image;
}

void URLRollAfterimagePoolSubsystem::Deinitialize()
{
	bShuttingDown = true;
	for (ARLRollAfterimage* Image : AllAfterimages)
	{
		if (IsValid(Image) && !Image->IsActorBeingDestroyed())
		{
			Image->DeactivateForPool();
			Image->Destroy();
		}
	}
	InactiveAfterimages.Reset();
	AllAfterimages.Reset();
	Super::Deinitialize();
}
