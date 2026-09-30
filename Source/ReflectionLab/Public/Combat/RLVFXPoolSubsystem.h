#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RLVFXPoolSubsystem.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/** Reuses short-lived Niagara components to avoid repeated VFX allocations during combat. */
UCLASS()
class REFLECTIONLAB_API URLVFXPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UNiagaraComponent* PlaySystemAtLocation(
		UNiagaraSystem* System,
		const FVector& Location,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FVector& Scale = FVector::OneVector);

	void ReleaseSystem(UNiagaraComponent* Component);

	virtual void Deinitialize() override;

private:
	UFUNCTION()
	void HandleSystemFinished(UNiagaraComponent* FinishedComponent);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> PooledComponents;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UNiagaraComponent>, TObjectPtr<UNiagaraSystem>> ComponentSystems;

	TSet<TObjectPtr<UNiagaraComponent>> ActiveComponents;
};
