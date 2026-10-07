#pragma once

#include "CoreMinimal.h"

namespace RLBossCharge
{
	inline bool IsWithinSweptContact(const FVector& Start, const FVector& End, const FVector& Target, float Radius)
	{
		const FVector Closest = FMath::ClosestPointOnSegment(FVector(Target.X, Target.Y, 0),
			FVector(Start.X, Start.Y, 0), FVector(End.X, End.Y, 0));
		return FVector::DistSquared2D(Target, Closest) <= FMath::Square(FMath::Max(0.0f, Radius));
	}

	inline float GetFanAngle(int32 Index, int32 Count, float SpreadDegrees)
	{
		return Count > 1 ? FMath::Clamp(SpreadDegrees, 0.0f, 180.0f) *
			(static_cast<float>(Index) / (Count - 1) - 0.5f) : 0.0f;
	}
}
