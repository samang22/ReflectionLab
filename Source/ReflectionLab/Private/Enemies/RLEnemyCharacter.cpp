#include "Enemies/RLEnemyCharacter.h"

#include "Animation/RLEnemyAnimInstance.h"
#include "Combat/RLProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/RLEnemyCombatRow.h"
#include "Enemies/Components/RLEnemyAttackComponent.h"
#include "Enemies/Components/RLEnemyFeedbackComponent.h"
#include "Enemies/Components/RLEnemyMovementComponent.h"
#include "Enemies/Components/RLEnemyShieldComponent.h"
#include "Enemies/RLEnemyPoolSubsystem.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "Player/Components/RLHealthComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ARLEnemyCharacter::ARLEnemyCharacter()
{
	static ConstructorHelpers::FObjectFinder<USoundBase> HitSoundFinder(
		TEXT("/Game/ReflectionLab/Audio/SFX/Combat/EnemyHit/SFX_EnemyHit.SFX_EnemyHit"));
	HitSound = HitSoundFinder.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DeathEffectShardMeshFinder(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	DeathEffectShardMesh = DeathEffectShardMeshFinder.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DeathEffectShardMaterialFinder(
		TEXT("/Game/ReflectionLab/Art/Materials/Projectiles/"
			 "MI_Projectile_Hostile.MI_Projectile_Hostile"));
	DeathEffectShardMaterial = DeathEffectShardMaterialFinder.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ShieldMeshFinder(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShieldMaterialFinder(
		TEXT("/Game/ReflectionLab/Art/Materials/Projectiles/"
			 "MI_Projectile_Reflected.MI_Projectile_Reflected"));

	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetMesh()->SetReceivesDecals(false);
	GetMesh()->SetAnimInstanceClass(URLEnemyAnimInstance::StaticClass());

	HealthComponent = CreateDefaultSubobject<URLHealthComponent>(TEXT("HealthComponent"));
	AttackComponent = CreateDefaultSubobject<URLEnemyAttackComponent>(TEXT("AttackComponent"));
	EnemyMovementComponent = CreateDefaultSubobject<URLEnemyMovementComponent>(TEXT("EnemyMovementComponent"));
	ShieldComponent = CreateDefaultSubobject<URLEnemyShieldComponent>(TEXT("ShieldComponent"));
	FeedbackComponent = CreateDefaultSubobject<URLEnemyFeedbackComponent>(TEXT("FeedbackComponent"));

	// Scene components stay actor-owned to preserve blueprint transforms and
	// avoid attachments to nested component templates.
	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(GetRootComponent());

	ShieldVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldVisual"));
	ShieldVisual->SetupAttachment(GetRootComponent());
	ShieldVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldVisual->SetCanEverAffectNavigation(false);
	ShieldVisual->SetCastShadow(false);
	ShieldVisual->SetReceivesDecals(false);
	ShieldVisual->SetVisibility(false, true);
	if (ShieldMeshFinder.Succeeded())
	{
		ShieldVisual->SetStaticMesh(ShieldMeshFinder.Object);
	}
	if (ShieldMaterialFinder.Succeeded())
	{
		ShieldVisual->SetMaterial(0, ShieldMaterialFinder.Object);
	}
}

void ARLEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent->OnHealthChanged.AddDynamic(this, &ThisClass::HandleHealthChanged);
	HealthComponent->OnDeath.AddDynamic(this, &ThisClass::HandleDeath);
	AttackComponent->OnFireShot.BindUObject(this, &ThisClass::Fire);
	AttackComponent->OnShotSpawned.BindUObject(FeedbackComponent.Get(), &URLEnemyFeedbackComponent::PlayShootAnimation);
	ConfigureEnemyComponents();
	if (bIsPoolActive)
	{
		AttackComponent->ActivateForPool();
	}
}

void ARLEnemyCharacter::ConfigureEnemyComponents()
{
	FRLEnemyCombatRow Config;
	if (CombatConfig.DataTable && !CombatConfig.RowName.IsNone())
	{
		const FRLEnemyCombatRow* Row = CombatConfig.GetRow<FRLEnemyCombatRow>(
			TEXT("Enemy combat configuration"));
		if (Row) { Config = *Row; }
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Enemy combat config row '%s' could not be loaded for %s."),
				*CombatConfig.RowName.ToString(), *GetName());
		}
	}

	HealthComponent->InitializeHealth(FMath::Max(1.0f, Config.MaxHealth));
	if (!bIsPoolActive) { HealthComponent->ClearHealth(); }
	AttackComponent->Configure(Config, ProjectileClass, MuzzlePoint, bAutoStartFiring);
	EnemyMovementComponent->InitializeMovement();
	ShieldComponent->Configure(ShieldVisual, ShieldProtectionRadius, ShieldVisualRadius, ShieldColor);

	FRLEnemyFeedbackSettings Feedback;
	Feedback.HitSound = HitSound;
	Feedback.HitSoundVolume = HitSoundVolume;
	Feedback.HitSoundPitchMin = HitSoundPitchMin;
	Feedback.HitSoundPitchMax = HitSoundPitchMax;
	Feedback.DeathEffectShardMesh = DeathEffectShardMesh;
	Feedback.DeathEffectShardMaterial = DeathEffectShardMaterial;
	Feedback.DeathEffectShardScale = DeathEffectShardScale;
	Feedback.DeathEffectRadius = DeathEffectRadius;
	Feedback.DeathEffectColor = DeathEffectColor;
	FeedbackComponent->Configure(Feedback);
}

void ARLEnemyCharacter::HandleHealthChanged(float NewHealth, float NewMaxHealth)
{
	// Compatibility mirror for existing blueprint readers, never the source of truth.
	CurrentHealth = NewHealth;
}

void ARLEnemyCharacter::HandleDeath()
{
	if (bIsPoolActive) { Die(); }
}

float ARLEnemyCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (!bIsPoolActive || bTutorialInvulnerable || HealthComponent->IsDead() ||
		!FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}
	if (const ARLProjectile* Projectile = Cast<ARLProjectile>(DamageCauser))
	{
		if (Projectile->IsReflected() && TryAbsorbReflectedProjectile()) { return 0.0f; }
	}

	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	// Super broadcasts damage events; a listener may have returned the enemy to the pool.
	if (!bIsPoolActive || HealthComponent->IsDead() ||
		!FMath::IsFinite(AppliedDamage) || AppliedDamage <= 0.0f)
	{
		return 0.0f;
	}

	FeedbackComponent->PlayHitSound();
	HealthComponent->ApplyDamage(AppliedDamage);
	return AppliedDamage;
}

void ARLEnemyCharacter::ApplyDifficultyPhase(const FRLDifficultyPhase& DifficultyPhase)
{
	AttackComponent->ApplyDifficultyPhase(DifficultyPhase);
}

void ARLEnemyCharacter::ApplyWaveDefinition(const FRLWaveDefinition& WaveDefinition)
{
	AttackComponent->ApplyWaveDefinition(WaveDefinition);
}

void ARLEnemyCharacter::StartFiring()
{
	AttackComponent->StartFiring();
}

void ARLEnemyCharacter::StopFiring()
{
	AttackComponent->StopFiring();
}

void ARLEnemyCharacter::Fire()
{
	AttackComponent->Fire();
}

void ARLEnemyCharacter::SetTutorialMovementLocked(bool bLocked)
{
	EnemyMovementComponent->SetTutorialMovementLocked(bLocked);
}

void ARLEnemyCharacter::SetTutorialCombatControlled(bool bControlled)
{
	AttackComponent->SetTutorialCombatControlled(bControlled);
}

void ARLEnemyCharacter::SetTutorialInvulnerable(bool bInvulnerable)
{
	bTutorialInvulnerable = bInvulnerable;
}

bool ARLEnemyCharacter::FireTutorialProjectile(URLProjectileDefinitionDataAsset* ProjectileDefinition)
{
	return AttackComponent->FireTutorialProjectile(ProjectileDefinition);
}

ARLExpandingRingAttack* ARLEnemyCharacter::FireTutorialRingAttack()
{
	return AttackComponent->FireTutorialRingAttack();
}

void ARLEnemyCharacter::SetShieldEmitter(bool bEnabled)
{
	ShieldComponent->SetEmitter(bEnabled);
}

bool ARLEnemyCharacter::IsShieldEmitterActive() const
{
	return ShieldComponent->IsEmitterActive();
}

bool ARLEnemyCharacter::TryAbsorbReflectedProjectile()
{
	return ShieldComponent->TryAbsorbReflectedProjectile();
}

void ARLEnemyCharacter::Die()
{
	FeedbackComponent->SpawnDeathEffect();
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
	// Reset attack timers before making this instance active again.
	AttackComponent->StopFiring();
	FeedbackComponent->ResetCombatAnimation();
	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	bIsPoolActive = true;
	bTutorialInvulnerable = false;
	HealthComponent->InitializeHealth(HealthComponent->GetMaxHealth());
	ShieldComponent->SetEmitter(false);
	EnemyMovementComponent->ActivateForPool();
	AttackComponent->ActivateForPool();
}

void ARLEnemyCharacter::DeactivateForPool()
{
	AttackComponent->DeactivateForPool();
	FeedbackComponent->ResetCombatAnimation();
	bIsPoolActive = false;
	bTutorialInvulnerable = false;
	HealthComponent->ClearHealth();
	ShieldComponent->SetEmitter(false);
	EnemyMovementComponent->DeactivateForPool();
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
}

