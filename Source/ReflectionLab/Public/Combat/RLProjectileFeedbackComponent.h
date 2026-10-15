#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLProjectileFeedbackComponent.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class USoundBase;
class URLProjectileDefinitionDataAsset;

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLProjectileFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URLProjectileFeedbackComponent();
	void ResetForActivation() { bReturnFeedbackHandled = false; PoolReturnSound = nullptr; }
	void ApplyDefinitionSettings(const URLProjectileDefinitionDataAsset& Definition);
	void MarkExplosionPlayed() { bReturnFeedbackHandled = true; }
	void PlayPoolReturnEffect(UStaticMeshComponent* Visual);

private:
	UPROPERTY()
	TObjectPtr<UStaticMesh> ShardMesh;
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> PoolReturnSound;
	float PoolReturnSoundVolume = 0.3f;
	bool bReturnFeedbackHandled = false;
};
