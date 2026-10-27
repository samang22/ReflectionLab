#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLArenaExperimentComponent.generated.h"
class URLArenaExperimentDataAsset;
class ARLEnemyCharacter;
class AStaticMeshActor;
class UMaterialInstanceDynamic;
class URLArenaVariationComponent;
class URLBallisticsExperimentComponent;
class URLMirrorArenaComponent;
class URLCrossfireExperimentComponent;

UCLASS(ClassGroup=(ReflectionLab), meta=(BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLArenaExperimentComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URLArenaExperimentComponent();
	void Configure(URLArenaExperimentDataAsset* Data);
	void ResetExperiment();
	bool HasActiveExperiment() const { return Settings != nullptr; }
	bool FindTeamSpawn(int32 Index, int32 Count, FTransform& OutTransform) const;
	void RegisterEnemy(ARLEnemyCharacter* Enemy, int32 Index);
	FText GetStatusText() const;
	// Explicit simulation step, also used by the standalone experiment harness.
	void AdvanceExperiment(float DeltaTime);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UPROPERTY(Transient) TObjectPtr<URLMirrorArenaComponent> MirrorArena;
	UPROPERTY(Transient) TObjectPtr<URLBallisticsExperimentComponent> Ballistics;
	UPROPERTY(Transient) TObjectPtr<URLArenaVariationComponent> Variation;
	UPROPERTY(Transient) TObjectPtr<URLCrossfireExperimentComponent> Crossfire;
	UPROPERTY(Transient) TObjectPtr<URLArenaExperimentDataAsset> Settings;
};
