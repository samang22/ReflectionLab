#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Framework/RLArenaVariationComponent.h"
#include "Data/RLArenaExperimentDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/RLEnemyPoolSubsystem.h"
#include "Enemies/Components/RLEnemyAttackComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLArenaVariationTest, "ReflectionLab.Experiments.SupportAndZones",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRLArenaVariationTest::RunTest(const FString& Parameters)
{
	const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	const FURL URL(nullptr, TEXT("?game=/Script/Engine.GameModeBase"), TRAVEL_Absolute);
	World->SetGameMode(URL); World->InitializeActorsForPlay(URL); World->BeginPlay();
	AActor* Owner = World->SpawnActor<AActor>();
	auto* Variation = NewObject<URLArenaVariationComponent>(Owner); Variation->RegisterComponent();
	auto* Data = NewObject<URLArenaExperimentDataAsset>();
	Data->Type = ERLArenaExperimentType::SupportNetwork;
	Data->SupportEnemyClass = ARLEnemyCharacter::StaticClass();
	TestTrue(TEXT("Support configuration"), Variation->Configure(Data));
	auto* Pool = World->GetSubsystem<URLEnemyPoolSubsystem>();
	auto* Support = Pool->AcquireEnemy(ARLEnemyCharacter::StaticClass(), FTransform(FVector(0,0,96)));
	auto* Recipient = Pool->AcquireEnemy(ARLEnemyCharacter::StaticClass(), FTransform(FVector(400,0,96)));
	Variation->RegisterEnemy(Support, 0); Variation->RegisterEnemy(Recipient, 1);
	auto* Attack = Recipient->FindComponentByClass<URLEnemyAttackComponent>();
	Variation->Advance(0.1f);
	TestEqual(TEXT("Nearby recipient buffed"), Attack->GetSupportAttackRate(), 1.5f);
	Variation->Advance(0.1f);
	TestEqual(TEXT("Buff does not compound"), Attack->GetSupportAttackRate(), 1.5f);
	Recipient->SetActorLocation(FVector(1400,0,96)); Variation->Advance(0.1f);
	TestEqual(TEXT("Leaving radius clears buff"), Attack->GetSupportAttackRate(), 1.0f);
	Recipient->SetActorLocation(FVector(400,0,96)); Variation->Advance(0.1f);
	Support->ReturnToPool(); Variation->Advance(0.1f);
	TestEqual(TEXT("Support death clears buff"), Attack->GetSupportAttackRate(), 1.0f);
	Variation->ResetVariation();
	Data->Type = ERLArenaExperimentType::AlternatingZones;
	Data->PhaseDuration = 8; Data->WarningDuration = 2.5f;
	TestTrue(TEXT("Zone configuration"), Variation->Configure(Data));
	Variation->Advance(2.4f);
	TestFalse(TEXT("Warning cannot hurt"), Variation->IsZoneDangerous(FVector(-500,0,0)));
	Variation->Advance(0.2f);
	TestTrue(TEXT("Left active after warning"), Variation->IsZoneDangerous(FVector(-500,0,9999)));
	TestFalse(TEXT("Opposite side safe"), Variation->IsZoneDangerous(FVector(500,0,0)));
	TestFalse(TEXT("Center crossing safe"), Variation->IsZoneDangerous(FVector(0,0,0)));
	Variation->Advance(8);
	TestTrue(TEXT("Next side active"), Variation->IsZoneDangerous(FVector(500,0,0)));
	TestFalse(TEXT("Old side safe"), Variation->IsZoneDangerous(FVector(-500,0,0)));
	Variation->ResetVariation();
	TestFalse(TEXT("Cleanup disables hazard"), Variation->IsZoneDangerous(FVector(500,0,0)));
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
