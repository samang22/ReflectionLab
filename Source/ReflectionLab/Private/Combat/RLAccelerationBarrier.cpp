#include "Combat/RLAccelerationBarrier.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLProjectileFieldResponseComponent.h"
#include "Framework/RLBallisticsExperimentComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

ARLAccelerationBarrier::ARLAccelerationBarrier()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	Sensor = CreateDefaultSubobject<UBoxComponent>(TEXT("FilterSensor"));
	SetRootComponent(Sensor);
	// Overlap does not stop projectile movement. Qualification decides whether to absorb it.
	Sensor->SetCollisionObjectType(ECC_WorldStatic);
	Sensor->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sensor->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sensor->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Sensor->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Sensor->SetGenerateOverlapEvents(true);
	Sensor->SetCanEverAffectNavigation(false);
	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FilterVisual"));
	Visual->SetupAttachment(Sensor);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetCanEverAffectNavigation(false);
	Visual->SetCastShadow(false);
	Visual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
}

void ARLAccelerationBarrier::Configure(const FVector& Size, UMaterialInterface* Material, URLBallisticsExperimentComponent* Field)
{
	Experiment = Field;
	Sensor->SetBoxExtent(Size * 0.5f);
	// Full-height gameplay filter, low visual so the player is never hidden behind it.
	Visual->SetRelativeScale3D(FVector(Size.X, Size.Y, 60) / 100);
	Visual->SetRelativeLocation(FVector(0, 0, (60 - Size.Z) * 0.5f));
	if (Material) { Visual->SetMaterial(0, Material); }
	Surface = Visual->CreateDynamicMaterialInstance(0);
	if (Surface) { Surface->SetVectorParameterValue(TEXT("SurfaceColor"), FLinearColor(0.05f, 0.5f, 0.6f)); }
}

void ARLAccelerationBarrier::HandleProjectile(ARLProjectile* Projectile, const FVector& ContactPosition)
{
	if (!Projectile || !Projectile->IsPoolActive() || Projectile->IsFadingOut()) { return; }
	// Hostile shots must enter so the player can parry. Only outgoing returns are filtered.
	if (!Projectile->IsReflected()) { return; }
	if (Experiment.IsValid()) { Experiment->ProcessProjectile(Projectile, ContactPosition); }
	const bool bQualified = Projectile->GetFieldResponse()->HasAccelerationCharge();
	if (Surface) { Surface->SetVectorParameterValue(TEXT("SurfaceColor"), bQualified ? FLinearColor(0.1f, 0.9f, 1) : FLinearColor(0.9f, 0.2f, 0.05f)); }
	FlashRemaining = 0.18f;
	SetActorTickEnabled(true);
	if (!bQualified) { Projectile->ReturnToPool(); }
}

void ARLAccelerationBarrier::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FlashRemaining -= FMath::Max(0.0f, DeltaSeconds);
	if (FlashRemaining <= 0)
	{
		if (Surface) { Surface->SetVectorParameterValue(TEXT("SurfaceColor"), FLinearColor(0.05f, 0.5f, 0.6f)); }
		SetActorTickEnabled(false);
	}
}
