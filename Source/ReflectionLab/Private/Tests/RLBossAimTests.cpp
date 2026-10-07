#include "Enemies/Components/RLBossAimMath.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLBossSocketAimTest,
	"ReflectionLab.Boss.SocketAimXY", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLBossSocketAimTest::RunTest(const FString& Parameters)
{
	const FVector Pivot = FVector::ZeroVector;
	const FVector Socket(60.0f, 100.0f, 300.0f);
	const FVector Forward(1.0f, 0.0f, 0.0f);
	const FVector Target(1000.0f, 500.0f, 80.0f);
	float Correction = 0.0f;
	TestTrue(TEXT("Offset barrel can aim at target"),
		RLBossAim::CalculateYawCorrection(Pivot, Socket, Forward, Target, Correction));
	const FRotator Rotation(0.0f, Correction, 0.0f);
	const FVector RotatedSocket = Rotation.RotateVector(Socket);
	const FVector RotatedForward = Rotation.RotateVector(Forward);
	const FVector Delta(Target.X - RotatedSocket.X, Target.Y - RotatedSocket.Y, 0.0f);
	TestTrue(TEXT("Target lies on socket's forward ray"),
		FMath::Abs(Delta.X * RotatedForward.Y - Delta.Y * RotatedForward.X) < 0.1f &&
		FVector::DotProduct(Delta, RotatedForward) > 0.0f);
	float DifferentHeight = 0.0f;
	TestTrue(TEXT("Height variation remains aimable"), RLBossAim::CalculateYawCorrection(
		FVector(0, 0, -2000), FVector(60, 100, 5000), FVector(1, 0, 12), FVector(1000, 500, -5000), DifferentHeight));
	TestTrue(TEXT("Z never changes yaw correction"), FMath::IsNearlyEqual(Correction, DifferentHeight, 0.001f));
	float InvalidCorrection = 0.0f;
	TestFalse(TEXT("Vertical-only socket direction is rejected"), RLBossAim::CalculateYawCorrection(
		Pivot, Socket, FVector::UpVector, Target, InvalidCorrection));
	TestFalse(TEXT("Target inside lateral offset cannot intersect forward ray"), RLBossAim::CalculateYawCorrection(
		Pivot, Socket, Forward, FVector(1, 0, 0), InvalidCorrection));
	return true;
}
#endif
