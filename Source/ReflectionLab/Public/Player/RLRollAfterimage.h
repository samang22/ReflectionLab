#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLRollAfterimage.generated.h"

class UPoseableMeshComponent;
class USkeletalMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class URLRollAfterimagePoolSubsystem;

// A frozen world-space pose, independent of the player's later animation.
UCLASS(NotBlueprintable, Transient)
class REFLECTIONLAB_API ARLRollAfterimage : public AActor
{
	GENERATED_BODY()

public:
	ARLRollAfterimage();
	bool IsPoolActive() const { return bIsPoolActive; }
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class URLRollAfterimagePoolSubsystem;
	bool PrepareResources(USkeletalMeshComponent* Source, UMaterialInterface* Material);
	bool ActivateFromPool(USkeletalMeshComponent* Source, UMaterialInterface* Material,
		const FLinearColor& Color, float Duration, float Opacity);
	void DeactivateForPool();
	void ReturnToPool();
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPoseableMeshComponent> PoseMesh;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FadeMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> CachedMaterial;
	bool bIsPoolActive = false;
	float Lifetime = 0.22f;
	float Elapsed = 0.0f;
	float InitialOpacity = 0.25f;
};
