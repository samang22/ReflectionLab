#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Data/RLRunDefinitionDataAsset.h"
#include "Data/RLDifficultyScheduleDataAsset.h"
#include "Data/RLArenaExperimentDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLRoundLayoutMigrationTest, "ReflectionLab.Run.RoundLayoutMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRLRoundLayoutMigrationTest::RunTest(const FString& Parameters)
{
	auto* Run = NewObject<URLRunDefinitionDataAsset>();
	auto* Tutorial = NewObject<URLDifficultyScheduleDataAsset>(Run);
	Tutorial->Waves.AddDefaulted();
	Run->Rounds.AddDefaulted_GetRef().DifficultySchedule = Tutorial;
	Run->Rounds.Last().bIsTutorial = true;
	auto* Schedule = NewObject<URLDifficultyScheduleDataAsset>(Run);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		auto& Wave = Schedule->Waves.AddDefaulted_GetRef();
		Wave.WaveName = FName(*FString::Printf(TEXT("Wave_%d"), Index));
		Wave.EnemyCount = Index + 1;
	}
	Run->Rounds.AddDefaulted_GetRef().DifficultySchedule = Schedule;
	auto* Experiment = NewObject<URLArenaExperimentDataAsset>(Run);
	Run->RoundExperiments.Add(2, Experiment);
	Run->RoundEnemyClasses.Add(2, ARLEnemyCharacter::StaticClass());
	Run->RoundEnemyCounts.Add(2, 1); Run->SingleWaveRounds.Add(2);
	FRLEnemyHealthMix Mix; Mix.NormalHealth = 7;
	Run->RoundEnemyHealthMixes.Add(1, Mix);
	Run->StartingRoundNumber = 2;
	TestTrue(TEXT("Migration succeeds"), Run->MigrateLegacyRoundLayout());
	TestEqual(TEXT("Tutorial plus two displayed rounds"), Run->Rounds.Num(), 3);
	TestEqual(TEXT("Odd waves preserve first part"), Run->Rounds[1].DifficultySchedule->Waves.Num(), 2);
	TestEqual(TEXT("Odd waves preserve second part"), Run->Rounds[2].DifficultySchedule->Waves.Num(), 1);
	TestEqual(TEXT("Last wave identity preserved"), Run->Rounds[2].DifficultySchedule->Waves[0].WaveName, Schedule->Waves[2].WaveName);
	TestEqual(TEXT("Legacy schedule untouched"), Schedule->Waves.Num(), 3);
	TestEqual(TEXT("Start override retained"), Run->StartingRoundNumber, 2);
	TestEqual(TEXT("Experiment follows displayed number"), Run->Rounds[2].Experiment.Get(), Experiment);
	TestEqual(TEXT("Enemy override copied"), Run->Rounds[2].EnemyCountOverride, 1);
	TestTrue(TEXT("Single wave copied"), Run->Rounds[2].bSingleWave);
	TestEqual(TEXT("Inherited health made explicit"), Run->Rounds[2].EnemyHealth.NormalHealth, 7.0f);
	TestTrue(TEXT("Schedule embedded in run"), Run->Rounds[2].DifficultySchedule->GetOuter() == Run);
	auto* FirstSchedule = Run->Rounds[1].DifficultySchedule.Get();
	TestTrue(TEXT("Second migration is safe"), Run->MigrateLegacyRoundLayout());
	TestEqual(TEXT("Never split twice"), Run->Rounds.Num(), 3);
	TestEqual(TEXT("No second duplicate"), Run->Rounds[1].DifficultySchedule.Get(), FirstSchedule);
	auto* Invalid = NewObject<URLRunDefinitionDataAsset>();
	Invalid->Rounds.AddDefaulted();
	AddExpectedError(TEXT("has no waves"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Invalid source rejected"), Invalid->MigrateLegacyRoundLayout());
	TestFalse(TEXT("Invalid migration does not set version"), Invalid->bExplicitRoundLayout);
	return true;
}
#endif
