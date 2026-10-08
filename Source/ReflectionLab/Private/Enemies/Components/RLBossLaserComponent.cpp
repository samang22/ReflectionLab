#include "Enemies/Components/RLBossLaserComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Data/RLBossLaserDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/Components/RLBossOverloadComponent.h"
#include "Enemies/Components/RLBossAimComponent.h"
#include "Enemies/Components/RLBossChargeComponent.h"
#include "Enemies/Components/RLEnemyMovementComponent.h"
#include "Enemies/Components/RLEnemyAttackComponent.h"
#include "Player/RLPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"

URLBossLaserComponent::URLBossLaserComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URLBossLaserComponent::Initialize(USkeletalMeshComponent* InMesh, UStaticMeshComponent* InBeam, UDecalComponent* InDecal)
{
	Mesh = InMesh; Beam = InBeam; Decal = InDecal;
	if (!Settings || !Mesh || !Beam || !Decal || !Mesh->DoesSocketExist(TEXT("laser")) ||
		!Settings->BeamMaterial || !Settings->DecalMaterial ||
		!FMath::IsFinite(Settings->PreparationDuration) || !FMath::IsFinite(Settings->FiringDuration) ||
		!FMath::IsFinite(Settings->FadeOutDuration) || !FMath::IsFinite(Settings->Cooldown) || !FMath::IsFinite(Settings->Length) ||
		!FMath::IsFinite(Settings->Width) || !FMath::IsFinite(Settings->Damage) || !FMath::IsFinite(Settings->DamageInterval))
	{
		UE_LOG(LogTemp, Error, TEXT("Boss %s laser settings/socket invalid; laser disabled."), *GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
		return;
	}
	Beam->SetMaterial(0, Settings->BeamMaterial);
	BeamMaterialInstance = Beam->CreateDynamicMaterialInstance(0);
	Decal->SetDecalMaterial(Settings->DecalMaterial);
	DecalMaterialInstance = Decal->CreateDynamicMaterialInstance();
	Decal->FadeScreenSize = 0.0f;
	Decal->SetSortOrder(16);
	Cancel();
}

void URLBossLaserComponent::BeginPreparation()
{
	Phase = EPhase::Preparing;
	UpdateVisualOpacity(1.0f);
	Remaining = FMath::Max(0.1f, Settings->PreparationDuration);
	if (auto* Attack = GetOwner()->FindComponentByClass<URLEnemyAttackComponent>()) { Attack->StopFiring(); }
	if (auto* Movement = GetOwner()->FindComponentByClass<URLEnemyMovementComponent>())
	{
		Movement->SetTutorialMovementLocked(true);
		Movement->UpdateFacingPlayer();
	}
	Beam->SetVisibility(false);
	Decal->SetVisibility(true);
	UpdatePreparationPreview();
}

void URLBossLaserComponent::UpdatePreparationPreview()
{
	// Only BeginFiring may fully fill the decal, on the same frame as damage starts.
	UpdateDecalProgress(FMath::Clamp(1.0f - Remaining / FMath::Max(0.1f, Settings->PreparationDuration), 0.0f, 0.99f));
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn) { Cancel(true); return; }
	if (auto* Aim = GetOwner()->FindComponentByClass<URLBossAimComponent>()) { Aim->AimAtPlayer(TEXT("laser")); }
	LockedDirection = Mesh->GetSocketTransform(TEXT("laser"), RTS_World).GetUnitAxis(EAxis::X).GetSafeNormal2D();
	if (LockedDirection.IsNearlyZero()) { LockedDirection = GetOwner()->GetActorForwardVector().GetSafeNormal2D(); }
	UpdateBeam(0.0f, false);
}

void URLBossLaserComponent::UpdateDecalProgress(float Progress)
{
	if (DecalMaterialInstance)
	{
		DecalMaterialInstance->SetScalarParameterValue(TEXT("FillProgress"), Progress);
	}
}

void URLBossLaserComponent::BeginFiring()
{
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn) { Cancel(true); return; }
	LockedDirection = Mesh->GetSocketTransform(TEXT("laser"), RTS_World).GetUnitAxis(EAxis::X).GetSafeNormal2D();
	if (LockedDirection.IsNearlyZero()) { LockedDirection = GetOwner()->GetActorForwardVector().GetSafeNormal2D(); }
	if (auto* Movement = GetOwner()->FindComponentByClass<URLEnemyMovementComponent>()) { Movement->SetFacingLocked(true); }
	Phase = EPhase::Firing;
	Remaining = FMath::Max(0.1f, Settings->FiringDuration);
	DamageDelay = 0.0f;
	UpdateDecalProgress(1.0f);
	Beam->SetVisibility(true);
	Decal->SetVisibility(true);
}

void URLBossLaserComponent::UpdateBeam(float DeltaTime, bool bApplyDamage)
{
	const FVector Origin = Mesh->GetSocketLocation(TEXT("laser"));
	FVector End = Origin + LockedDirection * FMath::Max(1.0f, Settings->Length);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(BossLaser), false, GetOwner());
	FHitResult WallHit;
	if (GetWorld()->LineTraceSingleByChannel(WallHit, Origin, End, ECC_WorldStatic, Query)) { End = WallHit.ImpactPoint; }
	const float Length = FVector::Dist(Origin, End);
	const float Width = FMath::Max(1.0f, Settings->Width);
	Beam->SetWorldLocation((Origin + End) * 0.5f);
	Beam->SetWorldRotation(LockedDirection.Rotation());
	Beam->SetWorldScale3D(FVector(FMath::Max(1.0f, Length) / 100.0f, Width / 100.0f, 0.12f));
	FHitResult FloorHit;
	FVector FloorCenter = (Origin + End) * 0.5f;
	if (GetWorld()->LineTraceSingleByChannel(FloorHit, FloorCenter + FVector(0,0,100), FloorCenter - FVector(0,0,2000), ECC_WorldStatic, Query))
	{
		FloorCenter.Z = FloorHit.ImpactPoint.Z + 30.0f;
	}
	Decal->SetWorldLocation(FloorCenter);
	Decal->SetWorldRotation(FRotator(-90.0f, LockedDirection.Rotation().Yaw, 0.0f));
	Decal->DecalSize = FVector(64.0f, Width * 0.5f, FMath::Max(1.0f, Length) * 0.5f);
	// Preparation previews share the geometry, never the damage path.
	if (!bApplyDamage) { return; }
	DamageDelay = FMath::Max(0.0f, DamageDelay - DeltaTime);
	ARLPlayerCharacter* PlayerCharacter = Cast<ARLPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!PlayerCharacter || DamageDelay > 0.0f) { return; }
	const FVector PlayerLocation = PlayerCharacter->GetActorLocation();
	const FVector Closest = FMath::ClosestPointOnSegment(FVector(PlayerLocation.X, PlayerLocation.Y, 0),
		FVector(Origin.X, Origin.Y, 0), FVector(End.X, End.Y, 0));
	const float Radius = Width * 0.5f + PlayerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	if (FVector::DistSquared2D(PlayerLocation, Closest) <= FMath::Square(Radius))
	{
		// Use the player's normal damage gate: rolling/debug invulnerability still apply.
		const float Applied = UGameplayStatics::ApplyDamage(PlayerCharacter, FMath::Max(0.0f, Settings->Damage),
			Cast<APawn>(GetOwner())->GetController(), GetOwner(), nullptr);
		if (Applied > 0.0f) { DamageDelay = FMath::Max(0.1f, Settings->DamageInterval); }
	}
}

void URLBossLaserComponent::UpdateVisualOpacity(float Opacity)
{
	if (BeamMaterialInstance) { BeamMaterialInstance->SetScalarParameterValue(TEXT("LaserOpacity"), Opacity); }
	if (DecalMaterialInstance) { DecalMaterialInstance->SetScalarParameterValue(TEXT("LaserOpacity"), Opacity); }
}

void URLBossLaserComponent::BeginFadeOut()
{
	if (Settings->FadeOutDuration <= 0.0f) { Cancel(true); return; }
	Phase = EPhase::Fading;
	Remaining = Settings->FadeOutDuration;
	// Keep the final geometry frozen; fading is visual-only and never applies damage.
}

void URLBossLaserComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	auto* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy || !Enemy->IsPoolActive() || Enemy->IsSpawning()) { Cancel(); return; }
	const auto* Overload = GetOwner()->FindComponentByClass<URLBossOverloadComponent>();
	if (Overload && Overload->WantsOverload()) { return; }
	const auto* Charge = GetOwner()->FindComponentByClass<URLBossChargeComponent>();
	if (Charge && Charge->IsActive()) { return; }
	Remaining -= FMath::Max(0.0f, DeltaTime);
	if (Phase == EPhase::Waiting)
	{
		if (Remaining <= 0.0f && !HasActivePeerLaser()) { BeginPreparation(); }
	}
	else if (Phase == EPhase::Preparing)
	{
		// Track through the final preparation frame, then fire on the advertised deadline.
		// Waiting for alignment here would leave a visually full warning before firing.
		UpdatePreparationPreview();
		if (Phase == EPhase::Preparing && Remaining <= 0.0f) { BeginFiring(); }
		if (Phase == EPhase::Firing) { UpdateBeam(0.0f, true); }
	}
	else if (Phase == EPhase::Firing)
	{
		if (Remaining <= 0.0f) { BeginFadeOut(); }
		else { UpdateBeam(DeltaTime, true); }
	}
	else if (Phase == EPhase::Fading)
	{
		if (Remaining <= 0.0f) { Cancel(true); }
		else { UpdateVisualOpacity(FMath::Clamp(Remaining / Settings->FadeOutDuration, 0.0f, 1.0f)); }
	}
}

bool URLBossLaserComponent::HasActivePeerLaser() const
{
	if (!bSerializeWithPeers || !GetWorld()) { return false; }
	for (TActorIterator<ARLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (*It == GetOwner() || !It->IsPoolActive() || It->IsSpawning()) { continue; }
		const auto* Peer = It->FindComponentByClass<URLBossLaserComponent>();
		if (Peer && Peer->bSerializeWithPeers && (Peer->IsPreparing() || Peer->IsFiring() || Peer->IsFading())) { return true; }
	}
	return false;
}

void URLBossLaserComponent::Cancel(bool bResumeCombat)
{
	const bool bWasActive = Phase != EPhase::Waiting;
	Phase = EPhase::Waiting;
	Remaining = Settings ? FMath::Max(0.1f, Settings->Cooldown) : 10.0f;
	DamageDelay = 0.0f;
	UpdateVisualOpacity(1.0f);
	UpdateDecalProgress(0.0f);
	if (Beam) { Beam->SetVisibility(false); }
	if (Decal) { Decal->SetVisibility(false); }
	if (!bWasActive) { return; }
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

void URLBossLaserComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Cancel();
	Super::EndPlay(EndPlayReason);
}
