#include "Data/RLRunDefinitionDataAsset.h"
#include "Data/RLDifficultyScheduleDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"

float FRLEnemyHealthMix::GetHealthForSpawn(int32 Index, int32 Count) const
{
	const float Alpha = (FMath::Max(0, Index) + 0.5f) / FMath::Max(1, Count);
	const float Heavy = FMath::IsFinite(HeavyRatio) ? FMath::Clamp(HeavyRatio, 0.0f, 1.0f) : 0.0f;
	const float Reinforced = FMath::IsFinite(ReinforcedRatio) ? FMath::Clamp(ReinforcedRatio, 0.0f, 1.0f - Heavy) : 0.0f;
	const float Health = Alpha < Heavy ? HeavyHealth : (Alpha < Heavy + Reinforced ? ReinforcedHealth : NormalHealth);
	return FMath::IsFinite(Health) ? FMath::Max(1.0f, Health) : 2.0f;
}

bool URLRunDefinitionDataAsset::MigrateLegacyRoundLayout()
{
	if (bExplicitRoundLayout) { return true; }
	// Validate before replacing the authored list. Keep old maps for compatibility/recovery.
	for (const FRLRoundDefinition& Round : Rounds)
	{
		if (!Round.DifficultySchedule || Round.DifficultySchedule->Waves.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("Cannot migrate %s: round %s has no waves."), *GetPathName(), *Round.RoundName.ToString());
			return false;
		}
	}
	if (Rounds.IsEmpty()) { return false; }
	TArray<FRLRoundDefinition> Converted;
	int32 MainRoundNumber = 0;
	for (const FRLRoundDefinition& Round : Rounds)
	{
		const auto* Schedule = Round.DifficultySchedule.Get();
		const int32 Parts = !Round.bIsTutorial && Schedule->Waves.Num() >= 2 ? 2 : 1;
		const int32 SplitIndex = (Schedule->Waves.Num() + 1) / 2;
		for (int32 Part = 0; Part < Parts; ++Part)
		{
			FRLRoundDefinition Result = Round;
			if (Parts == 2)
			{
				Result.RoundName = FName(*FString::Printf(TEXT("%s %d"), *Round.RoundName.ToString(), Part + 1));
			}
			// Embedded schedules persist with the run: editor rows now match displayed rounds.
			auto* AuthoredSchedule = DuplicateObject<URLDifficultyScheduleDataAsset>(Schedule, this,
				MakeUniqueObjectName(this, URLDifficultyScheduleDataAsset::StaticClass(), TEXT("RoundSchedule")));
			AuthoredSchedule->Waves.Reset();
			const int32 Begin = Parts == 1 || Part == 0 ? 0 : SplitIndex;
			const int32 End = Parts == 1 || Part == 1 ? Schedule->Waves.Num() : SplitIndex;
			for (int32 Index = Begin; Index < End; ++Index) { AuthoredSchedule->Waves.Add(Schedule->Waves[Index]); }
			Result.DifficultySchedule = AuthoredSchedule;
			if (!Round.bIsTutorial)
			{
				++MainRoundNumber;
				Result.Experiment = RoundExperiments.FindRef(MainRoundNumber);
				Result.EnemyClass = RoundEnemyClasses.FindRef(MainRoundNumber);
				Result.EnemyCountOverride = RoundEnemyCounts.FindRef(MainRoundNumber);
				Result.bSingleWave = SingleWaveRounds.Contains(MainRoundNumber);
				if (const auto* Mix = GetEnemyHealthMix(MainRoundNumber))
				{
					Result.bOverrideEnemyHealth = true;
					Result.EnemyHealth = *Mix;
				}
			}
			Converted.Add(Result);
		}
	}
	Rounds = MoveTemp(Converted);
	bExplicitRoundLayout = true;
	return true;
}
