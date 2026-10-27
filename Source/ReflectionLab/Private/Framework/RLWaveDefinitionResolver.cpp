#include "Framework/RLWaveDefinitionResolver.h"
#include "Data/RLRunDefinitionDataAsset.h"
#include "Data/RLArenaExperimentDataAsset.h"

FRLWaveDefinition RLWaveDefinitionResolver::Resolve(const FRLWaveDefinition& AuthoredWave,
	const FRLRoundDefinition& Round, float RoundAttackFrequency, const URLArenaExperimentDataAsset* Experiment)
{
	FRLWaveDefinition WaveDefinition = AuthoredWave;
	if (!Round.bIsTutorial)
	{
		WaveDefinition.EnemyCount = static_cast<int32>(FMath::Clamp<int64>(
			static_cast<int64>(WaveDefinition.EnemyCount) * FMath::Max(1, Round.EnemyCountMultiplier), 1, MAX_int32));
		if (Round.EnemyCountOverride > 0)
		{
			WaveDefinition.EnemyCount = Round.EnemyCountOverride;
		}
		// Round one retains the existing half-frequency baseline. Later rounds
		// accelerate without mutating the schedule or compounding per wave.
		const float Frequency = FMath::IsFinite(RoundAttackFrequency) && RoundAttackFrequency > 0
			? RoundAttackFrequency : 1.0f;
		const float AttackTimingScale = FMath::Max(0.1f, Round.BaseAttackTimingMultiplier) / Frequency;
		WaveDefinition.AttackIntervalMultiplier *= AttackTimingScale;
		WaveDefinition.TimeBetweenShotsMultiplier *= AttackTimingScale;
	}
	if (Experiment)
	{
		const float Frequency = FMath::IsFinite(Experiment->AttackFrequencyMultiplier)
			? FMath::Clamp(Experiment->AttackFrequencyMultiplier, 0.1f, 3.0f) : 1.0f;
		WaveDefinition.AttackIntervalMultiplier /= Frequency;
		WaveDefinition.TimeBetweenShotsMultiplier /= Frequency;
	}
	if (Experiment && Experiment->ProjectileDefinition)
	{
		WaveDefinition.DefaultProjectileDefinition = Experiment->ProjectileDefinition;
		WaveDefinition.ProjectileRules.Empty();
		WaveDefinition.RingAttack.Weight = 0;
		WaveDefinition.RingAttack.ShotInterval = 0;
		WaveDefinition.bUseWeightedPatterns = true;
		WaveDefinition.DefaultProjectileWeight = 1;
	}
	return WaveDefinition;
}
