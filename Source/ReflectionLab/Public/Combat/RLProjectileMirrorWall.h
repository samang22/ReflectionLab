#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLProjectileMirrorWall.generated.h"
class UStaticMeshComponent;
class UMaterialInterface;
class UProjectileMovementComponent;

UCLASS()
class REFLECTIONLAB_API ARLProjectileMirrorWall : public AActor
{
	GENERATED_BODY()
public:
	ARLProjectileMirrorWall();
	void Configure(float HalfWidth, UMaterialInterface* Material, int32 BounceLimit = 4);
	int32 GetBounceLimit() const { return MaxBounces; }
	FVector GetMirroredTarget(const FVector& Target, float ProjectileRadius) const;
	void ConfigureBounce(UProjectileMovementComponent* Movement) const;
private:
	int32 MaxBounces = 4;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
	static constexpr float HalfThickness = 20;
};
