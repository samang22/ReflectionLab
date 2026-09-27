#include "Enemies/RLEnemyCharacter.h"

#include "Combat/RLProjectile.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Enemies/RLEnemyPoolSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/RLEnemyCombatRow.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

ARLEnemyCharacter::ARLEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	GetMesh()->SetReceivesDecals(false);

	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(GetRootComponent());
}

void ARLEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyCombatConfig();
	CurrentHealth = MaxHealth;

	if (ProjectileClass)
	{
		if (URLProjectilePoolSubsystem* PoolSubsystem =
			GetWorld()->GetSubsystem<URLProjectilePoolSubsystem>())
		{
			PoolSubsystem->PrewarmPool(ProjectileClass, ProjectilePoolPrewarmCount);
		}
	}

	if (bIsPoolActive && bAutoStartFiring)
	{
		StartFiring();
	}
}

void ARLEnemyCharacter::ApplyCombatConfig()
{
	if (!CombatConfig.DataTable || CombatConfig.RowName.IsNone())
	{
		return;
	}

	static const FString ContextString(TEXT("Enemy combat configuration"));
	const FRLEnemyCombatRow* Config =
		CombatConfig.GetRow<FRLEnemyCombatRow>(ContextString);
	if (!Config)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Enemy combat config row '%s' could not be loaded for %s."),
			*CombatConfig.RowName.ToString(),
			*GetName());
		return;
	}

	MaxHealth = FMath::Max(1.0f, Config->MaxHealth);
	ProjectilePoolPrewarmCount = FMath::Max(0, Config->ProjectilePoolPrewarmCount);
	AttackInterval = FMath::Max(0.1f, Config->AttackInterval);
	ShotsPerBurst = FMath::Max(1, Config->ShotsPerBurst);
	TimeBetweenShots = FMath::Max(0.01f, Config->TimeBetweenShots);
	InitialFireDelay = FMath::Max(0.0f, Config->InitialFireDelay);
	ExplosiveShotInterval = FMath::Max(0, Config->ExplosiveShotInterval);
	RallyShotInterval = FMath::Max(0, Config->RallyShotInterval);
	RallyRelayCount = FMath::Max(1, Config->RallyRelayCount);
	RallySpeedMultiplierPerRelay = FMath::Max(1.0f, Config->RallySpeedMultiplierPerRelay);
}

float ARLEnemyCharacter::TakeDamage(
	float DamageAmount,
	const FDamageEvent& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (!bIsPoolActive)
	{
		return 0.0f;
	}

	const float AppliedDamage = Super::TakeDamage(
		DamageAmount,
		DamageEvent,
		EventInstigator,
		DamageCauser);

	if (AppliedDamage <= 0.0f || CurrentHealth <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - AppliedDamage);

	if (CurrentHealth <= 0.0f)
	{
		Die();
	}

	return AppliedDamage;
}

void ARLEnemyCharacter::StartFiring()
{
	if (!GetWorld() || AttackTimerHandle.IsValid())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		AttackTimerHandle,
		this,
		&ThisClass::BeginBurst,
		FMath::Max(0.1f, AttackInterval),
		true,
		FMath::Max(0.0f, InitialFireDelay));
}

void ARLEnemyCharacter::StopFiring()
{
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(BurstTimerHandle);
	RemainingShotsInBurst = 0;
}

void ARLEnemyCharacter::BeginBurst()
{
	if (RemainingShotsInBurst > 0)
	{
		return;
	}

	RemainingShotsInBurst = FMath::Max(1, ShotsPerBurst);
	FireNextShot();

	if (RemainingShotsInBurst > 0)
	{
		GetWorldTimerManager().SetTimer(
			BurstTimerHandle,
			this,
			&ThisClass::FireNextShot,
			FMath::Max(0.01f, TimeBetweenShots),
			true);
	}
}

void ARLEnemyCharacter::FireNextShot()
{
	if (RemainingShotsInBurst <= 0)
	{
		GetWorldTimerManager().ClearTimer(BurstTimerHandle);
		return;
	}

	Fire();
	--RemainingShotsInBurst;

	if (RemainingShotsInBurst <= 0)
	{
		GetWorldTimerManager().ClearTimer(BurstTimerHandle);
	}

}

void ARLEnemyCharacter::Fire()
{
	if (!ProjectileClass || !MuzzlePoint || !GetWorld())
	{
		return;
	}

	APawn* TargetPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!TargetPawn)
	{
		return;
	}

	const FVector SpawnLocation = MuzzlePoint->GetComponentLocation();
	const FRotator SpawnRotation =
		(TargetPawn->GetActorLocation() - SpawnLocation).Rotation();

	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
	if (URLProjectilePoolSubsystem* PoolSubsystem =
		GetWorld()->GetSubsystem<URLProjectilePoolSubsystem>())
	{
		++ShotsFiredSinceActivation;
		const bool bShouldFireExplosive = ExplosiveShotInterval > 0 &&
			ShotsFiredSinceActivation % ExplosiveShotInterval == 0;
		const bool bShouldFireRally = !bShouldFireExplosive &&
			RallyShotInterval > 0 &&
			ShotsFiredSinceActivation % RallyShotInterval == 0;

		ARLProjectile* Projectile = PoolSubsystem->AcquireProjectile(
			ProjectileClass,
			SpawnTransform,
			this,
			this);
		if (Projectile && bShouldFireExplosive)
		{
			Projectile->ConfigureAsExplosive();
		}
		else if (Projectile && bShouldFireRally)
		{
			Projectile->ConfigureAsRally(
				TargetPawn,
				RallyRelayCount,
				RallySpeedMultiplierPerRelay);
		}
	}
}

void ARLEnemyCharacter::Die()
{
	ReturnToPool();
}

void ARLEnemyCharacter::ReturnToPool()
{
	if (!bIsPoolActive)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (URLEnemyPoolSubsystem* PoolSubsystem =
			World->GetSubsystem<URLEnemyPoolSubsystem>())
		{
			PoolSubsystem->ReleaseEnemy(this);
			return;
		}
	}

	Destroy();
}

void ARLEnemyCharacter::ActivateFromPool(const FTransform& SpawnTransform)
{
	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetDefaultMovementMode();
	}

	bIsPoolActive = true;
	CurrentHealth = MaxHealth;
	ShotsFiredSinceActivation = 0;

	if (bAutoStartFiring)
	{
		StartFiring();
	}
}

void ARLEnemyCharacter::DeactivateForPool()
{
	StopFiring();
	bIsPoolActive = false;
	CurrentHealth = 0.0f;
	ShotsFiredSinceActivation = 0;

	if (AController* EnemyController = GetController())
	{
		EnemyController->StopMovement();
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
}

