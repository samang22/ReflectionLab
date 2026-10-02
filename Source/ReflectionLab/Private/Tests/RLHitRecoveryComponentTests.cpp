#include "Player/Components/RLHitRecoveryComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLHitRecoveryComponentTest, "ReflectionLab.Player.HitRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLHitRecoveryComponentTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues WorldSettings = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &WorldSettings);
	if (!TestNotNull(TEXT("Recovery test world"), World)) { return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	const FURL TestURL(nullptr, TEXT("?game=/Script/Engine.GameModeBase"), TRAVEL_Absolute);
	World->SetGameMode(TestURL);
	World->InitializeActorsForPlay(TestURL);
	World->BeginPlay();
	ACharacter* Character = World->SpawnActor<ACharacter>();
	if (TestNotNull(TEXT("Recovery owner"), Character))
	{
		URLHitRecoveryComponent* Recovery = NewObject<URLHitRecoveryComponent>(Character);
		Recovery->RegisterComponent();
		FRLHitRecoverySettings Tuning;
		Tuning.Duration = 0.6f;
		Tuning.MovementSpeedMultiplier = 0.25f;
		Recovery->Configure(Tuning);
		UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		Movement->MaxWalkSpeed = 600.0f;
		int32 StartedCount = 0;
		int32 EndedCount = 0;
		Recovery->OnRecoveryStarted.AddLambda([&StartedCount]() { ++StartedCount; });
		Recovery->OnRecoveryEnded.AddLambda([&EndedCount]() { ++EndedCount; });
		++GFrameCounter;
		World->GetTimerManager().Tick(0.0f);
		TestTrue(TEXT("Recovery starts"), Recovery->BeginRecovery());
		TestEqual(TEXT("Movement slowed"), Movement->MaxWalkSpeed, 150.0f);
		TestFalse(TEXT("Duplicate recovery rejected"), Recovery->BeginRecovery());
		++GFrameCounter;
		World->GetTimerManager().Tick(0.7f);
		TestFalse(TEXT("Recovery timer ends state"), Recovery->IsRecovering());
		TestEqual(TEXT("Movement restored"), Movement->MaxWalkSpeed, 600.0f);
		TestEqual(TEXT("One start event"), StartedCount, 1);
		TestEqual(TEXT("One end event"), EndedCount, 1);
		Recovery->EndRecovery();
		TestEqual(TEXT("Repeated cleanup does not emit end"), EndedCount, 1);
		Tuning.Duration = 0.0f;
		Recovery->Configure(Tuning);
		Movement->MaxWalkSpeed = 0.0f;
		TestTrue(TEXT("Zero duration accepted"), Recovery->BeginRecovery());
		TestFalse(TEXT("Zero duration ends immediately"), Recovery->IsRecovering());
		TestEqual(TEXT("Zero walk speed preserved"), Movement->MaxWalkSpeed, 0.0f);
		Tuning.Duration = 0.6f;
		Recovery->Configure(Tuning);
		Recovery->BeginRecovery();
		Recovery->EndRecovery();
		TestFalse(TEXT("Cancellation clears active state"), Recovery->IsRecovering());
		Recovery->OnRecoveryStarted.Clear();
		Recovery->OnRecoveryEnded.Clear();
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
