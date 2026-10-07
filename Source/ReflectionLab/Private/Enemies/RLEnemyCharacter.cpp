#include "Enemies/RLEnemyCharacter.h"

#include "Animation/RLEnemyAnimInstance.h"
#include "Combat/RLProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/RLEnemyCombatRow.h"
#include "Enemies/Components/RLEnemyAttackComponent.h"
#include "Enemies/Components/RLBossOverloadComponent.h"
#include "Enemies/Components/RLBossLaserComponent.h"
#include "Enemies/Components/RLBossChargeComponent.h"
#include "Enemies/Components/RLBossSummonComponent.h"
#include "Enemies/Components/RLEnemyFeedbackComponent.h"
#include "Enemies/Components/RLEnemyMovementComponent.h"
#include "Enemies/Components/RLEnemyShieldComponent.h"
#include "Enemies/Components/RLEnemySpawnVisualComponent.h"
#include "Enemies/RLEnemyPoolSubsystem.h"
#include "Enemies/RLEnemyAIController.h"
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
	AIControllerClass = ARLEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
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
	SpawnVisualComponent = CreateDefaultSubobject<URLEnemySpawnVisualComponent>(TEXT("SpawnVisualComponent"));

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
	SpawnVisualComponent->OnSpawnFinished.AddUObject(this, &ThisClass::HandleSpawnFinished);
	if (bIsPoolActive)
	{
		BeginSpawnPresentation();
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
	// Personal shields absorb one hit; their visual radius has no area-protection effect.
	ShieldComponent->Configure(ShieldVisual, ShieldVisualRadius, ShieldColor);

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
	const ARLProjectile* Projectile = Cast<ARLProjectile>(DamageCauser);
	const bool bReflectedRally = Projectile && Projectile->IsReflected() && Projectile->IsRallyProjectile();
	auto RejectDamage = [this, DamageAmount, DamageCauser, bReflectedRally](const TCHAR* Reason)
	{
		if (bReflectedRally)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[RallyDamage] rejected=%s enemy=%s projectile=%s requested=%.3f health=%.3f active=%d tutorialInvulnerable=%d"),
				Reason, *GetName(), *GetNameSafe(DamageCauser), DamageAmount,
				HealthComponent->GetCurrentHealth(), bIsPoolActive, bTutorialInvulnerable);
		}
		return 0.0f;
	};
	if (!bIsPoolActive) { return RejectDamage(TEXT("InactiveEnemy")); }
	if (IsSpawning()) { return RejectDamage(TEXT("SpawningEnemy")); }
	if (bTutorialInvulnerable) { return RejectDamage(TEXT("TutorialInvulnerability")); }
	if (HealthComponent->IsDead()) { return RejectDamage(TEXT("DeadEnemy")); }
	if (!FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f) { return RejectDamage(TEXT("InvalidDamage")); }
	if (Projectile && Projectile->IsReflected() && TryAbsorbReflectedProjectile()) { return RejectDamage(TEXT("Shield")); }

	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	// Super broadcasts damage events; a listener may have returned the enemy to the pool.
	if (!bIsPoolActive || HealthComponent->IsDead()) { return RejectDamage(TEXT("DamageEventChangedEnemy")); }
	if (!FMath::IsFinite(AppliedDamage) || AppliedDamage <= 0.0f) { return RejectDamage(TEXT("NoAppliedDamage")); }

	FeedbackComponent->PlayHitSound();
	const float PreviousHealth = HealthComponent->GetCurrentHealth();
	HealthComponent->ApplyDamage(AppliedDamage);
	if (bReflectedRally)
	{
		UE_LOG(LogTemp, Display, TEXT("[RallyDamage] enemy=%s projectile=%s applied=%.3f health=%.3f->%.3f active=%d"),
			*GetName(), *GetNameSafe(DamageCauser), AppliedDamage,
			PreviousHealth, HealthComponent->GetCurrentHealth(), bIsPoolActive);
	}
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
	if (IsSpawning()) { ShieldVisual->SetVisibility(false, true); }
}

bool ARLEnemyCharacter::IsShieldEmitterActive() const
{
	return ShieldComponent->IsEmitterActive();
}

bool ARLEnemyCharacter::TryAbsorbReflectedProjectile()
{
	if (IsSpawning()) { return false; }
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
	if (auto* Summon = FindComponentByClass<URLBossSummonComponent>()) { Summon->Reset(); }
	if (auto* Charge = FindComponentByClass<URLBossChargeComponent>()) { Charge->Cancel(); }
	if (auto* Laser = FindComponentByClass<URLBossLaserComponent>()) { Laser->Cancel(); }
	if (auto* Overload = FindComponentByClass<URLBossOverloadComponent>()) { Overload->Reset(); }
	// Reset attack timers before making this instance active again.
	AttackComponent->StopFiring();
	SpawnVisualComponent->CancelSpawn();
	FeedbackComponent->ResetCombatAnimation();
	SetActorEnableCollision(false);
	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	// Restore gameplay state before the spawn presentation eventually enables collision.
	bIsPoolActive = true;
	bTutorialInvulnerable = false;
	HealthComponent->InitializeHealth(HealthComponent->GetMaxHealth());
	ShieldComponent->SetEmitter(false);
	SetActorHiddenInGame(false);
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	BeginSpawnPresentation();
	// An immediate overlap may have killed and returned this instance to the pool.
	if (!bIsPoolActive) { return; }
	SetActorTickEnabled(true);
}

bool ARLEnemyCharacter::IsSpawning() const
{
	return SpawnVisualComponent->IsSpawning();
}

void ARLEnemyCharacter::BeginSpawnPresentation()
{
	SetActorEnableCollision(false);
	EnemyMovementComponent->ActivateForPool();
	if (SpawnVisualComponent->StartSpawn(GetMesh()))
	{
		// Reset pool combat state now, before tutorial/wave setup can override it.
		AttackComponent->ActivateForPool();
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
	else
	{
		HandleSpawnFinished();
		if (bIsPoolActive) { AttackComponent->ActivateForPool(); }
	}
}

void ARLEnemyCharacter::HandleSpawnFinished()
{
	if (!bIsPoolActive) { return; }
	EnemyMovementComponent->ResumeAfterSpawn();
	ShieldVisual->SetVisibility(ShieldComponent->IsEmitterActive(), true);
	SetActorEnableCollision(true);
	if (bIsPoolActive) { AttackComponent->StartFiring(); }
}

void ARLEnemyCharacter::DeactivateForPool()
{
	if (auto* Summon = FindComponentByClass<URLBossSummonComponent>()) { Summon->Reset(); }
	if (auto* Charge = FindComponentByClass<URLBossChargeComponent>()) { Charge->Cancel(); }
	if (auto* Laser = FindComponentByClass<URLBossLaserComponent>()) { Laser->Cancel(); }
	if (auto* Overload = FindComponentByClass<URLBossOverloadComponent>()) { Overload->Reset(); }
	SpawnVisualComponent->CancelSpawn();
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

