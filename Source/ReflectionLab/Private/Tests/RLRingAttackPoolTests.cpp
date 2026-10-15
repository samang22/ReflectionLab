#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Combat/RLExpandingRingAttack.h"
#include "Combat/RLRingAttackPoolSubsystem.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLRingWallPoolTest, "ReflectionLab.Combat.RingWallPool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLRingWallPoolTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	const FURL URL(nullptr, TEXT("?game=/Script/Engine.GameModeBase"), TRAVEL_Absolute);
	World->SetGameMode(URL);
	World->InitializeActorsForPlay(URL);
	World->BeginPlay();
	auto AddBlocker = [World](const FVector& Location, const FVector& Extent)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
		Actor->SetRootComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionObjectType(ECC_WorldStatic);
		Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->RegisterComponent();
		Box->SetWorldLocation(Location);
		return Box;
	};
	AddBlocker(FVector(0, 0, -5), FVector(2000, 2000, 5));
	UBoxComponent* Wall = AddBlocker(FVector(300, 0, 200), FVector(20, 500, 200));
	URLRingAttackDataAsset* Data = NewObject<URLRingAttackDataAsset>();
	Data->Settings.StartRadius = 50;
	Data->Settings.ExpansionSpeed = 1000;
	URLRingAttackPoolSubsystem* Pool = World->GetSubsystem<URLRingAttackPoolSubsystem>();
	ARLExpandingRingAttack* Ring = Pool->AcquireRing(FTransform::Identity, nullptr, nullptr, Data, false);
	if (TestNotNull(TEXT("Ring"), Ring))
	{
		TestTrue(TEXT("Manual tutorial ring waits in active pool state"), Ring->IsPoolActive());
		TestTrue(TEXT("Start ring"), Ring->StartAttack());
		Ring->Tick(0.1f);
		TestTrue(TEXT("Before wall remains active"), Ring->IsPoolActive());
		Ring->Tick(0.2f);
		TestFalse(TEXT("Fast expansion across wall returns to pool"), Ring->IsPoolActive());
		TestTrue(TEXT("Returned ring hidden"), Ring->IsHidden());
		TestFalse(TEXT("Returned ring stops ticking"), Ring->IsActorTickEnabled());
		Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ARLExpandingRingAttack* Reused = Pool->AcquireRing(FTransform::Identity, nullptr, nullptr, Data, true);
		TestEqual(TEXT("Same ring reused"), Reused, Ring);
		Ring->Tick(1.0f);
		TestTrue(TEXT("Ground does not count as a wall"), Ring->IsPoolActive());
		Ring->BeginFadeOut();
		Ring->Tick(1.0f);
		TestFalse(TEXT("Timed fade also returns to pool"), Ring->IsPoolActive());
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
