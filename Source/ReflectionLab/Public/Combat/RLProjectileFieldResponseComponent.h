#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLProjectileFieldResponseComponent.generated.h"

// Keeps environmental speed separate from authored projectile/parry/rally speed.
UCLASS()
class REFLECTIONLAB_API URLProjectileFieldResponseComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	void ResetForActivation(const FVector& Position) { SpeedMultiplier = 1; FlightStart = Position; MirrorBounceCount = 0; bAccelerationCharged = false; ++ActivationSerial; }
	bool TryConsumeMirrorBounce(int32 Limit) { if (MirrorBounceCount >= Limit) { return false; } ++MirrorBounceCount; return true; }
	const FVector& GetFlightStart() const { return FlightStart; }
	float GetSpeedMultiplier() const { return SpeedMultiplier; }
	uint64 GetActivationSerial() const { return ActivationSerial; }
	void SetSpeedMultiplier(float Value);
	void Refract(float Degrees);
	void MarkAccelerationCharged() { bAccelerationCharged = true; }
	void ClearAccelerationCharge() { bAccelerationCharged = false; }
	bool HasAccelerationCharge() const { return bAccelerationCharged; }
	int32 GetMirrorBounceCount() const { return MirrorBounceCount; }
private:
	int32 MirrorBounceCount = 0;
	bool bAccelerationCharged = false;
	float SpeedMultiplier = 1;
	uint64 ActivationSerial = 0;
	FVector FlightStart = FVector::ZeroVector;
};
