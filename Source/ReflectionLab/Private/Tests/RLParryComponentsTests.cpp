#include "Player/Components/RLParryComponent.h"
#include "Player/Components/RLParryProgressionComponent.h"
#include "Player/Components/RLRunRewardComponent.h"
#include "Data/RLRunRewardDataAsset.h"
#include "Player/Components/RLHealthComponent.h"
#include "Data/RLPlayerStatsDataAsset.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLParryProgressionTest, "ReflectionLab.Player.ParryProgression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLParryProgressionTest::RunTest(const FString& Parameters)
{
	URLParryProgressionComponent* Progression = NewObject<URLParryProgressionComponent>();
	URLPlayerStatsDataAsset* Stats = NewObject<URLPlayerStatsDataAsset>();
	Progression->Configure(Stats);
	const FRLParryResult Success{3, true, false, false};
	Progression->RegisterSuccess(Success);
	Progression->RegisterSuccess(Success);
	TestEqual(TEXT("Multi-parry increments once per swing"), Progression->GetChainCount(), 2);
	TestEqual(TEXT("Milestone swing predicts upgraded reflection"), Progression->PreviewNextSuccess(), 2);
	TestEqual(TEXT("Preview does not mutate level"), Progression->GetEnhancementLevel(), 1);
	TestEqual(TEXT("Repeated preview does not mutate combo"), Progression->GetChainCount(), 2);
	Progression->RegisterSuccess(Success);
	TestEqual(TEXT("Milestone upgrades once"), Progression->GetEnhancementLevel(), 2);
	for (int32 Index = 0; Index < 6; ++Index) { Progression->RegisterSuccess(Success); }
	TestEqual(TEXT("Nine swings reach overdrive"), Progression->GetEnhancementLevel(), 4);
	Progression->RegisterSuccess(FRLParryResult{5, true, true, true});
	Progression->ConsumeOverdriveEnhancement();
	TestEqual(TEXT("Overdrive returns to level three"), Progression->GetEnhancementLevel(), 3);
	TestEqual(TEXT("Overdrive preserves combo"), Progression->GetChainCount(), 10);
	Progression->DowngradeParryEnhancement();
	Progression->ResetParryChain();
	TestEqual(TEXT("Failure downgrades"), Progression->GetEnhancementLevel(), 2);
	TestEqual(TEXT("Failure resets chain"), Progression->GetChainCount(), 0);
	TestEqual(TEXT("Failure resets enhancement progress"), Progression->PreviewNextSuccess(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLRunRewardComponentTest, "ReflectionLab.Player.RunRewardComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLRunRewardComponentTest::RunTest(const FString& Parameters)
{
	URLHealthComponent* Health = NewObject<URLHealthComponent>();
	URLRunRewardComponent* Rewards = NewObject<URLRunRewardComponent>();
	URLParryComponent* Parry = NewObject<URLParryComponent>();
	URLPlayerStatsDataAsset* Stats = NewObject<URLPlayerStatsDataAsset>();
	Stats->ParryRange = 200.0f;
	Stats->PerfectParryOuterBandWidth = 180.0f;
	Health->InitializeHealth(10.0f);
	Rewards->Initialize(Health);
	auto ApplyReward = [Rewards](ERLRunRewardType Type, float Amount)
	{
		URLRunRewardDataAsset* Definition = NewObject<URLRunRewardDataAsset>();
		Definition->RewardType = Type;
		Definition->Amount = Amount;
		return Rewards->TryApplyReward(Definition);
	};
	Parry->Initialize(nullptr, Rewards, nullptr);
	TestTrue(TEXT("Range reward applies"), ApplyReward(ERLRunRewardType::ExtendedRange, 0.18f));
	TestTrue(TEXT("Range reward applies"), ApplyReward(ERLRunRewardType::ExtendedRange, 0.18f));
	Parry->Configure(Stats);
	const FRLParryStats First = Parry->GetViewState().Stats;
	TestTrue(TEXT("Two range rewards add on base range"), FMath::IsNearlyEqual(First.ReflectionRange, 272.0f));
	TestEqual(TEXT("Perfect band clamps against configured base range"), First.PerfectParryOuterBandWidth, 180.0f);
	Parry->Configure(Stats);
	TestEqual(TEXT("Reconfiguration never reapplies bonuses to effective stats"),
		Parry->GetViewState().Stats.ReflectionRange, First.ReflectionRange);
	Health->ApplyDamage(5.0f);
	TestTrue(TEXT("Vitality reward applies"), ApplyReward(ERLRunRewardType::Vitality, 2.0f));
	TestEqual(TEXT("Vitality restores only added capacity"), Health->GetCurrentHealth(), 7.0f);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TestTrue(TEXT("Recovery reward applies"), ApplyReward(ERLRunRewardType::PerfectRecovery, 1.0f));
	}
	Rewards->ApplyPerfectRecovery(FRLParryResult{5, true, false, false});
	TestEqual(TEXT("Stacked healing once for multi-parry"), Health->GetCurrentHealth(), 10.0f);
	Rewards->ResetRunRewards();
	Parry->Configure(Stats);
	TestEqual(TEXT("Reset removes range bonuses"), Parry->GetViewState().Stats.ReflectionRange, 200.0f);
	TestEqual(TEXT("Reset removes max-health bonus"), Health->GetMaxHealth(), 10.0f);
	TestFalse(TEXT("Reset removes healing reward"), Rewards->HasPerfectRecoveryReward());
	return true;
}
#endif
