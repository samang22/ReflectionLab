#pragma once

#include "CoreMinimal.h"

namespace RLBossAim
{
	// Rotate the owner so the socket's forward ray intersects the target in XY.
	// Account for both the barrel's lateral offset and its authored rotation.
	inline bool CalculateYawCorrection(const FVector& Pivot, const FVector& Socket,
		const FVector& SocketForward, const FVector& Target, float& OutDegrees)
	{
		OutDegrees = 0.0f;
		const FVector Forward = SocketForward.GetSafeNormal2D();
		const FVector ToTarget(Target.X - Pivot.X, Target.Y - Pivot.Y, 0.0f);
		const FVector Offset(Socket.X - Pivot.X, Socket.Y - Pivot.Y, 0.0f);
		const float Distance = ToTarget.Size2D();
		if (Forward.IsNearlyZero() || Distance <= KINDA_SMALL_NUMBER) { return false; }
		const FVector Side(-Forward.Y, Forward.X, 0.0f);
		const float LateralOffset = FVector::DotProduct(Offset, Side);
		if (FMath::Abs(LateralOffset) > Distance) { return false; }
		const float AlongRay = FMath::Sqrt(FMath::Max(0.0f,
			Distance * Distance - LateralOffset * LateralOffset)) - FVector::DotProduct(Offset, Forward);
		if (AlongRay < 0.0f) { return false; }
		const float DesiredYaw = ToTarget.Rotation().Yaw - FMath::RadiansToDegrees(
			FMath::Asin(FMath::Clamp(LateralOffset / Distance, -1.0f, 1.0f)));
		OutDegrees = FMath::FindDeltaAngleDegrees(Forward.Rotation().Yaw, DesiredYaw);
		return FMath::IsFinite(OutDegrees);
	}
}
