#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Framework/RLWaveDefinitionResolver.h"
#include "Data/RLRunDefinitionDataAsset.h"
#include "Data/RLArenaExperimentDataAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLWaveResolutionTest, "ReflectionLab.Run.WaveResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLWaveResolutionTest::RunTest(const FString& Parameters)
{
	FRLWaveDefinition Authored;
	Authored.EnemyCount = 4;
	Authored.AttackIntervalMultiplier = 2;
	Authored.TimeBetweenShotsMultiplier = 3;
	Authored.ProjectileRules.AddDefaulted();
	Authored.RingAttack.Weight = 5;
	Authored.RingAttack.ShotInterval = 4;
	FRLRoundDefinition Round;
	Round.bIsTutorial = false;
	Round.EnemyCountMultiplier = 3;
	Round.BaseAttackTimingMultiplier = 2;
	URLArenaExperimentDataAsset* Experiment = NewObject<URLArenaExperimentDataAsset>();
	Experiment->AttackFrequencyMultiplier = 2;
	Experiment->ProjectileDefinition = NewObject<URLProjectileDefinitionDataAsset>();
	const auto First = RLWaveDefinitionResolver::Resolve(Authored, Round, 4, Experiment);
	const auto Again = RLWaveDefinitionResolver::Resolve(Authored, Round, 4, Experiment);
	TestEqual(TEXT("Round count multiplier"), First.EnemyCount, 12);
	TestEqual(TEXT("Round and experiment timing compose"), First.AttackIntervalMultiplier, 0.5f);
	TestEqual(TEXT("Burst spacing follows the same timing"), First.TimeBetweenShotsMultiplier, 0.75f);
	TestEqual(TEXT("Experiment replaces projectile"), First.DefaultProjectileDefinition.Get(), Experiment->ProjectileDefinition.Get());
	TestTrue(TEXT("Experiment removes special rules"), First.ProjectileRules.IsEmpty());
	TestEqual(TEXT("Experiment disables weighted rings"), First.RingAttack.Weight, 0);
	TestEqual(TEXT("Experiment disables legacy rings"), First.RingAttack.ShotInterval, 0);
	TestTrue(TEXT("Experiment enables weighted selection"), First.bUseWeightedPatterns);
	TestEqual(TEXT("Authored count remains intact"), Authored.EnemyCount, 4);
	TestEqual(TEXT("Authored interval remains intact"), Authored.AttackIntervalMultiplier, 2.0f);
	TestEqual(TEXT("Authored rules remain intact"), Authored.ProjectileRules.Num(), 1);
	TestEqual(TEXT("Repeated resolution does not compound"), Again.AttackIntervalMultiplier, First.AttackIntervalMultiplier);
	Round.EnemyCountOverride = 1;
	TestEqual(TEXT("Boss override wins over count multiplier"),
		RLWaveDefinitionResolver::Resolve(Authored, Round, 4, nullptr).EnemyCount, 1);
	Round.bIsTutorial = true;
	const auto Tutorial = RLWaveDefinitionResolver::Resolve(Authored, Round, 4, nullptr);
	TestEqual(TEXT("Tutorial retains authored count"), Tutorial.EnemyCount, 4);
	TestEqual(TEXT("Tutorial retains authored timing"), Tutorial.AttackIntervalMultiplier, 2.0f);
	Round.bIsTutorial = false;
	const auto Invalid = RLWaveDefinitionResolver::Resolve(Authored, Round, 0, nullptr);
	TestTrue(TEXT("Invalid caller frequency produces finite timing"), FMath::IsFinite(Invalid.AttackIntervalMultiplier));
	Experiment->ProjectileDefinition = nullptr;
	Experiment->bAllowRingAttacks = false;
	const auto NoRings = RLWaveDefinitionResolver::Resolve(Authored, Round, 1, Experiment);
	TestEqual(TEXT("Ring opt-out disables weighted patterns without a projectile override"), NoRings.RingAttack.Weight, 0);
	TestEqual(TEXT("Ring opt-out disables legacy intervals"), NoRings.RingAttack.ShotInterval, 0);
	TestEqual(TEXT("Ring opt-out preserves special projectile rules"), NoRings.ProjectileRules.Num(), Authored.ProjectileRules.Num());
	TestEqual(TEXT("Ring opt-out does not alter authored rings"), Authored.RingAttack.Weight, 5);
	Experiment->bAllowRingAttacks = true;
	TestEqual(TEXT("Other experiments retain authored rings"),
		RLWaveDefinitionResolver::Resolve(Authored, Round, 1, Experiment).RingAttack.Weight, 5);
	auto* Run = LoadObject<URLRunDefinitionDataAsset>(nullptr,
		TEXT("/Game/ReflectionLab/Data/Difficulty/DA_DefaultRun.DA_DefaultRun"));
	if (TestNotNull(TEXT("Authored run"), Run))
	{
		int32 Number = 0;
		bool bFoundFourthRound = false;
		for (const auto& Entry : Run->Rounds)
		{
			if (Entry.bIsTutorial || ++Number != 4) { continue; }
			bFoundFourthRound = true;
			if (!TestNotNull(TEXT("Round four experiment"), Entry.Experiment.Get()) ||
				!TestNotNull(TEXT("Round four schedule"), Entry.DifficultySchedule.Get())) { break; }
			TestFalse(TEXT("Round four asset disables ring attacks"), Entry.Experiment->bAllowRingAttacks);
			for (const auto& Wave : Entry.DifficultySchedule->Waves)
			{
				const auto Resolved = RLWaveDefinitionResolver::Resolve(Wave, Entry, 1, Entry.Experiment);
				TestEqual(TEXT("Every round-four wave excludes weighted rings"), Resolved.RingAttack.Weight, 0);
				TestEqual(TEXT("Every round-four wave excludes interval rings"), Resolved.RingAttack.ShotInterval, 0);
			}
		}
		TestTrue(TEXT("Displayed round four exists"), bFoundFourthRound);
	}
	return true;
}
#endif
