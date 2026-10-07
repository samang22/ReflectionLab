#pragma once

#include "CoreMinimal.h"

namespace RLBossSummon
{
	inline float GetTelegraphDelay(int32 Index, float Duration, float Stagger)
	{
		return FMath::Max(0.1f, Duration) + FMath::Max(0, Index) * FMath::Max(0.0f, Stagger);
	}

	inline int32 GetSpawnCount(int32 AliveCount, int32 RequestedCount, int32 MaximumAlive)
	{
		return FMath::Min(FMath::Clamp(RequestedCount, 1, 10),
			FMath::Max(0, FMath::Clamp(MaximumAlive, 1, 10) - FMath::Max(0, AliveCount)));
	}

	inline bool IsEnraged(float CurrentHealth, float MaxHealth, float Threshold)
	{
		return CurrentHealth / FMath::Max(1.0f, MaxHealth) <= FMath::Clamp(Threshold, 0.0f, 1.0f);
	}
}
