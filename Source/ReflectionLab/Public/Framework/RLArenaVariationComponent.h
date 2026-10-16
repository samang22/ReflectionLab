#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLArenaVariationComponent.generated.h"
class URLArenaExperimentDataAsset;
class ARLEnemyCharacter;
class AStaticMeshActor;

// Runtime for non-shutter experiments. The coordinator owns round lifecycle.
UCLASS()
class REFLECTIONLAB_API URLArenaVariationComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	bool Configure(URLArenaExperimentDataAsset* Data);
	void ResetVariation();
	void RegisterEnemy(ARLEnemyCharacter* Enemy, int32 Index);
	void Advance(float DeltaTime);
	FText GetStatusText() const;
	bool IsZoneDangerous(const FVector& Position) const;
private:
	UPROPERTY(Transient) TObjectPtr<URLArenaExperimentDataAsset> Settings;
	UPROPERTY(Transient) TArray<TObjectPtr<AStaticMeshActor>> Visuals;
	TArray<TWeakObjectPtr<ARLEnemyCharacter>> Enemies;
	TWeakObjectPtr<ARLEnemyCharacter> Support;
	uint64 SupportSerial = 0;
	float Elapsed = 0;
	float DamageCooldown = 0;
	bool bSupportLaserTickEnabled = false;
	AStaticMeshActor* MakeVisual(const FVector& Position, const FVector& Scale);
};
