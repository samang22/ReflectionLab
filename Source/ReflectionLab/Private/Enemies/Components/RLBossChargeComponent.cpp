#include "Enemies/Components/RLBossChargeComponent.h"

#include "Combat/RLProjectile.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/RLBossChargeDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/Components/RLBossAimComponent.h"
#include "Enemies/Components/RLBossChargeMath.h"
#include "Enemies/Components/RLBossLaserComponent.h"
#include "Enemies/Components/RLBossOverloadComponent.h"
#include "Enemies/Components/RLEnemyAttackComponent.h"
#include "Enemies/Components/RLEnemyMovementComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/RLPlayerCharacter.h"

URLBossChargeComponent::URLBossChargeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URLBossChargeComponent::Initialize(USkeletalMeshComponent* InMesh, UDecalComponent* InDecal,
	TSubclassOf<ARLProjectile> InProjectileClass)
{
	Mesh = InMesh;
	Decal = InDecal;
	ProjectileClass = InProjectileClass;
	if (!Settings || !Mesh || !Decal || !ProjectileClass || !Settings->ProjectileDefinition ||
		!Settings->DecalMaterial || !Mesh->DoesSocketExist(TEXT("muzzle")))
	{
		UE_LOG(LogTemp, Error, TEXT("Boss %s charge settings/socket incomplete; charge disabled."), *GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
		return;
	}
	for (const float Value : {Settings->PreparationDuration, Settings->Distance, Settings->Speed,
		Settings->BrakingDuration, Settings->RecoveryDuration, Settings->Cooldown,
		Settings->ContactDamage, Settings->CounterSpreadDegrees})
	{
		if (!FMath::IsFinite(Value))
		{
			UE_LOG(LogTemp, Error, TEXT("Boss %s charge tuning contains non-finite values."), *GetNameSafe(GetOwner()));
			SetComponentTickEnabled(false);
			return;
		}
	}
	Decal->SetDecalMaterial(Settings->DecalMaterial);
	Decal->FadeScreenSize = 0.0f;
	Decal->SetSortOrder(17);
	Cancel();
}

void URLBossChargeComponent::BeginPreparation()
{
	Phase = EPhase::Preparing;
	Remaining = FMath::Max(0.1f, Settings->PreparationDuration);
	bContactAttempted = false;
	if (auto* Attack = GetOwner()->FindComponentByClass<URLEnemyAttackComponent>()) { Attack->StopFiring(); }
	if (auto* Movement = GetOwner()->FindComponentByClass<URLEnemyMovementComponent>())
	{
		Movement->SetTutorialMovementLocked(true);
		Movement->SetFacingLocked(true);
	}
	Decal->SetVisibility(true);
	if (!UpdatePreview()) { Cancel(true); }
}

bool URLBossChargeComponent::UpdatePreview()
{
	const auto* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Enemy || !PlayerPawn) { return false; }
	if (auto* Aim = GetOwner()->FindComponentByClass<URLBossAimComponent>()) { Aim->AimAtPlayer(TEXT("muzzle")); }
	const FVector Origin = Enemy->GetActorLocation();
	ChargeDirection = (PlayerPawn->GetActorLocation() - Origin).GetSafeNormal2D();
	if (ChargeDirection.IsNearlyZero()) { ChargeDirection = Enemy->GetActorForwardVector().GetSafeNormal2D(); }
	const FVector DesiredEnd = Origin + ChargeDirection * FMath::Max(1.0f, Settings->Distance);
	// Match the warning to the same swept capsule used by the actual charge.
	FHitResult Obstacle;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(BossChargePreview), false, Enemy);
	Query.AddIgnoredActor(PlayerPawn);
	FVector End = DesiredEnd;
	if (GetWorld()->SweepSingleByChannel(Obstacle, Origin, DesiredEnd, FQuat::Identity,
		Enemy->GetCapsuleComponent()->GetCollisionObjectType(), Enemy->GetCapsuleComponent()->GetCollisionShape(), Query))
	{
		End = Obstacle.Location;
	}
	FVector Center = (Origin + End) * 0.5f;
	FHitResult Floor;
	if (GetWorld()->LineTraceSingleByChannel(Floor, Center + FVector(0, 0, 100),
		Center - FVector(0, 0, 2000), ECC_WorldStatic, Query)) { Center.Z = Floor.ImpactPoint.Z + 30.0f; }
	else { Center.Z = Origin.Z - Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 30.0f; }
	Decal->SetWorldLocation(Center);
	Decal->SetWorldRotation(FRotator(-90.0f, ChargeDirection.Rotation().Yaw, 0.0f));
	Decal->DecalSize = FVector(64.0f, Enemy->GetCapsuleComponent()->GetScaledCapsuleRadius(),
		FMath::Max(1.0f, FVector::Dist2D(Origin, End)) * 0.5f);
	return true;
}

void URLBossChargeComponent::UpdateCharge(float DeltaTime)
{
	auto* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	const FVector Start = Enemy->GetActorLocation();
	const float Step = FMath::Min(TravelRemaining, FMath::Max(1.0f, Settings->Speed) * DeltaTime);
	FHitResult Hit;
	Enemy->SetActorLocation(Start + ChargeDirection * Step, true, &Hit);
	// Swept root collision prevents tunnelling and stops the charge at obstacles.
	if (!Enemy->IsPoolActive()) { Cancel(); return; }
	const FVector End = Enemy->GetActorLocation();
	// Translation is swept explicitly while navigation is locked; expose actual
	// speed to the existing locomotion AnimInstance without enabling AI movement.
	Enemy->GetCharacterMovement()->Velocity = (End - Start) / DeltaTime;
	TravelRemaining = FMath::Max(0.0f, TravelRemaining - FVector::Dist2D(Start, End));
	if (!bContactAttempted)
	{
		if (auto* PlayerCharacter = Cast<ARLPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			const float Radius = Enemy->GetCapsuleComponent()->GetScaledCapsuleRadius() +
				PlayerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
			if (RLBossCharge::IsWithinSweptContact(Start, End, PlayerCharacter->GetActorLocation(), Radius + 1.0f))
			{
				bContactAttempted = true;
				UGameplayStatics::ApplyDamage(PlayerCharacter, FMath::Max(0.0f, Settings->ContactDamage),
					Enemy->GetController(), Enemy, nullptr);
			}
		}
	}
	if (Hit.bBlockingHit || TravelRemaining <= 1.0f || FVector::DistSquared2D(Start, End) < 0.01f)
	{
		Phase = EPhase::Braking;
		Enemy->GetCharacterMovement()->StopMovementImmediately();
		Remaining = FMath::Max(0.1f, Settings->BrakingDuration);
		Decal->SetVisibility(false);
	}
}

void URLBossChargeComponent::FireCounterShots()
{
	auto* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	auto* Pool = GetWorld()->GetSubsystem<URLProjectilePoolSubsystem>();
	if (!Enemy || !Pool || !Mesh->DoesSocketExist(TEXT("muzzle"))) { return; }
	const FTransform Socket = Mesh->GetSocketTransform(TEXT("muzzle"), RTS_World);
	FVector Forward = Socket.GetUnitAxis(EAxis::X).GetSafeNormal2D();
	if (Forward.IsNearlyZero()) { Forward = ChargeDirection; }
	const int32 Count = FMath::Clamp(Settings->CounterShotCount, 1, 31);
	const float Spread = FMath::Clamp(Settings->CounterSpreadDegrees, 0.0f, 180.0f);
	bool bSpawned = false;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Angle = RLBossCharge::GetFanAngle(Index, Count, Spread);
		const FVector Direction = Forward.RotateAngleAxis(Angle, FVector::UpVector);
		if (ARLProjectile* Projectile = Pool->AcquireProjectile(ProjectileClass,
			FTransform(Direction.Rotation(), Socket.GetLocation()), Enemy, Enemy))
		{
			Projectile->InitializeFromDefinition(Settings->ProjectileDefinition, nullptr);
			bSpawned = true;
		}
	}
	if (bSpawned)
	{
		if (auto* Attack = Enemy->FindComponentByClass<URLEnemyAttackComponent>()) { Attack->OnShotSpawned.ExecuteIfBound(); }
	}
}

void URLBossChargeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const auto* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy || !Enemy->IsPoolActive() || Enemy->IsSpawning()) { Cancel(); return; }
	const auto* Overload = GetOwner()->FindComponentByClass<URLBossOverloadComponent>();
	if (Overload && Overload->WantsOverload()) { if (IsActive()) { Cancel(); } return; }
	const auto* Laser = GetOwner()->FindComponentByClass<URLBossLaserComponent>();
	if (Laser && (Laser->IsPreparing() || Laser->IsFiring() || Laser->IsFading())) { return; }
	const float StepTime = FMath::Max(0.0f, DeltaTime);
	Remaining -= StepTime;
	if (Phase == EPhase::Waiting)
	{
		if (Remaining <= 0.0f) { BeginPreparation(); }
	}
	else if (Phase == EPhase::Preparing)
	{
		if (!UpdatePreview()) { Cancel(true); return; }
		if (Remaining <= 0.0f)
		{
			Phase = EPhase::Charging;
			TravelRemaining = FMath::Max(1.0f, Settings->Distance);
			Decal->SetVisibility(false);
		}
	}
	else if (Phase == EPhase::Charging) { if (StepTime > 0.0f) { UpdateCharge(StepTime); } }
	else if (Phase == EPhase::Braking)
	{
		// Reacquire the player after passing them; shots still follow socket +X.
		if (auto* Aim = GetOwner()->FindComponentByClass<URLBossAimComponent>()) { Aim->AimAtPlayer(TEXT("muzzle")); }
		if (Remaining <= 0.0f)
		{
			FireCounterShots();
			Phase = EPhase::Recovering;
			Remaining = FMath::Max(0.1f, Settings->RecoveryDuration);
		}
	}
	else if (Remaining <= 0.0f) { Cancel(true); }
}

void URLBossChargeComponent::Cancel(bool bResumeCombat)
{
	const bool bWasActive = IsActive();
	Phase = EPhase::Waiting;
	Remaining = Settings ? FMath::Max(0.1f, Settings->Cooldown) : 8.0f;
	TravelRemaining = 0.0f;
	bContactAttempted = false;
	if (Decal) { Decal->SetVisibility(false); }
	if (!bWasActive) { return; }
	if (auto* Enemy = Cast<ARLEnemyCharacter>(GetOwner())) { Enemy->GetCharacterMovement()->StopMovementImmediately(); }
	if (auto* Movement = GetOwner()->FindComponentByClass<URLEnemyMovementComponent>())
	{
		Movement->SetFacingLocked(false);
		if (bResumeCombat) { Movement->SetTutorialMovementLocked(false); }
	}
	if (bResumeCombat)
	{
		if (auto* Attack = GetOwner()->FindComponentByClass<URLEnemyAttackComponent>()) { Attack->StartFiring(); }
	}
}

void URLBossChargeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Cancel();
	Super::EndPlay(EndPlayReason);
}
