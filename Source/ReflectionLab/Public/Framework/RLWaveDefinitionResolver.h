#pragma once
#include "CoreMinimal.h"
#include "Data/RLDifficultyScheduleDataAsset.h"
struct FRLRoundDefinition;
class URLArenaExperimentDataAsset;

// Resolves a value copy. No world access, spawning, or authored asset mutation.
namespace RLWaveDefinitionResolver
{
	REFLECTIONLAB_API FRLWaveDefinition Resolve(const FRLWaveDefinition& AuthoredWave,
		const FRLRoundDefinition& Round, float RoundAttackFrequency, const URLArenaExperimentDataAsset* Experiment);
}
