#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLMirrorArenaComponent.generated.h"
class URLArenaExperimentDataAsset;
class ARLProjectileMirrorWall;
class AStaticMeshActor;
class ARLEnemyCharacter;
class USpringArmComponent;

UCLASS()
class REFLECTIONLAB_API URLMirrorArenaComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URLMirrorArenaComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	bool Configure(URLArenaExperimentDataAsset* Data);
	void ResetArena();
	bool FindSpawn(int32 Index, FTransform& Transform) const;
	void RegisterEnemy(ARLEnemyCharacter* Enemy);
	FText GetStatusText() const;
private:
	bool AddBarrier(const FVector& Location, const FVector& Size, bool bPawnOnly);
	UPROPERTY(Transient) TObjectPtr<URLArenaExperimentDataAsset> Settings;
	UPROPERTY(Transient) TObjectPtr<ARLProjectileMirrorWall> Mirror;
	UPROPERTY(Transient) TArray<TObjectPtr<ARLProjectileMirrorWall>> ExtraMirrors;
	UPROPERTY(Transient) TArray<TObjectPtr<AStaticMeshActor>> Barriers;
	TArray<TWeakObjectPtr<ARLEnemyCharacter>> Enemies;
	TWeakObjectPtr<USpringArmComponent> CameraBoom;
	float OriginalArmLength = 0;
	FVector OriginalBoomLocation = FVector::ZeroVector;
	FVector CameraAnchor = FVector::ZeroVector;
	bool bOriginalAbsoluteLocation = false;
};
