#pragma once

#include "CoreMinimal.h"
#include "RLEnemyAttackVariation.generated.h"

USTRUCT(BlueprintType)
struct FRLEnemyAttackVariation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Variation")
	bool bRandomizePatternStart = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Variation", meta = (ClampMin = "0.0"))
	float InitialDelayJitterSeconds = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Variation", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float AttackIntervalJitterRatio = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Variation", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float BurstIntervalJitterRatio = 0.1f;

	static float SampleInterval(float Base, float Ratio, float Minimum)
	{
		const float SafeBase = FMath::IsFinite(Base) ? FMath::Max(Minimum, Base) : Minimum;
		const float SafeRatio = FMath::IsFinite(Ratio) ? FMath::Clamp(Ratio, 0.0f, 0.5f) : 0.0f;
		return FMath::Max(Minimum, SafeBase * FMath::FRandRange(1.0f - SafeRatio, 1.0f + SafeRatio));
	}

	static int32 GetPatternCycleLength(const TArray<int32>& Intervals)
	{
		// Exact common period for ordinary rules. Bound extreme authored periods
		// to avoid overflow and enormous offsets.
		constexpr int32 MaxCycleLength = 4096;
		int32 Cycle = 1;
		for (int32 Interval : Intervals)
		{
			if (Interval <= 0) { continue; }
			int32 A = Cycle;
			int32 B = Interval;
			while (B != 0)
			{
				const int32 Remainder = A % B;
				A = B;
				B = Remainder;
			}
			const int64 Combined = static_cast<int64>(Cycle / A) * Interval;
			if (Combined >= MaxCycleLength) { return MaxCycleLength; }
			Cycle = static_cast<int32>(Combined);
		}
		return Cycle;
	}
};
