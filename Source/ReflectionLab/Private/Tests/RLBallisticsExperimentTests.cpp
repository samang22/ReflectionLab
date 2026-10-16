#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Framework/RLBallisticsExperimentComponent.h"
#include "Data/RLArenaExperimentDataAsset.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLProjectileFieldResponseComponent.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLBallisticsExperimentTest, "ReflectionLab.Experiments.BallisticsFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRLBallisticsExperimentTest::RunTest(const FString& Parameters)
{
	const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	const FURL URL(nullptr, TEXT("?game=/Script/Engine.GameModeBase"), TRAVEL_Absolute);
	World->SetGameMode(URL); World->InitializeActorsForPlay(URL); World->BeginPlay();
	auto* Owner = World->SpawnActor<AActor>();
	auto* Experiment = NewObject<URLBallisticsExperimentComponent>(Owner); Experiment->RegisterComponent();
	auto* Settings = NewObject<URLArenaExperimentDataAsset>();
	Settings->Type = ERLArenaExperimentType::AccelerationField;
	Settings->FieldCenters = {FVector2D::ZeroVector}; Settings->FieldHalfExtent = FVector2D(100,500);
	TestTrue(TEXT("Acceleration config"), Experiment->Configure(Settings));
	auto* Definition = NewObject<URLProjectileDefinitionDataAsset>(); Definition->ProjectileSpeed = 1000;
	auto* Pool = World->GetSubsystem<URLProjectilePoolSubsystem>();
	auto* Bullet = Pool->AcquireProjectile(ARLProjectile::StaticClass(), FTransform(FVector(-300,0,80)), Owner, nullptr);
	Bullet->InitializeFromDefinition(Definition, nullptr);
	auto* Movement = Bullet->FindComponentByClass<UProjectileMovementComponent>();
	Movement->TickComponent(0.6f, LEVELTICK_All, nullptr);
	Experiment->Advance();
	TestEqual(TEXT("Swept crossing accelerates even with both endpoints outside"), Movement->Velocity.Size(), 1500.0);
	Experiment->Advance();
	TestEqual(TEXT("Repeated processing does not stack"), Movement->Velocity.Size(), 1500.0);
	Bullet->SetProjectileSpeed(800, FVector::ForwardVector);
	TestEqual(TEXT("Special movement retains environmental multiplier"), Movement->Velocity.Size(), 1200.0);
	Experiment->ResetExperiment();
	TestEqual(TEXT("Experiment cleanup restores unmodified speed"), Movement->Velocity.Size(), 800.0);
	Settings->BaselineHostileSpeedMultiplier = 0.65f;
	TestTrue(TEXT("Slow baseline config"), Experiment->Configure(Settings));
	Bullet->ActivateProjectile(FTransform(FVector(-600,0,80)), Owner, nullptr);
	Bullet->InitializeFromDefinition(Definition, nullptr); Experiment->Advance();
	TestTrue(TEXT("Unaccelerated hostile is slower from data"), FMath::IsNearlyEqual(Movement->Velocity.Size(), 650.0, 0.01));
	Experiment->Advance();
	TestTrue(TEXT("Baseline does not compound"), FMath::IsNearlyEqual(Movement->Velocity.Size(), 650.0, 0.01));
	Movement->TickComponent(1.2f, LEVELTICK_All, nullptr); Experiment->Advance();
	TestTrue(TEXT("Field boosts to authored speed, not slow baseline times boost"), FMath::IsNearlyEqual(Movement->Velocity.Size(), 1500.0, 0.01));
	Experiment->ResetExperiment();
	TestTrue(TEXT("Refraction config"), [&] { Settings->Type = ERLArenaExperimentType::RefractionGate; return Experiment->Configure(Settings); }());
	Bullet->ActivateProjectile(FTransform(FVector(-300,0,999)), Owner, nullptr);
	Bullet->InitializeFromDefinition(Definition, nullptr);
	Movement->TickComponent(0.6f, LEVELTICK_All, nullptr); Experiment->Advance();
	TestTrue(TEXT("Refraction angle ignores height"), FMath::IsNearlyEqual(Movement->Velocity.Rotation().Yaw, 35.0, 0.01));
	TestEqual(TEXT("Refraction preserves speed"), Movement->Velocity.Size(), 1000.0);
	Experiment->Advance();
	TestTrue(TEXT("No repeated refraction"), FMath::IsNearlyEqual(Movement->Velocity.Rotation().Yaw, 35.0, 0.01));
	auto* Reflector = World->SpawnActor<ACharacter>(FVector(1000,1000,96), FRotator::ZeroRotator);
	FRLProjectileReflectionParams Params;
	TestTrue(TEXT("Reflect active shot"), Bullet->Reflect(Reflector, Reflector, -FVector::ForwardVector, Params));
	Movement->TickComponent(0.6f, LEVELTICK_All, nullptr); Experiment->Advance();
	TestTrue(TEXT("Returning shot refracts again"), FMath::IsNearlyEqual(Movement->Velocity.Rotation().Yaw, -145.0, 0.01));
	const auto Serial = Bullet->GetFieldResponse()->GetActivationSerial();
	Pool->ReleaseProjectile(Bullet);
	auto* Reused = Pool->AcquireProjectile(ARLProjectile::StaticClass(), FTransform(FVector(-300,0,80)), Owner, nullptr);
	TestEqual(TEXT("Pool reused same actor"), Reused, Bullet);
	TestTrue(TEXT("Reuse has fresh flight identity"), Reused->GetFieldResponse()->GetActivationSerial() != Serial);
	Reused->InitializeFromDefinition(Definition, nullptr);
	Movement->TickComponent(0.6f, LEVELTICK_All, nullptr); Experiment->Advance();
	TestTrue(TEXT("Pooled shot can refract anew"), FMath::IsNearlyEqual(Movement->Velocity.Rotation().Yaw, 35.0, 0.01));
	Experiment->ResetExperiment();
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
