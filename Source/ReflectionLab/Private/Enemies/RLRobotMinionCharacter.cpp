#include "Enemies/RLRobotMinionCharacter.h"

#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/RLEnemyCombatRow.h"
#include "Data/RLRobotMinionDataAsset.h"
#include "Enemies/Components/RLBossAimComponent.h"
#include "Enemies/Components/RLBossLaserComponent.h"
#include "Enemies/Components/RLEnemyAttackComponent.h"
#include "Engine/StaticMesh.h"
#include "Player/Components/RLHealthComponent.h"
#include "UObject/ConstructorHelpers.h"

ARLRobotMinionCharacter::ARLRobotMinionCharacter()
{
	AimComponent = CreateDefaultSubobject<URLBossAimComponent>(TEXT("AimComponent"));
	LaserComponent = CreateDefaultSubobject<URLBossLaserComponent>(TEXT("LaserComponent"));
	LaserComponent->bSerializeWithPeers = true;
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
}

void ARLRobotMinionCharacter::BeginPlay()
{
	if (GetMesh()->DoesSocketExist(TEXT("muzzle")))
	{
		MuzzlePoint->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("muzzle"));
	}
	Super::BeginPlay();
	if (!Settings || !Settings->ProjectileDefinition || !Settings->LaserSettings ||
		!FMath::IsFinite(Settings->MaxHealth) || !FMath::IsFinite(Settings->AttackInterval))
	{
		UE_LOG(LogTemp, Error, TEXT("Robot minion %s settings incomplete; combat disabled."), *GetName());
		AttackComponent->StopFiring();
		SetTutorialCombatControlled(true);
		LaserComponent->SetComponentTickEnabled(false);
		return;
	}
	HealthComponent->SetMaxHealth(FMath::Max(1.0f, Settings->MaxHealth), IsPoolActive());
	FRLEnemyCombatRow Combat;
	Combat.MaxHealth = Settings->MaxHealth;
	Combat.AttackInterval = Settings->AttackInterval;
	Combat.ShotsPerBurst = 1;
	AttackComponent->Configure(Combat, ProjectileClass, MuzzlePoint, true);
	// Do not inherit round-level explosive/rally/ring rules: basic shots only.
	FRLWaveDefinition Wave;
	Wave.DefaultProjectileDefinition = Settings->ProjectileDefinition;
	AttackComponent->ApplyWaveDefinition(Wave);
	LaserComponent->Settings = Settings->LaserSettings;
	LaserComponent->Initialize(GetMesh(), LaserBeam, LaserDecal);
}
