#include "Enemies/Components/RLBossOverloadComponent.h"

#include "Combat/RLProjectile.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Data/RLBossOverloadDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/Components/RLEnemyAttackComponent.h"
#include "Enemies/Components/RLBossLaserComponent.h"
#include "Enemies/Components/RLEnemyMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Components/RLHealthComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

URLBossOverloadComponent::URLBossOverloadComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URLBossOverloadComponent::Initialize(URLHealthComponent* InHealth, USkeletalMeshComponent* InMesh,
	USphereComponent* InVolume, UDecalComponent* InDecal, TSubclassOf<ARLProjectile> InProjectileClass)
{
	Health = InHealth;
	Mesh = InMesh;
	Volume = InVolume;
	Decal = InDecal;
	ProjectileClass = InProjectileClass;
	if (!Settings || !Health || !Mesh || !Volume || !Decal || !ProjectileClass ||
		!Settings->ProjectileDefinition || !Settings->DecalMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("Boss overload configuration incomplete on %s; pattern disabled."), *GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
		return;
	}
	if (!FMath::IsFinite(Settings->MaxHealth) || !FMath::IsFinite(Settings->DamageThreshold) ||
		!FMath::IsFinite(Settings->Duration) || !FMath::IsFinite(Settings->VulnerableDuration) || !FMath::IsFinite(Settings->Radius) ||
		!FMath::IsFinite(Settings->SpinShotInterval) || !FMath::IsFinite(Settings->SpinSpeed) || !FMath::IsFinite(Settings->PulseFrequency) ||
		!FMath::IsFinite(Settings->PulseAmplitude) || !FMath::IsFinite(Settings->HealPerProjectile))
	{
		UE_LOG(LogTemp, Error, TEXT("Boss overload tuning contains non-finite values on %s."), *GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
		return;
	}
	BaseMeshScale = Mesh->GetRelativeScale3D();
	Volume->SetSphereRadius(FMath::Max(1.0f, Settings->Radius));
	Decal->DecalSize = FVector(64.0f, Settings->Radius, Settings->Radius);
	Decal->SetDecalMaterial(Settings->DecalMaterial);
	Decal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	Decal->FadeScreenSize = 0.0f;
	Decal->SetSortOrder(15);
	Decal->SetHiddenInGame(false);
	PreviousHealth = Health->GetCurrentHealth();
	Health->OnHealthChanged.AddUniqueDynamic(this, &ThisClass::HandleHealthChanged);
	Health->SetMaxHealth(FMath::Max(1.0f, Settings->MaxHealth), true);
	if (!Mesh->DoesSocketExist(TEXT("muzzle")))
	{
		UE_LOG(LogTemp, Error, TEXT("Boss %s needs a 'muzzle' socket for overload fire; pattern disabled."), *GetNameSafe(GetOwner()));
		Health->OnHealthChanged.RemoveDynamic(this, &ThisClass::HandleHealthChanged);
		SetComponentTickEnabled(false);
	}
}

void URLBossOverloadComponent::HandleHealthChanged(float NewHealth, float NewMaxHealth)
{
	const float Damage = FMath::Max(0.0f, PreviousHealth - NewHealth);
	PreviousHealth = NewHealth;
	const ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy || !Enemy->IsPoolActive() || NewHealth <= 0.0f)
	{
		Reset();
		return;
	}
	// Healing never advances the threshold. Damage during overload is not queued.
	if (!Settings || bOverloading || bPendingOverload || Enemy->IsSpawning()) { return; }
	AccumulatedDamage += Damage;
	if (AccumulatedDamage >= FMath::Max(0.1f, Settings->DamageThreshold))
	{
		// Finish the triggering hit before opening the absorption area.
		bPendingOverload = true;
	}
}

void URLBossOverloadComponent::BeginOverload()
{
	if (auto* Laser = GetOwner()->FindComponentByClass<URLBossLaserComponent>()) { Laser->Cancel(); }
	bOverloading = true;
	bAbsorbing = true;
	AccumulatedDamage = 0.0f;
	Elapsed = 0.0f;
	UntilNextVolley = 0.0f;
	if (auto* Attack = GetOwner()->FindComponentByClass<URLEnemyAttackComponent>()) { Attack->StopFiring(); }
	if (auto* Movement = GetOwner()->FindComponentByClass<URLEnemyMovementComponent>())
	{
		Movement->SetTutorialMovementLocked(true);
		Movement->SetFacingLocked(true);
	}
	if (auto* Enemy = Cast<ARLEnemyCharacter>(GetOwner())) { Enemy->GetCharacterMovement()->StopMovementImmediately(); }
	// Project onto the actual floor, not from the boss capsule's center.
	const FVector Origin = GetOwner()->GetActorLocation();
	FHitResult GroundHit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(BossAbsorptionGround), false, GetOwner());
	FVector Ground = Origin;
	if (GetWorld()->LineTraceSingleByChannel(GroundHit, Origin + FVector(0, 0, 100),
		Origin - FVector(0, 0, 2000), ECC_WorldStatic, Query))
	{
		Ground = GroundHit.ImpactPoint;
	}
	else if (const auto* Enemy = Cast<ARLEnemyCharacter>(GetOwner()))
	{
		Ground.Z -= Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	Decal->SetWorldLocation(Ground + FVector(0, 0, 30));
	if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Volume->SetWorldLocation(FVector(Origin.X, Origin.Y, PlayerPawn->GetActorLocation().Z));
	}
	Decal->SetVisibility(true);
	Volume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// Also consume returns already inside the area when overload begins.
	TArray<AActor*> Overlapping;
	Volume->GetOverlappingActors(Overlapping, ARLProjectile::StaticClass());
	for (AActor* Actor : Overlapping) { TryAbsorb(Cast<ARLProjectile>(Actor)); }
}

bool URLBossOverloadComponent::OwnsVolume(const UPrimitiveComponent* Component) const
{
	return Component && Component == Volume;
}

bool URLBossOverloadComponent::TryAbsorb(ARLProjectile* Projectile)
{
	if (!bAbsorbing || !Settings || !Health || Health->IsDead() || !Projectile ||
		!Projectile->IsPoolActive() || Projectile->IsFadingOut() || !Projectile->IsReflected()) { return false; }
	const float Radius = Volume->GetScaledSphereRadius() + Projectile->GetCollisionSphere()->GetScaledSphereRadius();
	if (FVector::DistSquared(Projectile->GetActorLocation(), Volume->GetComponentLocation()) > FMath::Square(Radius)) { return false; }
	// Disable collision/fuse through the existing lifecycle before emitting health events.
	Projectile->ReturnToPool();
	Health->RestoreHealth(FMath::Max(0.0f, Settings->HealPerProjectile));
	return true;
}

void URLBossOverloadComponent::FireSpinningShot()
{
	auto* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	auto* Pool = GetWorld()->GetSubsystem<URLProjectilePoolSubsystem>();
	if (!Enemy || !Pool || !Settings || !ProjectileClass) { return; }
	const FName MuzzleSocket(TEXT("muzzle"));
	if (!Mesh || !Mesh->DoesSocketExist(MuzzleSocket)) { return; }
	// Socket X is the authored firing direction. All shots start at the socket,
	// without a synthetic radial offset or a player-height position override.
	const FTransform SocketTransform = Mesh->GetSocketTransform(MuzzleSocket, RTS_World);
	FVector Direction = SocketTransform.GetUnitAxis(EAxis::X).GetSafeNormal2D();
	if (Direction.IsNearlyZero()) { Direction = Enemy->GetActorForwardVector().GetSafeNormal2D(); }
	if (ARLProjectile* Projectile = Pool->AcquireProjectile(ProjectileClass,
		FTransform(Direction.Rotation(), SocketTransform.GetLocation()), Enemy, Enemy))
	{
		Projectile->InitializeFromDefinition(Settings->ProjectileDefinition, nullptr);
		if (auto* Attack = GetOwner()->FindComponentByClass<URLEnemyAttackComponent>()) { Attack->OnShotSpawned.ExecuteIfBound(); }
	}
}

void URLBossOverloadComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const auto* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy || !Enemy->IsPoolActive() || !Health || Health->IsDead()) { Reset(); return; }
	if (bPendingOverload)
	{
		bPendingOverload = false;
		BeginOverload();
	}
	if (!bOverloading) { return; }
	FRotator Rotation = GetOwner()->GetActorRotation();
	Rotation.Yaw = FRotator::NormalizeAxis(Rotation.Yaw + FMath::Max(1.0f, Settings->SpinSpeed) * FMath::Max(0.0f, DeltaTime));
	GetOwner()->SetActorRotation(Rotation);
	Elapsed += FMath::Max(0.0f, DeltaTime);
	const float AbsorptionDuration = FMath::Max(0.1f, Settings->Duration);
	if (Elapsed >= AbsorptionDuration + FMath::Max(0.1f, Settings->VulnerableDuration))
	{
		FinishOverload(true);
		return;
	}
	if (bAbsorbing && Elapsed >= AbsorptionDuration)
	{
		bAbsorbing = false;
		Volume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Decal->SetVisibility(false);
		Mesh->SetRelativeScale3D(BaseMeshScale);
	}
	const float Pulse = FMath::Sin(Elapsed * 2.0f * PI * FMath::Max(0.1f, Settings->PulseFrequency));
	if (bAbsorbing)
	{
		Mesh->SetRelativeScale3D(BaseMeshScale * (1.0f + FMath::Clamp(Settings->PulseAmplitude, 0.0f, 0.5f) * Pulse));
		// Only the visual pulses: collision and absorption radius remain stable.
		Decal->SetVisibility(AbsorptionDuration - Elapsed > 1.0f || FMath::Fmod(Elapsed, 0.2f) < 0.1f);
	}
	UntilNextVolley -= DeltaTime;
	if (UntilNextVolley <= 0.0f)
	{
		FireSpinningShot();
		UntilNextVolley += FMath::Max(0.01f, Settings->SpinShotInterval);
	}
}

void URLBossOverloadComponent::FinishOverload(bool bResumeCombat)
{
	bOverloading = false;
	bAbsorbing = false;
	if (Volume) { Volume->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	if (Decal) { Decal->SetVisibility(false); }
	if (Mesh) { Mesh->SetRelativeScale3D(BaseMeshScale); }
	if (auto* Movement = GetOwner()->FindComponentByClass<URLEnemyMovementComponent>()) { Movement->SetFacingLocked(false); }
	if (bResumeCombat)
	{
		if (auto* Movement = GetOwner()->FindComponentByClass<URLEnemyMovementComponent>()) { Movement->SetTutorialMovementLocked(false); }
		if (auto* Attack = GetOwner()->FindComponentByClass<URLEnemyAttackComponent>()) { Attack->StartFiring(); }
	}
}

void URLBossOverloadComponent::Reset()
{
	FinishOverload(false);
	AccumulatedDamage = 0.0f;
	bPendingOverload = false;
	Elapsed = 0.0f;
	if (Health) { PreviousHealth = Health->GetCurrentHealth(); }
}

void URLBossOverloadComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Reset();
	if (Health) { Health->OnHealthChanged.RemoveDynamic(this, &ThisClass::HandleHealthChanged); }
	Super::EndPlay(EndPlayReason);
}
