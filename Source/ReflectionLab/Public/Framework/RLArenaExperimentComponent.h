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
	struct FEnemyEntry { TWeakObjectPtr<ARLEnemyCharacter> Enemy; };
	TArray<FEnemyEntry> Enemies;
	UPROPERTY(Transient)
	TObjectPtr<URLArenaExperimentDataAsset> Settings;
	UPROPERTY(Transient)
	TArray<TObjectPtr<AStaticMeshActor>> Shutters;
	UPROPERTY(Transient)
	TArray<TObjectPtr<AStaticMeshActor>> Indicators;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
	float Elapsed = 0;
	int32 LastPhase = INDEX_NONE;
	bool bSkipClosing[2] = {false, false};
	float PhaseStartHeight[2] = {0, 0};
	float PhaseTime() const;
	bool PlayerBlocksShutter(int32 Index) const;
};
