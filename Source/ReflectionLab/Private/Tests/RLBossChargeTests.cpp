#include "Enemies/Components/RLBossChargeMath.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLBossChargeGeometryTest,
	"ReflectionLab.Boss.ChargeGeometryXY", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLBossChargeGeometryTest::RunTest(const FString& Parameters)
{
	const FVector Start(0, 0, 300);
	const FVector End(900, 0, 300);
	TestTrue(TEXT("Swept contact includes targets passed between frames"),
		RLBossCharge::IsWithinSweptContact(Start, End, FVector(450, 40, -2000), 50));
	TestFalse(TEXT("Outside the charge width is safe"),
		RLBossCharge::IsWithinSweptContact(Start, End, FVector(450, 51, 300), 50));
	TestFalse(TEXT("Targets beyond the stopped endpoint are not hit"),
		RLBossCharge::IsWithinSweptContact(Start, End, FVector(1000, 0, 300), 50));
	TestTrue(TEXT("Blocked charge still checks initial contact"),
		RLBossCharge::IsWithinSweptContact(Start, Start, FVector(10, 0, 0), 50));
	TestEqual(TEXT("Single counter shot has no spread offset"), RLBossCharge::GetFanAngle(0, 1, 80), 0.0f);
	TestEqual(TEXT("Fan begins at minus half spread"), RLBossCharge::GetFanAngle(0, 7, 80), -40.0f);
	TestEqual(TEXT("Fan center follows muzzle direction"), RLBossCharge::GetFanAngle(3, 7, 80), 0.0f);
	TestEqual(TEXT("Fan ends at plus half spread"), RLBossCharge::GetFanAngle(6, 7, 80), 40.0f);
	return true;
}
#endif
