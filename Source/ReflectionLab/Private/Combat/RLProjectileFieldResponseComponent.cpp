#include "Combat/RLProjectileFieldResponseComponent.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLProjectileVisualComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

void URLProjectileFieldResponseComponent::SetSpeedMultiplier(float Value)
{
	auto* Projectile = Cast<ARLProjectile>(GetOwner());
	if (!Projectile || !Projectile->IsPoolActive() || Projectile->IsFadingOut() || !FMath::IsFinite(Value)) { return; }
	auto* Movement = Projectile->FindComponentByClass<UProjectileMovementComponent>();
	const float BaseSpeed = Movement->Velocity.Size() / SpeedMultiplier;
	SpeedMultiplier = FMath::Clamp(Value, 0.1f, 3.0f);
	if (BaseSpeed > UE_SMALL_NUMBER) { Projectile->SetProjectileSpeed(BaseSpeed, Movement->Velocity.GetSafeNormal()); }
	Projectile->GetVisualComponent()->UpdateProjectileMaterial();
}

void URLProjectileFieldResponseComponent::Refract(float Degrees)
{
	auto* Projectile = Cast<ARLProjectile>(GetOwner());
	if (!Projectile || !Projectile->IsPoolActive() || Projectile->IsFadingOut() || !FMath::IsFinite(Degrees)) { return; }
	auto* Movement = Projectile->FindComponentByClass<UProjectileMovementComponent>();
	const float BaseSpeed = Movement->Velocity.Size() / SpeedMultiplier;
	const FVector Direction = Movement->Velocity.GetSafeNormal2D().RotateAngleAxis(Degrees, FVector::UpVector);
	if (BaseSpeed > UE_SMALL_NUMBER) { Projectile->SetProjectileSpeed(BaseSpeed, Direction); }
}
