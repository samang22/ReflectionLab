#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLExplosionVisual.generated.h"

class UDecalComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(NotBlueprintable)
class REFLECTIONLAB_API ARLExplosionVisual : public AActor
{
	GENERATED_BODY()

public:
	ARLExplosionVisual();

	virtual void Tick(float DeltaTime) override;

	void Initialize(
		UStaticMesh* ShardMesh,
		UMaterialInterface* ShardMaterial,
		const FVector& ShardBaseScale,
		float BlastRadius,
		const FLinearColor& DecalColor,
		bool bShowGroundDecal = true);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDecalComponent> ExplosionRadiusDecalFront;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDecalComponent> ExplosionRadiusDecalBack;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDecalComponent> ExplosionRadiusDecalLeft;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDecalComponent> ExplosionRadiusDecalRight;

	UPROPERTY(EditDefaultsOnly, Category = "Explosion Visual", meta = (ClampMin = "0.1"))
	float EffectDuration = 0.7f;

	UPROPERTY(EditDefaultsOnly, Category = "Explosion Visual", meta = (ClampMin = "4", ClampMax = "24"))
	int32 ShardRayCount = 12;

	UPROPERTY(EditDefaultsOnly, Category = "Explosion Visual", meta = (ClampMin = "1", ClampMax = "6"))
	int32 TrailSegmentsPerRay = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Explosion Visual", meta = (ClampMin = "0.01", ClampMax = "0.2"))
	float TrailSegmentDelay = 0.045f;

	UPROPERTY(EditDefaultsOnly, Category = "Explosion Visual", meta = (ClampMin = "0.1"))
	float ShardTravelRadiusMultiplier = 1.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Explosion Visual", meta = (ClampMin = "0.0"))
	float ShardArcHeight = 65.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Explosion Visual", meta = (ClampMin = "0.01"))
	float ShardScaleMultiplier = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "Explosion Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DecalOpacity = 0.38f;

	UPROPERTY(EditDefaultsOnly, Category = "Explosion Visual", meta = (ClampMin = "0.1"))
	float DecalRadiusMultiplier = 3.0f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> ShardMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ExplosionDecalMaterialInstances;

	TArray<FVector> ShardDirections;
	FVector CachedShardBaseScale = FVector::OneVector;
	float CachedBlastRadius = 1.0f;
	float ElapsedTime = 0.0f;
	bool bInitialized = false;
};
