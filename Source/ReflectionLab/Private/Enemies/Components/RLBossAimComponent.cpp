#include "Enemies/Components/RLBossAimComponent.h"

#include "Enemies/Components/RLBossAimMath.h"
#include "Enemies/Components/RLBossLaserComponent.h"
#include "Enemies/Components/RLBossChargeComponent.h"
#include "Enemies/Components/RLBossOverloadComponent.h"
#include "Enemies/Components/RLEnemyMovementComponent.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

URLBossAimComponent::URLBossAimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Read the animated socket pose after skeletal mesh updates.
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void URLBossAimComponent::AimAtPlayer(FName SocketName)
{
	auto* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Enemy || !Enemy->IsPoolActive() || Enemy->IsSpawning() || !PlayerPawn) { return; }
	const auto* Overload = Enemy->FindComponentByClass<URLBossOverloadComponent>();
	const auto* Laser = Enemy->FindComponentByClass<URLBossLaserComponent>();
	if ((Overload && Overload->WantsOverload()) || (Laser && Laser->IsFiring())) { return; }
	USkeletalMeshComponent* Mesh = Enemy->GetMesh();
	if (!Mesh || !Mesh->DoesSocketExist(SocketName)) { return; }
	if (auto* Movement = Enemy->FindComponentByClass<URLEnemyMovementComponent>()) { Movement->SetFacingLocked(true); }
	const FTransform Socket = Mesh->GetSocketTransform(SocketName, RTS_World);
	float Correction = 0.0f;
	if (RLBossAim::CalculateYawCorrection(Enemy->GetActorLocation(), Socket.GetLocation(),
		Socket.GetUnitAxis(EAxis::X), PlayerPawn->GetActorLocation(), Correction))
	{
		FRotator Rotation = Enemy->GetActorRotation();
		const float Speed = FMath::IsFinite(RotationInterpSpeed) ? FMath::Max(0.1f, RotationInterpSpeed) : 8.0f;
		const float Alpha = 1.0f - FMath::Exp(-Speed * GetWorld()->GetDeltaSeconds());
		Rotation.Yaw = FRotator::NormalizeAxis(Rotation.Yaw + Correction * Alpha);
		Enemy->SetActorRotation(Rotation);
	}
}

bool URLBossAimComponent::IsAligned(FName SocketName) const
{
	const auto* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Enemy || !PlayerPawn || !Enemy->GetMesh()->DoesSocketExist(SocketName)) { return false; }
	const FTransform Socket = Enemy->GetMesh()->GetSocketTransform(SocketName, RTS_World);
	float Correction = 0.0f;
	return RLBossAim::CalculateYawCorrection(Enemy->GetActorLocation(), Socket.GetLocation(),
		Socket.GetUnitAxis(EAxis::X), PlayerPawn->GetActorLocation(), Correction) &&
		FMath::Abs(Correction) <= FMath::Clamp(AlignmentToleranceDegrees, 0.01f, 5.0f);
}

bool URLBossAimComponent::RequestShot(FSimpleDelegate OnAligned)
{
	if (PendingShot.IsBound() || !OnAligned.IsBound()) { return false; }
	const auto* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy || !Enemy->IsPoolActive() || Enemy->IsSpawning() || !Enemy->GetMesh()->DoesSocketExist(TEXT("muzzle"))) { return false; }
	PendingShot = MoveTemp(OnAligned);
	return true;
}

void URLBossAimComponent::CancelShot()
{
	PendingShot.Unbind();
	ShotAimRemaining = 0.0f;
}

void URLBossAimComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const auto* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy || !Enemy->IsPoolActive() || Enemy->IsSpawning()) { CancelShot(); return; }
	const auto* Overload = GetOwner()->FindComponentByClass<URLBossOverloadComponent>();
	const auto* Laser = GetOwner()->FindComponentByClass<URLBossLaserComponent>();
	if ((Overload && Overload->WantsOverload()) || (Laser && Laser->IsFiring()))
	{
		CancelShot();
		return;
	}
	const auto* Charge = GetOwner()->FindComponentByClass<URLBossChargeComponent>();
	if (Charge && Charge->IsActive())
	{
		CancelShot();
		// Charge preparation owns aim; the dash and counter-shot keep that rotation.
		return;
	}
	if (Laser && Laser->IsPreparing())
	{
		CancelShot();
		// Laser preview advances its own interpolated aim once per frame.
		return;
	}
	if (PendingShot.IsBound())
	{
		AimAtPlayer(TEXT("muzzle"));
		if (IsAligned(TEXT("muzzle")))
		{
			FSimpleDelegate Shot = MoveTemp(PendingShot);
			PendingShot.Unbind();
			ShotAimRemaining = FMath::Max(0.0f, ShotAimHoldDuration);
			Shot.ExecuteIfBound();
		}
		return;
	}
	ShotAimRemaining = FMath::Max(0.0f, ShotAimRemaining - FMath::Max(0.0f, DeltaTime));
	if (ShotAimRemaining > 0.0f) { return; }
	// Ordinary idle/repositioning faces the player with the body's front.
	if (auto* Movement = GetOwner()->FindComponentByClass<URLEnemyMovementComponent>())
	{
		// Returning to body-front facing is interpolated too.
		Movement->SetFacingLocked(true);
		const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (PlayerPawn)
		{
			FRotator Rotation = GetOwner()->GetActorRotation();
			const float TargetYaw = (PlayerPawn->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal2D().Rotation().Yaw;
			const float Alpha = 1.0f - FMath::Exp(-FMath::Max(0.1f, RotationInterpSpeed) * FMath::Max(0.0f, DeltaTime));
			Rotation.Yaw = FRotator::NormalizeAxis(Rotation.Yaw + FMath::FindDeltaAngleDegrees(Rotation.Yaw, TargetYaw) * Alpha);
			GetOwner()->SetActorRotation(Rotation);
		}
	}
}
