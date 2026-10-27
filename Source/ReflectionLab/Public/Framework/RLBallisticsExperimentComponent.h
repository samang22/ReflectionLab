#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLBallisticsExperimentComponent.generated.h"
class URLArenaExperimentDataAsset;
class ARLProjectile;
class AStaticMeshActor;
class ARLAccelerationBarrier;
class ARLEnemyCharacter;

UCLASS()
class REFLECTIONLAB_API URLBallisticsExperimentComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	bool Configure(URLArenaExperimentDataAsset* Data);
	void ResetExperiment();
	void Advance();
	void ProcessProjectile(ARLProjectile* Projectile, const FVector& Position);
	bool FindBarrierSpawn(int32 Index, int32 Count, FTransform& Transform) const;
	void RegisterEnemy(ARLEnemyCharacter* Enemy);
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
	UPROPERTY(Transient) TArray<TObjectPtr<ARLAccelerationBarrier>> Barriers;
	TArray<TWeakObjectPtr<ARLEnemyCharacter>> Enemies;
	bool AddPlate(const FVector& Position, const FVector& Scale, float Yaw, const FLinearColor& Color);
};
