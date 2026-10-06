#include "Combat/RLProjectileVisualComponent.h"

#include "Combat/RLProjectile.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Combat/RLProjectileSpecialComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

namespace
{
	const FName VisualProjectileColorParameterName(TEXT("ProjectileColor"));
	const FName VisualEmissiveIntensityParameterName(TEXT("EmissiveIntensity"));
	const FName VisualFadeOpacityParameterName(TEXT("FadeOpacity"));
}

URLProjectileVisualComponent::URLProjectileVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URLProjectileVisualComponent::ApplyDefinitionSettings(const URLProjectileDefinitionDataAsset& Definition)
{
	Settings.FadeOutDuration = FMath::Max(0.0f, Definition.FadeOutDuration);
	Settings.HostileMaterial = Definition.HostileMaterial;
	Settings.ReflectedMaterial = Definition.ReflectedMaterial;
	ReflectedTrailVFX = Definition.ReflectedTrailVFX;
	ReflectedTrailScale = FMath::Clamp(Definition.ReflectedTrailScale, 0.1f, 5.0f);
}

void URLProjectileVisualComponent::ResetRuntimeState()
{
	ResetReflectedAfterimages();
	DeactivateReflectedTrail();
	SpecialMaterialInstance = nullptr;
	FadeMaterialInstance = nullptr;
	FadeOutElapsedTime = 0.0f;
	ReflectedTrailVFX = nullptr;
	ReflectedTrailScale = 1.0f;
}

void URLProjectileVisualComponent::CreateReflectedAfterimages()
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	ReflectedAfterimageMeshes.Reset();
	const int32 AfterimageCount = FMath::Clamp(Settings.ReflectedAfterimageCount, 1, 8);
	for (int32 AfterimageIndex = 0; AfterimageIndex < AfterimageCount; ++AfterimageIndex)
	{
		const FName ComponentName(*FString::Printf(
			TEXT("ReflectedAfterimage_%d"),
			AfterimageIndex));
		UStaticMeshComponent* AfterimageMesh = NewObject<UStaticMeshComponent>(Projectile, ComponentName);
		if (!AfterimageMesh)
		{
			continue;
		}

		AfterimageMesh->SetupAttachment(Projectile->GetCollisionSphere());
		AfterimageMesh->SetAbsolute(true, true, true);
		AfterimageMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AfterimageMesh->SetCanEverAffectNavigation(false);
		AfterimageMesh->SetCastShadow(false);
		AfterimageMesh->SetReceivesDecals(false);
		AfterimageMesh->bDisallowNanite = true;
		AfterimageMesh->SetStaticMesh(Projectile->GetVisualMesh()->GetStaticMesh());
		if (Settings.ReflectedMaterial)
		{
			AfterimageMesh->SetMaterial(0, Settings.ReflectedMaterial);
		}
		AfterimageMesh->SetVisibility(false, true);
		AfterimageMesh->RegisterComponent();
		ReflectedAfterimageMeshes.Add(AfterimageMesh);
	}
}

void URLProjectileVisualComponent::ResetReflectedAfterimages()
{
	ReflectedAfterimageHistory.Reset();
	ReflectedAfterimageSampleAccumulator = 0.0f;
	for (UStaticMeshComponent* AfterimageMesh : ReflectedAfterimageMeshes)
	{
		if (AfterimageMesh)
		{
			AfterimageMesh->SetVisibility(false, true);
		}
	}
}

void URLProjectileVisualComponent::UpdateReflectedAfterimages(float DeltaTime)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->GetVisualMesh() || ReflectedAfterimageMeshes.IsEmpty())
	{
		return;
	}

	ReflectedAfterimageSampleAccumulator += DeltaTime;
	const float SampleInterval = FMath::Max(0.01f, Settings.ReflectedAfterimageSampleInterval);
	if (ReflectedAfterimageSampleAccumulator < SampleInterval)
	{
		return;
	}

	ReflectedAfterimageSampleAccumulator = FMath::Fmod(
		ReflectedAfterimageSampleAccumulator,
		SampleInterval);
	ReflectedAfterimageHistory.Insert(Projectile->GetVisualMesh()->GetComponentTransform(), 0);
	ReflectedAfterimageHistory.SetNum(
		FMath::Min(
			ReflectedAfterimageHistory.Num(),
			ReflectedAfterimageMeshes.Num() + 1),
		EAllowShrinking::No);

	for (int32 AfterimageIndex = 0;
		AfterimageIndex < ReflectedAfterimageMeshes.Num();
		++AfterimageIndex)
	{
		UStaticMeshComponent* AfterimageMesh = ReflectedAfterimageMeshes[AfterimageIndex];
		const int32 HistoryIndex = AfterimageIndex + 1;
		if (!AfterimageMesh || !ReflectedAfterimageHistory.IsValidIndex(HistoryIndex))
		{
			if (AfterimageMesh)
			{
				AfterimageMesh->SetVisibility(false, true);
			}
			continue;
		}

		FTransform AfterimageTransform = ReflectedAfterimageHistory[HistoryIndex];
		const UStaticMesh* AfterimageStaticMesh = AfterimageMesh->GetStaticMesh();
		const FVector LocalMeshCenter = AfterimageStaticMesh
			? AfterimageStaticMesh->GetBounds().Origin
			: FVector::ZeroVector;
		const FVector WorldMeshCenter = AfterimageTransform.TransformPosition(LocalMeshCenter);
		const float ScaleMultiplier = FMath::Max(
			0.35f,
			1.0f - Settings.ReflectedAfterimageScaleFalloff * static_cast<float>(AfterimageIndex + 1));
		AfterimageTransform.SetScale3D(
			AfterimageTransform.GetScale3D() * ScaleMultiplier);
		// Keep the visual center fixed even when the mesh pivot is off-center.
		AfterimageTransform.AddToTranslation(
			WorldMeshCenter - AfterimageTransform.TransformPosition(LocalMeshCenter));
		AfterimageMesh->SetWorldTransform(
			AfterimageTransform,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		AfterimageMesh->SetVisibility(true, true);
	}
}

void URLProjectileVisualComponent::UpdateProjectileMaterial()
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	UMaterialInterface* Material = Projectile->IsReflected()
		? Settings.ReflectedMaterial.Get()
		: Settings.HostileMaterial.Get();

	if (Projectile->GetVisualMesh() && Material)
	{
		Projectile->GetVisualMesh()->SetMaterial(0, Material);
	}
	SpecialMaterialInstance = nullptr;
	FLinearColor SpecialColor;
	float SpecialEmissive;
	float SpecialOpacity;
	if (Projectile->GetSpecialComponent()->GetVisualStyle(SpecialColor, SpecialEmissive, SpecialOpacity))
	{
		ApplySpecialColor(SpecialColor, SpecialEmissive, SpecialOpacity);
	}

	for (UStaticMeshComponent* AfterimageMesh : ReflectedAfterimageMeshes)
	{
		if (AfterimageMesh && Projectile->GetVisualMesh())
		{
			AfterimageMesh->SetMaterial(0, Projectile->GetVisualMesh()->GetMaterial(0));
		}
	}
}

void URLProjectileVisualComponent::ActivateReflectedTrail(float VisualScaleMultiplier)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->GetTrailComponent() || !ReflectedTrailVFX)
	{
		return;
	}

	Projectile->GetTrailComponent()->SetAsset(ReflectedTrailVFX);
	Projectile->GetTrailComponent()->SetRelativeScale3D(
		FVector(ReflectedTrailScale * FMath::Max(0.1f, VisualScaleMultiplier)));
	UpdateReflectedTrailLocation();
	Projectile->GetTrailComponent()->Activate(true);
}

void URLProjectileVisualComponent::UpdateReflectedTrailLocation()
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->GetTrailComponent() || !ReflectedTrailVFX || !Projectile->GetVisualMesh())
	{
		return;
	}

	const UStaticMesh* VisualMesh = Projectile->GetVisualMesh()->GetStaticMesh();
	const FVector LocalMeshCenter = VisualMesh ? VisualMesh->GetBounds().Origin : FVector::ZeroVector;
	// Match the afterimages' visual center, not the collision root or mesh pivot.
	Projectile->GetTrailComponent()->SetWorldLocation(
		Projectile->GetVisualMesh()->GetComponentTransform().TransformPosition(LocalMeshCenter));
}

void URLProjectileVisualComponent::DeactivateReflectedTrail()
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->GetTrailComponent())
	{
		return;
	}

	Projectile->GetTrailComponent()->Deactivate();
	Projectile->GetTrailComponent()->SetAsset(nullptr);
}

void URLProjectileVisualComponent::ApplySpecialColor(
	const FLinearColor& Color,
	float EmissiveIntensity,
	float Opacity)
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	if (!Projectile->GetVisualMesh())
	{
		return;
	}

	SpecialMaterialInstance = Projectile->GetVisualMesh()->CreateDynamicMaterialInstance(0);
	if (!SpecialMaterialInstance)
	{
		return;
	}

	SpecialMaterialInstance->SetVectorParameterValue(VisualProjectileColorParameterName, Color);
	SpecialMaterialInstance->SetScalarParameterValue(
		VisualEmissiveIntensityParameterName,
		FMath::Max(0.0f, EmissiveIntensity));
	SpecialMaterialInstance->SetScalarParameterValue(
		VisualFadeOpacityParameterName,
		FMath::Clamp(Opacity, 0.0f, 1.0f));
}

void URLProjectileVisualComponent::BeginFadeOut()
{
	ARLProjectile* Projectile = CastChecked<ARLProjectile>(GetOwner());
	FadeOutElapsedTime = 0.0f;
	FadeMaterialInstance = Projectile->GetVisualMesh()
		? Projectile->GetVisualMesh()->CreateDynamicMaterialInstance(0)
		: nullptr;
	if (FadeMaterialInstance)
	{
		FadeMaterialInstance->SetScalarParameterValue(VisualFadeOpacityParameterName, 1.0f);
	}
}

bool URLProjectileVisualComponent::UpdateFadeOut(float DeltaTime)
{
	FadeOutElapsedTime += FMath::Max(0.0f, DeltaTime);
	const float SafeDuration = FMath::Max(KINDA_SMALL_NUMBER, Settings.FadeOutDuration);
	const float FadeAlpha = FMath::Clamp(FadeOutElapsedTime / SafeDuration, 0.0f, 1.0f);
	if (FadeMaterialInstance)
	{
		FadeMaterialInstance->SetScalarParameterValue(
			VisualFadeOpacityParameterName,
			1.0f - FadeAlpha);
	}

	return FadeAlpha >= 1.0f;
}
