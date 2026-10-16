#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLBallisticsExperimentComponent.generated.h"
class URLArenaExperimentDataAsset;
class ARLProjectile;
class AStaticMeshActor;

UCLASS()
class REFLECTIONLAB_API URLBallisticsExperimentComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	bool Configure(URLArenaExperimentDataAsset* Data);
	void ResetExperiment();
	void Advance();
	FText GetStatusText() const;
private:
	struct FTransit
	{
		uint64 Serial = 0;
		FVector Previous = FVector::ZeroVector;
		bool bReflected = false;
		bool bApplied = false;
	};
	TMap<TWeakObjectPtr<ARLProjectile>, FTransit> Transits;
	UPROPERTY(Transient) TObjectPtr<URLArenaExperimentDataAsset> Settings;
	UPROPERTY(Transient) TArray<TObjectPtr<AStaticMeshActor>> Visuals;
	bool AddPlate(const FVector& Position, const FVector& Scale, float Yaw, const FLinearColor& Color);
};
