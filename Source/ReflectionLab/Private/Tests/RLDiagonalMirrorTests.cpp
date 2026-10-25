#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Framework/RLMirrorArenaComponent.h"
#include "Framework/RLArenaExperimentComponent.h"
#include "Combat/RLProjectileMirrorWall.h"
#include "Data/RLArenaExperimentDataAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLDiagonalMirrorTest, "ReflectionLab.Experiments.DiagonalMirrors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRLDiagonalMirrorTest::RunTest(const FString& Parameters)
{
	const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	const FURL URL(nullptr,TEXT("?game=/Script/Engine.GameModeBase"),TRAVEL_Absolute);
	World->SetGameMode(URL); World->InitializeActorsForPlay(URL); World->BeginPlay();
	AActor* Owner = World->SpawnActor<AActor>();
	auto* Arena = NewObject<URLMirrorArenaComponent>(Owner); Arena->RegisterComponent();
	auto* Data = NewObject<URLArenaExperimentDataAsset>();
	TestTrue(TEXT("Configure diagonal arena"), Arena->Configure(Data));
	TArray<ARLProjectileMirrorWall*> Mirrors;
	for (TActorIterator<ARLProjectileMirrorWall> It(World); It; ++It) { Mirrors.Add(*It); }
	TestEqual(TEXT("Four diagonal reflectors"), Mirrors.Num(), 4);
	for (int32 Index=0; Index<8; ++Index)
	{
		FTransform Spawn;
		TestTrue(TEXT("Enemy spawn is unobstructed"), Arena->FindSpawn(Index, Spawn));
		const FVector Start(Spawn.GetLocation().X, Spawn.GetLocation().Y, 100);
		const FVector Target(0,0,100);
		for (auto* Wall : Mirrors)
		{
			const FVector WallLocation = Wall->GetActorLocation();
			if (WallLocation.X * Start.X <= 0 || WallLocation.Y * (Start.Y >= 0 ? 1 : -1) <= 0) { continue; }
			const FVector Aim = Wall->GetMirroredTarget(Target,0);
			FHitResult Hit;
			TestTrue(TEXT("Incoming path reaches a surface"), World->LineTraceSingleByChannel(Hit,Start,Aim,ECC_Visibility));
			TestTrue(TEXT("Incoming path reaches intended mirror, not black cover"), Hit.GetActor()==Wall);
			if (Hit.GetActor()==Wall)
			{
				const FVector Direction = (Target-Hit.ImpactPoint).GetSafeNormal();
				FHitResult Obstruction;
				TestFalse(TEXT("Reflected path reaches center through pawn-only rail"),
					World->LineTraceSingleByChannel(Obstruction,Hit.ImpactPoint+Direction*5,Target,ECC_Visibility));
			}
		}
	}
	Arena->ResetArena();
	int32 Remaining=0;
	for (TActorIterator<ARLProjectileMirrorWall> It(World); It; ++It) { ++Remaining; }
	TestEqual(TEXT("Reset removes mirrors"),Remaining,0);
	// The wave-level entry point must retain the camera and player position.
	ACharacter* Player = World->SpawnActor<ACharacter>(FVector(0,0,96),FRotator::ZeroRotator);
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	Controller->Possess(Player);
	auto* Boom = NewObject<USpringArmComponent>(Player);
	Player->AddInstanceComponent(Boom);
	Boom->SetupAttachment(Player->GetRootComponent());
	Boom->RegisterComponent(); Boom->TargetArmLength = 1950;
	Data->Type = ERLArenaExperimentType::MirrorArena;
	auto* Experiment = NewObject<URLArenaExperimentComponent>(Owner); Experiment->RegisterComponent();
	Experiment->Configure(Data);
	for (TObjectIterator<URLMirrorArenaComponent> It; It; ++It)
	{
		if (It->GetOwner()==Owner) { It->TickComponent(1,LEVELTICK_All,nullptr); }
	}
	TestEqual(TEXT("First entry zooms out"),Boom->TargetArmLength,Data->MirrorCameraArmLength);
	Player->SetActorLocation(FVector(100,100,96));
	Experiment->Configure(Data);
	TestEqual(TEXT("Next wave preserves zoom"),Boom->TargetArmLength,Data->MirrorCameraArmLength);
	TestTrue(TEXT("Next wave does not teleport player"),Player->GetActorLocation().Equals(FVector(100,100,96)));
	Experiment->ResetExperiment();
	TestEqual(TEXT("Round exit restores original camera"),Boom->TargetArmLength,1950.0f);
	Experiment->Configure(Data);
	TestEqual(TEXT("New round starts zoom transition afresh"),Boom->TargetArmLength,1950.0f);
	Experiment->ResetExperiment();
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
	return true;
}
#endif
