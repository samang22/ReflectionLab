#include "Data/RLDifficultyScheduleDataAsset.h"

const FRLDifficultyPhase* URLDifficultyScheduleDataAsset::FindPhaseAtTime(
	float ElapsedSeconds,
	int32& OutPhaseIndex) const
{
	OutPhaseIndex = INDEX_NONE;
	const FRLDifficultyPhase* SelectedPhase = nullptr;
	float LatestStartTime = -1.0f;

	for (int32 PhaseIndex = 0; PhaseIndex < Phases.Num(); ++PhaseIndex)
	{
		const FRLDifficultyPhase& Phase = Phases[PhaseIndex];
		const float StartTime = FMath::Max(0.0f, Phase.StartTimeSeconds);
		if (StartTime <= ElapsedSeconds && StartTime >= LatestStartTime)
		{
			LatestStartTime = StartTime;
			SelectedPhase = &Phase;
			OutPhaseIndex = PhaseIndex;
		}
	}

	return SelectedPhase;
}
