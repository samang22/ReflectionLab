#include "Combat/RLVFXPoolSubsystem.h"

#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

UNiagaraComponent* URLVFXPoolSubsystem::PlaySystemAtLocation(
	UNiagaraSystem* System,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& Scale)
{
	UWorld* World = GetWorld();
	if (!World || !System)
	{
		return nullptr;
	}

	UNiagaraComponent* Component = nullptr;
	for (UNiagaraComponent* Candidate : PooledComponents)
	{
		if (Candidate &&
			ComponentSystems.FindRef(Candidate) == System &&
			!ActiveComponents.Contains(Candidate))
		{
			Component = Candidate;
			break;
		}
	}

	if (!Component)
	{
		Component = NewObject<UNiagaraComponent>(this);
		if (!Component)
		{
			return nullptr;
		}

		Component->SetAutoActivate(false);
		Component->SetAutoDestroy(false);
		Component->SetAsset(System);
		Component->OnSystemFinished.AddUniqueDynamic(
			this,
			&ThisClass::HandleSystemFinished);
		Component->RegisterComponentWithWorld(World);
		Component->SetHiddenInGame(true);
		PooledComponents.Add(Component);
		ComponentSystems.Add(Component, System);
	}

	Component->SetWorldLocationAndRotation(Location, Rotation);
	Component->SetWorldScale3D(Scale);
	Component->SetHiddenInGame(false);
	Component->SetVisibility(true, true);
	ActiveComponents.Add(Component);
	Component->Activate(true);
	return Component;
}

void URLVFXPoolSubsystem::ReleaseSystem(UNiagaraComponent* Component)
{
	if (!Component || !ActiveComponents.Remove(Component))
	{
		return;
	}

	Component->DeactivateImmediate();
	Component->SetVisibility(false, true);
	Component->SetHiddenInGame(true);
}

void URLVFXPoolSubsystem::Deinitialize()
{
	for (UNiagaraComponent* Component : PooledComponents)
	{
		if (Component)
		{
			Component->OnSystemFinished.RemoveDynamic(
				this,
				&ThisClass::HandleSystemFinished);
			Component->DeactivateImmediate();
			Component->DestroyComponent();
		}
	}

	ActiveComponents.Reset();
	ComponentSystems.Reset();
	PooledComponents.Reset();
	Super::Deinitialize();
}

void URLVFXPoolSubsystem::HandleSystemFinished(UNiagaraComponent* FinishedComponent)
{
	ReleaseSystem(FinishedComponent);
}
