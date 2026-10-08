#include "Enemies/RLRobotBossCharacter.h"

#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Enemies/Components/RLBossOverloadComponent.h"
#include "Enemies/Components/RLBossLaserComponent.h"
#include "Enemies/Components/RLBossAimComponent.h"
#include "Enemies/Components/RLBossChargeComponent.h"
#include "Enemies/Components/RLBossSummonComponent.h"
#include "Enemies/Components/RLBossSummonVisualComponent.h"
#include "Enemies/Components/RLPaperBurnComponent.h"
#include "Enemies/Components/RLEnemyAttackComponent.h"
#include "Enemies/Components/RLEnemyMovementComponent.h"
#include "Enemies/Components/RLEnemySpawnVisualComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ARLRobotBossCharacter::ARLRobotBossCharacter()
{
	OverloadComponent = CreateDefaultSubobject<URLBossOverloadComponent>(TEXT("OverloadComponent"));
	AimComponent = CreateDefaultSubobject<URLBossAimComponent>(TEXT("AimComponent"));
	AbsorptionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("AbsorptionVolume"));
	AbsorptionVolume->SetupAttachment(GetRootComponent());
	AbsorptionVolume->SetCollisionObjectType(ECC_Pawn);
	AbsorptionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	AbsorptionVolume->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	AbsorptionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AbsorptionVolume->SetGenerateOverlapEvents(true);
	AbsorptionVolume->SetCanEverAffectNavigation(false);
	AbsorptionDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("AbsorptionDecal"));
	AbsorptionDecal->SetupAttachment(GetRootComponent());
	AbsorptionDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	AbsorptionDecal->SetVisibility(false);
	LaserComponent = CreateDefaultSubobject<URLBossLaserComponent>(TEXT("LaserComponent"));
	LaserBeam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LaserBeam"));
	LaserBeam->SetupAttachment(GetRootComponent());
	LaserBeam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LaserBeam->SetCanEverAffectNavigation(false);
	LaserBeam->SetCastShadow(false);
	LaserBeam->SetReceivesDecals(false);
	LaserBeam->bDisallowNanite = true;
	LaserBeam->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	LaserBeam->SetStaticMesh(Cube.Object);
	LaserDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("LaserDecal"));
	LaserDecal->SetupAttachment(GetRootComponent());
	LaserDecal->SetVisibility(false);
	ChargeComponent = CreateDefaultSubobject<URLBossChargeComponent>(TEXT("ChargeComponent"));
	ChargeDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("ChargeDecal"));
	ChargeDecal->SetupAttachment(GetRootComponent());
	ChargeDecal->SetVisibility(false);
	SummonComponent = CreateDefaultSubobject<URLBossSummonComponent>(TEXT("SummonComponent"));
	SummonVisualComponent = CreateDefaultSubobject<URLBossSummonVisualComponent>(TEXT("SummonVisualComponent"));
	PaperBurnComponent = CreateDefaultSubobject<URLPaperBurnComponent>(TEXT("PaperBurnComponent"));
}

void ARLRobotBossCharacter::BeginPlay()
{
	// Attach at runtime so existing boss Blueprint component defaults also
	// follow the animated socket without an asset migration.
	const FName MuzzleSocket(TEXT("muzzle"));
	if (GetMesh()->DoesSocketExist(MuzzleSocket))
	{
		if (!MuzzlePoint->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, MuzzleSocket))
		{
			UE_LOG(LogTemp, Warning, TEXT("Boss %s could not attach its muzzle point to socket 'muzzle'."), *GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Boss %s has no 'muzzle' socket; retaining the existing firing point."), *GetName());
	}
	Super::BeginPlay();
	OverloadComponent->Initialize(HealthComponent, GetMesh(), AbsorptionVolume, AbsorptionDecal, ProjectileClass);
	LaserComponent->Initialize(GetMesh(), LaserBeam, LaserDecal);
	ChargeComponent->Initialize(GetMesh(), ChargeDecal, ProjectileClass);
	SummonComponent->Initialize(HealthComponent);
	PaperBurnComponent->OnFinished.BindUObject(this, &ARLEnemyCharacter::ReturnToPool);
}

void ARLRobotBossCharacter::Die()
{
	if (PaperBurnComponent->IsPlaying()) { return; }
	// Stop gameplay first; the still-active corpse holds round completion until the burn ends.
	AttackComponent->StopFiring();
	LaserComponent->Cancel();
	ChargeComponent->Cancel();
	OverloadComponent->Reset();
	SummonComponent->Reset();
	SpawnVisualComponent->CancelSpawn();
	EnemyMovementComponent->SetTutorialMovementLocked(true);
	EnemyMovementComponent->SetFacingLocked(true);
	SetActorEnableCollision(false);
	if (!PaperBurnComponent->Start(GetMesh(), {AimComponent.Get(), LaserComponent.Get(), ChargeComponent.Get(),
		OverloadComponent.Get(), SummonComponent.Get(), EnemyMovementComponent.Get()}))
	{
		ReturnToPool();
	}
}
