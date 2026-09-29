#include "Framework/Menu/RLMenuCombatDirector.h"

#include "Camera/CameraComponent.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/TargetPoint.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ARLMenuCombatDirector::ARLMenuCombatDirector()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(SceneRoot);
	CameraBoom->TargetArmLength = 1800.0f;
	CameraBoom->SetRelativeRotation(FRotator(-58.0f, 45.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	MenuCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("MenuCamera"));
	MenuCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	MenuCamera->FieldOfView = 58.0f;

	static ConstructorHelpers::FClassFinder<ARLEnemyCharacter> EnemyClassFinder(
		TEXT("/Game/ReflectionLab/Gameplay/Enemies/BP_RLEnemyCharacter"));
	static ConstructorHelpers::FClassFinder<ARLProjectile> ProjectileClassFinder(
		TEXT("/Game/ReflectionLab/Gameplay/Projectiles/BP_RLProjectile"));
	static ConstructorHelpers::FObjectFinder<URLProjectileDefinitionDataAsset> ProjectileDefinitionFinder(
		TEXT("/Game/ReflectionLab/Data/Projectiles/DA_Projectile_Rally.DA_Projectile_Rally"));
	EnemyClass = EnemyClassFinder.Class;
	ProjectileClass = ProjectileClassFinder.Class;
	RallyProjectileDefinition = ProjectileDefinitionFinder.Object;
}

void ARLMenuCombatDirector::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = GetWorld()
		? GetWorld()->GetFirstPlayerController()
		: nullptr)
	{
		PlayerController->SetViewTarget(this);
	}

	SpawnMenuCombatants();
	if (MenuEnemies.Num() == EnemyOffsets.Num() &&
		RallyExitTargets.Num() == EnemyOffsets.Num() &&
		MenuEnemies.Num() >= 2 &&
		ProjectileClass &&
		RallyProjectileDefinition)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("Menu rally presentation started with %d enemies."),
			MenuEnemies.Num());
		GetWorldTimerManager().SetTimer(
			ShotTimerHandle,
			this,
			&ThisClass::FireNextPresentationShot,
			FMath::Max(0.1f, ShotInterval),
			true,
			0.35f);
	}
}

void ARLMenuCombatDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ShotTimerHandle);
	for (ARLEnemyCharacter* Enemy : MenuEnemies)
	{
		if (Enemy)
		{
			Enemy->Destroy();
		}
	}
	MenuEnemies.Reset();
	for (ATargetPoint* ExitTarget : RallyExitTargets)
	{
		if (ExitTarget)
		{
			ExitTarget->Destroy();
		}
	}
	RallyExitTargets.Reset();
	Super::EndPlay(EndPlayReason);
}

void ARLMenuCombatDirector::SpawnMenuCombatants()
{
	if (!GetWorld() || !EnemyClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Menu combat director has no enemy class."));
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	MenuEnemies.Reserve(EnemyOffsets.Num());
	RallyExitTargets.Reserve(EnemyOffsets.Num());
	for (const FVector& EnemyOffset : EnemyOffsets)
	{
		const FVector SpawnLocation =
			GetActorTransform().TransformPosition(EnemyOffset);
		const FRotator SpawnRotation =
			(GetActorLocation() - SpawnLocation).Rotation();
		ARLEnemyCharacter* Enemy = GetWorld()->SpawnActor<ARLEnemyCharacter>(
			EnemyClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParameters);
		if (Enemy)
		{
			Enemy->SetTutorialCombatControlled(true);
			Enemy->SetTutorialInvulnerable(true);
			Enemy->SetTutorialMovementLocked(true);
			MenuEnemies.Add(Enemy);
		}

		const FVector ExitDirection =
			FVector(EnemyOffset.X, EnemyOffset.Y, 0.0f).GetSafeNormal();
		ATargetPoint* ExitTarget = GetWorld()->SpawnActor<ATargetPoint>(
			GetActorLocation() + ExitDirection * 3000.0f + FVector(0.0f, 0.0f, 85.0f),
			FRotator::ZeroRotator,
			SpawnParameters);
		if (ExitTarget)
		{
			ExitTarget->SetActorHiddenInGame(true);
			RallyExitTargets.Add(ExitTarget);
		}
	}

	if (MenuEnemies.Num() != EnemyOffsets.Num() ||
		RallyExitTargets.Num() != EnemyOffsets.Num())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Menu combat director spawned %d enemies and %d exit targets; expected %d."),
			MenuEnemies.Num(),
			RallyExitTargets.Num(),
			EnemyOffsets.Num());
	}
}

void ARLMenuCombatDirector::FireNextPresentationShot()
{
	if (MenuEnemies.Num() < 2 || RallyExitTargets.Num() != MenuEnemies.Num())
	{
		return;
	}

	NextShooterIndex %= MenuEnemies.Num();
	const int32 TargetIndex =
		(NextShooterIndex + MenuEnemies.Num() / 2) % MenuEnemies.Num();
	if (SpawnPresentationProjectile(
		MenuEnemies[NextShooterIndex],
		MenuEnemies[TargetIndex],
		RallyExitTargets[NextShooterIndex]))
	{
		NextShooterIndex = (NextShooterIndex + 1) % MenuEnemies.Num();
	}
}

bool ARLMenuCombatDirector::SpawnPresentationProjectile(
	ARLEnemyCharacter* SourceEnemy,
	ARLEnemyCharacter* TargetEnemy,
	AActor* ExitTarget)
{
	UWorld* World = GetWorld();
	URLProjectilePoolSubsystem* PoolSubsystem = World
		? World->GetSubsystem<URLProjectilePoolSubsystem>()
		: nullptr;
	if (!PoolSubsystem || !SourceEnemy || !TargetEnemy || !ExitTarget ||
		!ProjectileClass || !RallyProjectileDefinition)
	{
		return false;
	}

	const FVector SpawnLocation = SourceEnemy->GetActorLocation() + FVector(0.0f, 0.0f, 85.0f);
	const FVector TargetLocation = TargetEnemy->GetActorLocation() + FVector(0.0f, 0.0f, 85.0f);
	const FVector Direction = (TargetLocation - SpawnLocation).GetSafeNormal();
	ARLProjectile* Projectile = PoolSubsystem->AcquireProjectile(
		ProjectileClass,
		FTransform(Direction.Rotation(), SpawnLocation),
		SourceEnemy,
		SourceEnemy);
	if (!Projectile)
	{
		return false;
	}

	Projectile->InitializeFromDefinition(RallyProjectileDefinition, ExitTarget);
	return true;
}
