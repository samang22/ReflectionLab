#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RLRingAttackPoolSubsystem.generated.h"

class ARLExpandingRingAttack;
class URLRingAttackDataAsset;

UCLASS()
class REFLECTIONLAB_API URLRingAttackPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	ARLExpandingRingAttack* AcquireRing(const FTransform& Transform, AActor* NewOwner,
		APawn* NewInstigator, URLRingAttackDataAsset* Data, bool bAutoStart);
	void ReleaseRing(ARLExpandingRingAttack* Ring);
private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<ARLExpandingRingAttack>> InactiveRings;
};
