#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLPaperBurnComponent.generated.h"

class USkeletalMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class URLPaperBurnDataAsset;

UCLASS(ClassGroup=(ReflectionLab), meta=(BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLPaperBurnComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URLPaperBurnComponent();
	bool Start(USkeletalMeshComponent* Mesh, const TArray<UActorComponent*>& ComponentsToSuspend);
	void Cancel();
	bool IsPlaying() const { return bPlaying; }
	FSimpleDelegate OnFinished;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Death|PaperBurn") TObjectPtr<URLPaperBurnDataAsset> Settings;
protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> TargetMesh;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BurnMaterial;
	TArray<TWeakObjectPtr<UActorComponent>> SuspendedComponents;
	float Elapsed = 0.0f;
	bool bPlaying = false;
	bool bPreviousPauseAnims = false;
};
