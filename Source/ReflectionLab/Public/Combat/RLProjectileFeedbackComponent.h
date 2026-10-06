#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLProjectileFeedbackComponent.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLProjectileFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URLProjectileFeedbackComponent();
	void ResetForActivation() { bReturnFeedbackHandled = false; }
	void MarkExplosionPlayed() { bReturnFeedbackHandled = true; }
	void PlayPoolReturnEffect(UStaticMeshComponent* Visual);

private:
	UPROPERTY()
	TObjectPtr<UStaticMesh> ShardMesh;
	bool bReturnFeedbackHandled = false;
};
