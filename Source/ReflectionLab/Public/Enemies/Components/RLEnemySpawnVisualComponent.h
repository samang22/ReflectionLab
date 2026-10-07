#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLEnemySpawnVisualComponent.generated.h"

class USkeletalMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

DECLARE_MULTICAST_DELEGATE(FRLEnemySpawnFinished);

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLEnemySpawnVisualComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URLEnemySpawnVisualComponent();
	bool StartSpawn(USkeletalMeshComponent* Mesh);
	void CancelSpawn();
	bool IsSpawning() const { return bSpawning; }
	FRLEnemySpawnFinished OnSpawnFinished;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	TObjectPtr<UMaterialInterface> SpawnMaterial;
	UPROPERTY(EditDefaultsOnly, Category = "Spawn", meta = (ClampMin = "0.0"))
	float SpawnDuration = 0.5f;
	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	bool bOverrideSpawnColor = false;
	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	FLinearColor SpawnColor = FLinearColor(1.0f, 0.04f, 0.015f);
	UPROPERTY(EditDefaultsOnly, Category = "Spawn", meta = (ClampMin = "0.0"))
	float CompletionFlashDuration = 0.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> TargetMesh;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SpawnInstance;
	float Elapsed = 0.0f;
	bool bSpawning = false;
};
