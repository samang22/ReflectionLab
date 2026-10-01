#include "Enemies/RLEnemyCharacter.h"
#include "Animation/RLEnemyAnimInstance.h"

#include "Combat/RLExplosionVisual.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Enemies/RLEnemyPoolSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/RLEnemyCombatRow.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
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

	// Enforce facing ownership even when a blueprint has saved rotation defaults.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	UpdateFacingPlayer();

	ApplyCombatConfig();
	CacheBaseCombatValues();
	CurrentHealth = MaxHealth;
	ShieldMaterialInstance = ShieldVisual
		? ShieldVisual->CreateDynamicMaterialInstance(0)
		: nullptr;
	if (ShieldMaterialInstance)
	{
		ShieldMaterialInstance->SetVectorParameterValue(TEXT("ProjectileColor"), ShieldColor);
		ShieldMaterialInstance->SetScalarParameterValue(TEXT("EmissiveIntensity"), 8.0f);
		ShieldMaterialInstance->SetScalarParameterValue(TEXT("FadeOpacity"), 0.12f);
	}
	UpdateShieldVisual();

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

void ARLEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateFacingPlayer();
}

void ARLEnemyCharacter::UpdateFacingPlayer()
{
	if (!bIsPoolActive)
	{
		return;
	}

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!IsValid(PlayerPawn))
	{
		return;
	}

	const FVector ToPlayer = (PlayerPawn->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!ToPlayer.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.0f, ToPlayer.Rotation().Yaw, 0.0f));
	}
}

void ARLEnemyCharacter::CacheBaseCombatValues()
{
	BaseAttackInterval = FMath::Max(0.1f, AttackInterval);
	BaseShotsPerBurst = FMath::Max(1, ShotsPerBurst);
	BaseTimeBetweenShots = FMath::Max(0.01f, TimeBetweenShots);
}

void ARLEnemyCharacter::ApplyDifficultyPhase(const FRLDifficultyPhase& DifficultyPhase)
{
	AttackInterval = BaseAttackInterval *
		FMath::Max(0.1f, DifficultyPhase.AttackIntervalMultiplier);
	ShotsPerBurst = DifficultyPhase.ShotsPerBurstOverride > 0
		? DifficultyPhase.ShotsPerBurstOverride
		: BaseShotsPerBurst;
	TimeBetweenShots = BaseTimeBetweenShots *
		FMath::Max(0.1f, DifficultyPhase.TimeBetweenShotsMultiplier);
	DefaultProjectileDefinition = DifficultyPhase.DefaultProjectileDefinition;
	ProjectileRules = DifficultyPhase.ProjectileRules;

	if (bIsPoolActive && bAutoStartFiring)
	{
		StopFiring();
		StartFiring();
	}
}

void ARLEnemyCharacter::ApplyWaveDefinition(const FRLWaveDefinition& WaveDefinition)
{
	AttackInterval = BaseAttackInterval *
		FMath::Max(0.1f, WaveDefinition.AttackIntervalMultiplier);
	ShotsPerBurst = WaveDefinition.ShotsPerBurstOverride > 0
		? WaveDefinition.ShotsPerBurstOverride
		: BaseShotsPerBurst;
	TimeBetweenShots = BaseTimeBetweenShots *
		FMath::Max(0.1f, WaveDefinition.TimeBetweenShotsMultiplier);
	DefaultProjectileDefinition = WaveDefinition.DefaultProjectileDefinition;
	ProjectileRules = WaveDefinition.ProjectileRules;

	if (bIsPoolActive && bAutoStartFiring)
	{
		StopFiring();
		StartFiring();
	}
}

void ARLEnemyCharacter::SetTutorialMovementLocked(bool bLocked)
{
	bTutorialMovementLocked = bLocked;
	SetActorTickEnabled(bIsPoolActive);

	if (!bIsPoolActive)
	{
		return;
	}

	if (AController* EnemyController = GetController())
	{
		EnemyController->StopMovement();
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		if (bTutorialMovementLocked)
		{
			Movement->DisableMovement();
		}
		else
		{
			Movement->SetDefaultMovementMode();
		}
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
	if (bTutorialInvulnerable)
	{
		return 0.0f;
	}
	if (const ARLProjectile* Projectile = Cast<ARLProjectile>(DamageCauser))
	{
		if (Projectile->IsReflected() && TryAbsorbReflectedProjectile())
		{
			return 0.0f;
		}
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
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			HitSound,
			GetActorLocation(),
			FMath::Max(0.0f, HitSoundVolume),
			FMath::FRandRange(
				FMath::Min(HitSoundPitchMin, HitSoundPitchMax),
				FMath::Max(HitSoundPitchMin, HitSoundPitchMax)));
	}

	if (CurrentHealth <= 0.0f)
	{
		Die();
	}

	return AppliedDamage;
}

void ARLEnemyCharacter::StartFiring()
{
	if (!GetWorld() || AttackTimerHandle.IsValid() || bTutorialCombatControlled)
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

	++ShotsFiredSinceActivation;
	const FRLProjectileSpawnRule* SelectedRule = nullptr;
	for (const FRLProjectileSpawnRule& Rule : ProjectileRules)
	{
		if (Rule.ProjectileDefinition && Rule.ShotInterval > 0 &&
			ShotsFiredSinceActivation % Rule.ShotInterval == 0)
		{
			SelectedRule = &Rule;
			break;
		}
	}

	SpawnProjectile(
		SelectedRule ? SelectedRule->ProjectileDefinition.Get() : DefaultProjectileDefinition.Get(),
		SelectedRule ? SelectedRule->ShotPattern : ERLShotPattern::Single,
		SelectedRule ? SelectedRule->CrossLateralOffset : 90.0f,
		SelectedRule ? SelectedRule->CrossTargetOffset : 110.0f);
}

bool ARLEnemyCharacter::SpawnProjectile(
	URLProjectileDefinitionDataAsset* ProjectileDefinition,
	ERLShotPattern ShotPattern,
	float CrossLateralOffset,
	float CrossTargetOffset)
{
	if (!ProjectileClass || !MuzzlePoint || !GetWorld() || !ProjectileDefinition)
	{
		return false;
	}

	APawn* TargetPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	URLProjectilePoolSubsystem* PoolSubsystem =
		GetWorld()->GetSubsystem<URLProjectilePoolSubsystem>();
	if (!TargetPawn || !PoolSubsystem)
	{
		return false;
	}

	const FVector SpawnLocation = MuzzlePoint->GetComponentLocation();
	const FRotator SpawnRotation =
		(TargetPawn->GetActorLocation() - SpawnLocation).Rotation();
	auto SpawnConfiguredProjectile = [
		this,
		PoolSubsystem,
		ProjectileDefinition,
		TargetPawn](const FTransform& SpawnTransform)
	{
		ARLProjectile* Projectile = PoolSubsystem->AcquireProjectile(
			ProjectileClass, SpawnTransform, this, this);
		if (Projectile)
		{
			Projectile->InitializeFromDefinition(ProjectileDefinition, TargetPawn);
		}
		return Projectile != nullptr;
	};

	if (ShotPattern == ERLShotPattern::Cross)
	{
		bool bSpawnedAny = false;
		const FVector RightDirection = SpawnRotation.RotateVector(FVector::RightVector);
		for (const float Side : {-1.0f, 1.0f})
		{
			const FVector CrossSpawnLocation = SpawnLocation +
				RightDirection * Side * FMath::Max(0.0f, CrossLateralOffset);
			const FVector CrossTargetLocation = TargetPawn->GetActorLocation() -
				RightDirection * Side * FMath::Max(0.0f, CrossTargetOffset);
			const FVector CrossDirection =
				(CrossTargetLocation - CrossSpawnLocation).GetSafeNormal();
			bSpawnedAny |= SpawnConfiguredProjectile(
				FTransform(CrossDirection.Rotation(), CrossSpawnLocation));
		}
		if (bSpawnedAny)
		{
			if (URLEnemyAnimInstance* AnimInstance = Cast<URLEnemyAnimInstance>(GetMesh()->GetAnimInstance()))
			{
				AnimInstance->PlayShootAnimation();
			}
		}
		return bSpawnedAny;
	}

	const bool bSpawned = SpawnConfiguredProjectile(FTransform(SpawnRotation, SpawnLocation));
	if (bSpawned)
	{
		if (URLEnemyAnimInstance* AnimInstance = Cast<URLEnemyAnimInstance>(GetMesh()->GetAnimInstance()))
		{
			AnimInstance->PlayShootAnimation();
		}
	}
	return bSpawned;
}

void ARLEnemyCharacter::SetTutorialCombatControlled(bool bControlled)
{
	bTutorialCombatControlled = bControlled;
	StopFiring();
	if (!bTutorialCombatControlled && bIsPoolActive && bAutoStartFiring)
	{
		StartFiring();
	}
}

void ARLEnemyCharacter::SetTutorialInvulnerable(bool bInvulnerable)
{
	bTutorialInvulnerable = bInvulnerable;
}

bool ARLEnemyCharacter::FireTutorialProjectile(
	URLProjectileDefinitionDataAsset* ProjectileDefinition)
{
	return bIsPoolActive && bTutorialCombatControlled &&
		SpawnProjectile(ProjectileDefinition);
}

void ARLEnemyCharacter::SetShieldEmitter(bool bEnabled)
{
	bShieldEmitterActive = bEnabled && bIsPoolActive;
	UpdateShieldVisual();
}

bool ARLEnemyCharacter::TryAbsorbReflectedProjectile()
{
	if (bShieldEmitterActive)
	{
		SetShieldEmitter(false);
		UE_LOG(LogTemp, Display, TEXT("Shield broken on %s."), *GetName());
		return true;
	}

	return IsProtectedByShield();
}

bool ARLEnemyCharacter::IsProtectedByShield() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	for (TActorIterator<ARLEnemyCharacter> Iterator(World); Iterator; ++Iterator)
	{
		const ARLEnemyCharacter* ShieldEnemy = *Iterator;
		if (!IsValid(ShieldEnemy) || ShieldEnemy == this ||
			!ShieldEnemy->bIsPoolActive || !ShieldEnemy->bShieldEmitterActive)
		{
			continue;
		}

		if (FVector::DistSquared2D(GetActorLocation(), ShieldEnemy->GetActorLocation()) <=
			FMath::Square(FMath::Max(1.0f, ShieldEnemy->ShieldProtectionRadius)))
		{
			return true;
		}
	}

	return false;
}

void ARLEnemyCharacter::UpdateShieldVisual()
{
	if (!ShieldVisual)
	{
		return;
	}

	const float SphereMeshRadius = 50.0f;
	const float ShieldScale = FMath::Max(1.0f, ShieldVisualRadius) / SphereMeshRadius;
	ShieldVisual->SetRelativeScale3D(FVector(ShieldScale));
	ShieldVisual->SetVisibility(bShieldEmitterActive && bIsPoolActive, true);
}

void ARLEnemyCharacter::Die()
{
	SpawnDeathEffect();
	ReturnToPool();
}

void ARLEnemyCharacter::SpawnDeathEffect()
{
	UWorld* World = GetWorld();
	if (!World || !DeathEffectShardMesh || !DeathEffectShardMaterial)
	{
		return;
	}

	FVector GroundLocation = GetActorLocation();
	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyDeathEffectGroundTrace), false, this);
	const FVector TraceStart = GroundLocation + FVector(0.0f, 0.0f, 200.0f);
	const FVector TraceEnd = GroundLocation - FVector(0.0f, 0.0f, 600.0f);
	if (World->LineTraceSingleByChannel(
		GroundHit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams))
	{
		GroundLocation = GroundHit.ImpactPoint;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARLExplosionVisual* DeathEffect = World->SpawnActor<ARLExplosionVisual>(
		ARLExplosionVisual::StaticClass(),
		GroundLocation,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (DeathEffect)
	{
		DeathEffect->Initialize(
			DeathEffectShardMesh,
			DeathEffectShardMaterial,
			DeathEffectShardScale,
			FMath::Max(1.0f, DeathEffectRadius),
			DeathEffectColor,
			false);
	}
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
	if (URLEnemyAnimInstance* AnimInstance = Cast<URLEnemyAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AnimInstance->ResetCombatAnimation();
	}
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
	UpdateFacingPlayer();
	bShieldEmitterActive = false;
	bTutorialCombatControlled = false;
	bTutorialInvulnerable = false;
	CurrentHealth = MaxHealth;
	ShotsFiredSinceActivation = 0;
	UpdateShieldVisual();

	if (bAutoStartFiring)
	{
		StartFiring();
	}
}

void ARLEnemyCharacter::DeactivateForPool()
{
	if (URLEnemyAnimInstance* AnimInstance = Cast<URLEnemyAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AnimInstance->ResetCombatAnimation();
	}
	StopFiring();
	bIsPoolActive = false;
	bShieldEmitterActive = false;
	bTutorialMovementLocked = false;
	bTutorialCombatControlled = false;
	bTutorialInvulnerable = false;
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
	UpdateShieldVisual();
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
}

