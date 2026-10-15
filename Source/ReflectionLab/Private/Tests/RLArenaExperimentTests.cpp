#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Framework/RLArenaExperimentComponent.h"
#include "Data/RLArenaExperimentDataAsset.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/RLEnemyPoolSubsystem.h"
#include "Enemies/Components/RLEnemyAttackComponent.h"
#include "Combat/RLProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLArenaExperimentTest, "ReflectionLab.Experiments.CrossfireShutters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLArenaExperimentTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	const FURL URL(nullptr, TEXT("?game=/Script/Engine.GameModeBase"), TRAVEL_Absolute);
	World->SetGameMode(URL); World->InitializeActorsForPlay(URL); World->BeginPlay();
	AActor* Owner = World->SpawnActor<AActor>();
	URLArenaExperimentComponent* Experiment = NewObject<URLArenaExperimentComponent>(Owner);
	Experiment->RegisterComponent();
	URLArenaExperimentDataAsset* Data = NewObject<URLArenaExperimentDataAsset>();
	Data->ProjectileDefinition = NewObject<URLProjectileDefinitionDataAsset>();
	TestTrue(TEXT("First phase left"), Data->IsLeftPhase(0));
	TestFalse(TEXT("Second phase right"), Data->IsLeftPhase(6.1f));
	Experiment->Configure(Data);
	FTransform Left, Right;
	TestTrue(TEXT("Left spawn available"), Experiment->FindTeamSpawn(0, 8, Left));
	TestTrue(TEXT("Right spawn available"), Experiment->FindTeamSpawn(1, 8, Right));
	TestTrue(TEXT("Separated spawn banks"), Left.GetLocation().X < -1000 && Right.GetLocation().X > 1000);
	auto* Pool = World->GetSubsystem<URLEnemyPoolSubsystem>();
	ARLEnemyCharacter* LeftEnemy = Pool->AcquireEnemy(ARLEnemyCharacter::StaticClass(), Left);
	ARLEnemyCharacter* RightEnemy = Pool->AcquireEnemy(ARLEnemyCharacter::StaticClass(), Right);
	Experiment->RegisterEnemy(LeftEnemy, 0);
	Experiment->RegisterEnemy(RightEnemy, 1);
	auto* LeftAttack = LeftEnemy->FindComponentByClass<URLEnemyAttackComponent>();
	auto* RightAttack = RightEnemy->FindComponentByClass<URLEnemyAttackComponent>();
	auto IsBlocked = [World](float X)
	{
		FHitResult Hit;
		return World->LineTraceSingleByChannel(Hit, FVector(0,0,80), FVector(X,0,80), ECC_Visibility);
	};
	Experiment->AdvanceExperiment(0.5f);
	TestTrue(TEXT("Both banks keep firing during warning"), LeftAttack->IsExperimentAttackAllowed() && RightAttack->IsExperimentAttackAllowed());
	TestFalse(TEXT("Warning does not close right gate"), IsBlocked(1400));
	Experiment->AdvanceExperiment(1.0f);
	TestTrue(TEXT("Left bank allowed"), LeftAttack->IsExperimentAttackAllowed());
	TestTrue(TEXT("Right bank attacks behind closed cover"), RightAttack->IsExperimentAttackAllowed());
	TestTrue(TEXT("Right gate blocks hostile and reflected projectile traces"), IsBlocked(1400));
	TestFalse(TEXT("Left gate open"), IsBlocked(-1400));
	ACharacter* Reflector = World->SpawnActor<ACharacter>(FVector(0,1500,96), FRotator::ZeroRotator);
	for (const bool bReflected : {false, true})
	{
		ARLProjectile* Bullet = World->SpawnActor<ARLProjectile>();
		URLProjectileDefinitionDataAsset* BulletData = NewObject<URLProjectileDefinitionDataAsset>();
		BulletData->ProjectileSpeed = 1000;
		Bullet->ActivateProjectile(FTransform(FVector(0,0,80)), Owner, nullptr);
		Bullet->InitializeFromDefinition(BulletData, nullptr);
		if (bReflected)
		{
			FRLProjectileReflectionParams Reflection;
			Bullet->Reflect(Reflector, Reflector, FVector::ForwardVector, Reflection);
		}
		Bullet->FindComponentByClass<UProjectileMovementComponent>()->TickComponent(0.8f, LEVELTICK_All, nullptr);
		TestTrue(bReflected ? TEXT("Reflected bullet stops at closed shutter") : TEXT("Hostile bullet stops at closed shutter"),
			Bullet->GetActorLocation().X < 500 && (Bullet->IsFadingOut() || !Bullet->IsPoolActive()));
		Bullet->Destroy();
	}
	Experiment->AdvanceExperiment(6.0f);
	TestTrue(TEXT("Left bank keeps attacking on swap"), LeftAttack->IsExperimentAttackAllowed());
	TestTrue(TEXT("Right bank allowed on swap"), RightAttack->IsExperimentAttackAllowed());
	TestTrue(TEXT("Left gate closes on swap"), IsBlocked(-1400));
	TestFalse(TEXT("Right gate opens on swap"), IsBlocked(1400));
	ARLProjectile* ThroughBullet = World->SpawnActor<ARLProjectile>();
	ThroughBullet->ActivateProjectile(FTransform(FVector(0,0,80)), Owner, nullptr);
	URLProjectileDefinitionDataAsset* ThroughData = NewObject<URLProjectileDefinitionDataAsset>();
	ThroughData->ProjectileSpeed = 1000;
	ThroughBullet->InitializeFromDefinition(ThroughData, nullptr);
	ThroughBullet->FindComponentByClass<UProjectileMovementComponent>()->TickComponent(0.8f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Bullet passes through open shutter"), ThroughBullet->GetActorLocation().X > 500 && !ThroughBullet->IsFadingOut());
	ThroughBullet->Destroy();
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACharacter* Player = World->SpawnActor<ACharacter>(FVector(500,0,96), FRotator::ZeroRotator, Spawn);
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	Controller->Possess(Player);
	Experiment->AdvanceExperiment(6.0f);
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		if (It->GetActorLocation().X > 0 && It->GetActorScale3D().Z > 1)
		{
			TestTrue(TEXT("Occupied gate stays beneath floor"), It->GetActorLocation().Z < 0);
			TestEqual(TEXT("Occupied gate has no collision"), It->GetStaticMeshComponent()->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		}
	}
	Experiment->ResetExperiment();
	TestTrue(TEXT("Reset clears enemy attack gates"), LeftAttack->IsExperimentAttackAllowed() && RightAttack->IsExperimentAttackAllowed());
	TestFalse(TEXT("Reset disables experiment"), Experiment->HasActiveExperiment());
	TestFalse(TEXT("Reset removes barrier collision"), IsBlocked(-1400));
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
	return true;
}
#endif
