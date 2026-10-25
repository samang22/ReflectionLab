#include "Combat/RLProjectileEffectPoolSubsystem.h"
#include "Combat/RLExplosionVisual.h"
#include "Framework/RLPoolDiagnosticsSubsystem.h"
#include "Engine/World.h"

ARLExplosionVisual* URLProjectileEffectPoolSubsystem::PlayEffect(
	const FVector& Location, UStaticMesh* Mesh, UMaterialInterface* Material)
{
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || !Mesh || !Material) { return nullptr; }
	ARLExplosionVisual* Effect = nullptr;
	while (!InactiveEffects.IsEmpty() && !IsValid(Effect))
	{
		Effect = InactiveEffects.Pop(EAllowShrinking::No);
	}
	const bool bReused = IsValid(Effect);
	if (!Effect)
	{
		Effects.RemoveAll([](const TObjectPtr<ARLExplosionVisual>& Item) { return !IsValid(Item); });
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Effect = World->SpawnActor<ARLExplosionVisual>(ARLExplosionVisual::StaticClass(), Location, FRotator::ZeroRotator, Parameters);
		if (!Effect) { return nullptr; }
		Effects.Add(Effect);
		URLPoolDiagnosticsSubsystem::Created(Effect, TEXT("ProjectileEffect"));
	}
	Effect->SetActorLocationAndRotation(Location, FRotator::ZeroRotator);
	URLPoolDiagnosticsSubsystem::Acquired(Effect, TEXT("ProjectileEffect"), bReused);
	if (!Effect->ActivatePooledProjectileEffect(Mesh, Material))
	{
		ReleaseEffect(Effect);
		return nullptr;
	}
	return Effect;
}

void URLProjectileEffectPoolSubsystem::ReleaseEffect(ARLExplosionVisual* Effect)
{
	if (!IsValid(Effect) || !Effects.Contains(Effect) || InactiveEffects.Contains(Effect)) { return; }
	Effect->DeactivateForPool();
	URLPoolDiagnosticsSubsystem::Released(Effect, TEXT("ProjectileEffect"));
	InactiveEffects.Add(Effect);
}
