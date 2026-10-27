#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLCrossfireExperimentComponent.generated.h"
class URLArenaExperimentDataAsset;
class ARLEnemyCharacter;
class AStaticMeshActor;
class UMaterialInstanceDynamic;

UCLASS(ClassGroup=(ReflectionLab), meta=(BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLCrossfireExperimentComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URLCrossfireExperimentComponent();
	void Configure(URLArenaExperimentDataAsset* Data);
	void ResetExperiment();
	bool HasActiveExperiment() const { return Settings != nullptr; }
	bool FindTeamSpawn(int32 Index, int32 Count, FTransform& OutTransform) const;
	void RegisterEnemy(ARLEnemyCharacter* Enemy, int32 Index);
	FText GetStatusText() const;
	// Explicit simulation step, also used by the standalone experiment harness.
	void AdvanceExperiment(float DeltaTime);
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
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
	TArray<bool> bSkipClosing;
	TArray<FVector> ShutterSizes;
	TArray<int32> ShutterPhaseGroups;
	float PhaseTime() const;
	bool PlayerBlocksShutter(int32 Index) const;
};
