#include "Combat/RLProjectileMirrorWall.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ARLProjectileMirrorWall::ARLProjectileMirrorWall()
{
	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MirrorVisual"));
	SetRootComponent(Visual);
	Visual->SetMobility(EComponentMobility::Movable);
	Visual->SetCollisionProfileName(TEXT("BlockAll"));
	Visual->SetCanEverAffectNavigation(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Visual->SetStaticMesh(Cube.Object);
}

void ARLProjectileMirrorWall::Configure(float HalfWidth, UMaterialInterface* Material, int32 BounceLimit)
{
	MaxBounces = FMath::Clamp(BounceLimit,1,8);
	SetActorScale3D(FVector(HalfWidth / 50, HalfThickness / 50, 2));
	if (Material) { Visual->SetMaterial(0, Material); }
	if (auto* Dynamic = Visual->CreateDynamicMaterialInstance(0))
	{ Dynamic->SetVectorParameterValue(TEXT("SurfaceColor"), FLinearColor(0.1f,0.35f,0.85f)); }
}

FVector ARLProjectileMirrorWall::GetMirroredTarget(const FVector& Target, float ProjectileRadius) const
{
	const FVector Normal = -GetActorRightVector();
	const FVector Plane = GetActorLocation() + Normal * (HalfThickness + FMath::Max(0.0f, ProjectileRadius));
	FVector Result = Target - 2 * FVector::DotProduct(Target-Plane, Normal) * Normal;
	Result.Z = Target.Z;
	return Result;
}

void ARLProjectileMirrorWall::ConfigureBounce(UProjectileMovementComponent* Movement) const
{
	if (!Movement) { return; }
	Movement->bShouldBounce = true;
	Movement->Bounciness = 1;
	Movement->Friction = 0;
	Movement->BounceVelocityStopSimulatingThreshold = 0;
}
