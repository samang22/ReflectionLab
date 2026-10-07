#include "Enemies/Components/RLBossSummonMath.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLBossSummonLimitTest,
	"ReflectionLab.Boss.SummonLimits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLBossSummonLimitTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Initial summon adds two minions"), RLBossSummon::GetSpawnCount(0, 2, 3), 2);
	TestEqual(TEXT("Next summon fills only the remaining slot"), RLBossSummon::GetSpawnCount(2, 2, 3), 1);
	TestEqual(TEXT("Full cap prevents additional minions"), RLBossSummon::GetSpawnCount(3, 2, 3), 0);
	TestEqual(TEXT("Reduced cap never produces a negative count"), RLBossSummon::GetSpawnCount(4, 2, 3), 0);
	TestEqual(TEXT("A killed minion frees one slot"), RLBossSummon::GetSpawnCount(2, 2, 3), 1);
	TestTrue(TEXT("Exactly half health uses the faster interval"), RLBossSummon::IsEnraged(10, 20, 0.5f));
	TestFalse(TEXT("Above half health uses the normal interval"), RLBossSummon::IsEnraged(11, 20, 0.5f));
	TestTrue(TEXT("First minion has a 0.6-second telegraph"),
		FMath::IsNearlyEqual(RLBossSummon::GetTelegraphDelay(0, 0.6f, 0.2f), 0.6f));
	TestTrue(TEXT("Second minion is staggered by 0.2 seconds"),
		FMath::IsNearlyEqual(RLBossSummon::GetTelegraphDelay(1, 0.6f, 0.2f), 0.8f));
	TestEqual(TEXT("Pending reservations also occupy the cap"), RLBossSummon::GetSpawnCount(1 + 2, 2, 3), 0);
	return true;
}
#endif
