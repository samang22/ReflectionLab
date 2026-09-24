#include "Enemies/RLEnemyCharacter.h"

#include "Combat/RLProjectile.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ARLEnemyCharacter::ARLEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(GetRootComponent());
}

void ARLEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	if (ProjectileClass)
	{
		if (URLProjectilePoolSubsystem* PoolSubsystem =
			GetWorld()->GetSubsystem<URLProjectilePoolSubsystem>())
		{
			PoolSubsystem->PrewarmPool(ProjectileClass, ProjectilePoolPrewarmCount);
		}
	}

	if (bAutoStartFiring)
	{
		StartFiring();
	}
}

float ARLEnemyCharacter::TakeDamage(
	float DamageAmount,
	const FDamageEvent& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
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

	const APawn* TargetPawn = UGameplayStatics::GetPlayerPawn(this, 0);
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
		PoolSubsystem->AcquireProjectile(
			ProjectileClass,
			SpawnTransform,
			this,
			this);
	}
}

void ARLEnemyCharacter::Die()
{
	StopFiring();
	Destroy();
}

