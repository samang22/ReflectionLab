#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Data/RLCombatPacingSettings.h"
#include "Data/RLEnemyCombatRow.h"
#include "Framework/RLCombatPacingComponent.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/Components/RLEnemyAttackComponent.h"
#include "Enemies/Components/RLEnemySpawnVisualComponent.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLCleanupPolicyTest, "ReflectionLab.Combat.CleanupPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRLCleanupPolicyTest::RunTest(const FString& Parameters)
{
	FRLCombatPacingSettings Settings;
	TestEqual(TEXT("Large encounter uses normal timing"), Settings.GetAttackRate(4), 1.0f);
	TestTrue(TEXT("Three enemies receive mild acceleration"), Settings.GetAttackRate(3) > 1);
	TestTrue(TEXT("Acceleration grows as enemies disappear"), Settings.GetAttackRate(2) > Settings.GetAttackRate(3));
	TestTrue(TEXT("Last enemy receives greatest acceleration"), Settings.GetAttackRate(1) > Settings.GetAttackRate(2));
	TestEqual(TEXT("Empty encounter cannot request acceleration"), Settings.GetAttackRate(0), 1.0f);
	TestTrue(TEXT("Approaching shot counts despite different height"),
		Settings.IsApproaching(FVector(500,0,1000), FVector(-1000,0,0), FVector::ZeroVector));
	TestFalse(TEXT("Missed shot no longer blocks next attack"),
		Settings.IsApproaching(FVector(-500,0,0), FVector(-1000,0,0), FVector::ZeroVector));
	TestFalse(TEXT("Passing far to the side is not an opportunity"),
		Settings.IsApproaching(FVector(500,800,0), FVector(-1000,0,0), FVector::ZeroVector));
	TestFalse(TEXT("Distant slow shot does not stall cleanup indefinitely"),
		Settings.IsApproaching(FVector(10000,0,0), FVector(-100,0,0), FVector::ZeroVector));
	Settings.bEnabled = false;
	TestEqual(TEXT("Asset toggle disables pacing"), Settings.GetAttackRate(1), 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLCleanupLifecycleTest, "ReflectionLab.Combat.CleanupLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRLCleanupLifecycleTest::RunTest(const FString& Parameters)
{
	const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	const FURL URL(nullptr, TEXT("?game=/Script/Engine.GameModeBase"), TRAVEL_Absolute);
	World->SetGameMode(URL); World->InitializeActorsForPlay(URL); World->BeginPlay();
	FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Player = World->SpawnActor<ACharacter>(FVector(0,0,96), FRotator::ZeroRotator, Spawn);
	auto* Controller = World->SpawnActor<APlayerController>(); Controller->Possess(Player);
	auto* Enemy = World->SpawnActor<ARLEnemyCharacter>(FVector(1000,0,96), FRotator::ZeroRotator, Spawn);
	Enemy->FindComponentByClass<URLEnemySpawnVisualComponent>()->CancelSpawn();
	Enemy->SetTutorialMovementLocked(true);
	auto* Attack = Enemy->FindComponentByClass<URLEnemyAttackComponent>();
	FRLEnemyCombatRow Config; Config.InitialFireDelay = 10; Config.AttackInterval = 10; Config.ShotsPerBurst = 1;
	Attack->Configure(Config, nullptr, nullptr, false);
	int32 ShotCount = 0;
	Attack->OnFireShot.BindLambda([&ShotCount]() { ++ShotCount; });
	auto* Owner = World->SpawnActor<AActor>();
	auto* Pacing = NewObject<URLCombatPacingComponent>(Owner); Pacing->RegisterComponent();
	FRLCombatPacingSettings Settings;
	Pacing->Configure(Settings, true);
	Pacing->SetComponentTickEnabled(false); // This harness advances the coordinator explicitly.
	Attack->StartFiring();
	for (int32 Step = 0; Step < 10; ++Step) { Pacing->UpdatePacing(); ++GFrameCounter; World->Tick(LEVELTICK_All, 0.1f); }
	TestTrue(TEXT("Empty screen shortens an existing ten-second wait"), ShotCount > 0);
	TestEqual(TEXT("Last enemy receives configured acceleration"), Attack->GetCleanupAttackRate(), Settings.LastEnemyAttackRate);
	Attack->StopFiring(); ShotCount = 0;
	Config.InitialFireDelay = 0.1f;
	Attack->Configure(Config, nullptr, nullptr, false);
	Attack->StartFiring();
	auto* Definition = NewObject<URLProjectileDefinitionDataAsset>(); Definition->ProjectileSpeed = 1000;
	auto* Pool = World->GetSubsystem<URLProjectilePoolSubsystem>();
	auto* Shot = Pool->AcquireProjectile(ARLProjectile::StaticClass(), FTransform(FRotator(0,180,0), FVector(500,0,96)), Enemy, Enemy);
	Shot->InitializeFromDefinition(Definition, Player);
	Shot->FindComponentByClass<UProjectileMovementComponent>()->SetComponentTickEnabled(false);
	Shot->SetActorTickEnabled(false);
	for (int32 Step = 0; Step < 10; ++Step) { Pacing->UpdatePacing(); ++GFrameCounter; World->Tick(LEVELTICK_All, 0.1f); }
	TestEqual(TEXT("Approaching shot holds the next burst even when its timer expires"), ShotCount, 0);
	Pool->ReleaseProjectile(Shot);
	for (int32 Step = 0; Step < 3; ++Step) { Pacing->UpdatePacing(); ++GFrameCounter; World->Tick(LEVELTICK_All, 0.1f); }
	TestTrue(TEXT("Returned shot releases the attack without a fresh long cooldown"), ShotCount > 0);
	Pacing->Configure(Settings, false);
	Pacing->UpdatePacing();
	TestEqual(TEXT("Tutorial and custom enemy encounters opt out"), Attack->GetCleanupAttackRate(), 1.0f);
	Attack->SetCleanupPressure(2, true, 0.7f, 0.15f);
	Enemy->ReturnToPool();
	TestEqual(TEXT("Pool return clears cleanup rate"), Attack->GetCleanupAttackRate(), 1.0f);
	Pacing->ResetPacing();
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
	return true;
}
#endif
