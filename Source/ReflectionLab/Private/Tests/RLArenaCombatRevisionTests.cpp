#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Framework/RLArenaExperimentComponent.h"
#include "Framework/RLBallisticsExperimentComponent.h"
#include "Framework/RLMirrorArenaComponent.h"
#include "Combat/RLAccelerationBarrier.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Combat/RLProjectileFieldResponseComponent.h"
#include "Combat/RLProjectileMirrorWall.h"
#include "Data/RLArenaExperimentDataAsset.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/PlayerController.h"

namespace
{
	struct FRevisionWorld
	{
		UWorld* World = nullptr;
		FRevisionWorld()
		{
			const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
				.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
			World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
			if (!World) { return; }
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			World->SetGameInstance(NewObject<UGameInstance>(GEngine));
			const FURL URL(nullptr, TEXT("?game=/Script/Engine.GameModeBase"), TRAVEL_Absolute);
			World->SetGameMode(URL); World->InitializeActorsForPlay(URL); World->BeginPlay();
		}
		~FRevisionWorld() { if (World) { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); } }
	};

	URLArenaExperimentDataAsset* LoadExperiment(const TCHAR* Name)
	{
		return LoadObject<URLArenaExperimentDataAsset>(nullptr, *FString::Printf(TEXT("/Game/ReflectionLab/Data/Experiments/%s.%s"), Name, Name));
	}

	TSet<FIntPoint> WalkableCells(UWorld* World, float Radius)
	{
		TSet<FIntPoint> Visited;
		TArray<FIntPoint> Queue;
		Queue.Add(FIntPoint::ZeroValue); Visited.Add(FIntPoint::ZeroValue);
		const FIntPoint Steps[] = {FIntPoint(1,0), FIntPoint(-1,0), FIntPoint(0,1), FIntPoint(0,-1)};
		for (int32 Index = 0; Index < Queue.Num(); ++Index)
		{
			for (const FIntPoint& Step : Steps)
			{
				const FIntPoint Next = Queue[Index] + Step;
				if (FMath::Abs(Next.X) > 17 || FMath::Abs(Next.Y) > 17 || Visited.Contains(Next)) { continue; }
				FHitResult Hit;
				const FVector Start(Queue[Index].X*100, Queue[Index].Y*100, 130);
				const FVector End(Next.X*100, Next.Y*100, 130);
				if (!World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Radius, 130)))
				{ Visited.Add(Next); Queue.Add(Next); }
			}
		}
		return Visited;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLAccelerationBarrierTest, "ReflectionLab.Experiments.AccelerationBarrier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRLAccelerationBarrierTest::RunTest(const FString& Parameters)
{
	FRevisionWorld Context; UWorld* World = Context.World;
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	auto* Authored = LoadExperiment(TEXT("DA_AccelerationField"));
	if (!TestNotNull(TEXT("Authored acceleration settings"), Authored)) { return false; }
	auto* Data = DuplicateObject<URLArenaExperimentDataAsset>(Authored, World);
	TestTrue(TEXT("Return filters enabled in real asset"), Data->Acceleration.bEnableReturnBarriers);
	auto* Owner = World->SpawnActor<AActor>();
	auto* Field = NewObject<URLBallisticsExperimentComponent>(Owner); Field->RegisterComponent();
	if (!TestTrue(TEXT("Configure filters"), Field->Configure(Data))) { return false; }
	int32 FilterCount = 0;
	for (TActorIterator<ARLAccelerationBarrier> It(World); It; ++It) { ++FilterCount; }
	TestEqual(TEXT("Two filters"), FilterCount, 2);
	FHitResult PawnHit;
	TestTrue(TEXT("Player cannot walk through filter"), World->LineTraceSingleByChannel(PawnHit, FVector(0,0,100), FVector(1400,0,100), ECC_Pawn));
	for (int32 Index = 0; Index < 16; ++Index)
	{
		FTransform Spawn;
		TestTrue(TEXT("Enemy spawn behind filter is free"), Field->FindBarrierSpawn(Index, 16, Spawn));
		TestTrue(TEXT("Enemy remains outside filter"), FMath::Abs(Spawn.GetLocation().X) > 1000);
	}
	auto* Shots = World->GetSubsystem<URLProjectilePoolSubsystem>();
	auto* Definition = NewObject<URLProjectileDefinitionDataAsset>(); Definition->ProjectileSpeed = 1000;
	auto* Reflector = World->SpawnActor<ACharacter>(FVector(0,500,96), FRotator::ZeroRotator);
	auto Fire = [&](const FVector& Position, const FVector& Direction, bool bReflected)
	{
		auto* Shot = Shots->AcquireProjectile(ARLProjectile::StaticClass(), FTransform(Direction.Rotation(), Position), Owner, nullptr);
		Shot->InitializeFromDefinition(Definition, nullptr);
		if (bReflected)
		{
			FRLProjectileReflectionParams Params; Params.SpeedMultiplier = 1;
			Shot->Reflect(Reflector, Reflector, Direction, Params);
		}
		return Shot;
	};
	auto* Shot = Fire(FVector(1400,0,80), -FVector::ForwardVector, false);
	Shot->FindComponentByClass<UProjectileMovementComponent>()->TickComponent(0.6f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Hostile shot enters through filter"), Shot->IsPoolActive() && !Shot->IsFadingOut() && Shot->GetActorLocation().X < 950);
	Shots->ReleaseProjectile(Shot);
	Shot = Fire(FVector(0,1600,80), FVector::ForwardVector, true);
	Shot->GetFieldResponse()->SetSpeedMultiplier(2); // Speed alone must never qualify.
	Shot->FindComponentByClass<UProjectileMovementComponent>()->TickComponent(0.8f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Fast but uncharged return is absorbed"), Shot->IsFadingOut() || !Shot->IsPoolActive());
	Shots->ReleaseProjectile(Shot);
	Shot = Fire(FVector(0,0,80), FVector::ForwardVector, true);
	// One large sweep crosses the field and filter before the post-physics experiment tick.
	Shot->FindComponentByClass<UProjectileMovementComponent>()->TickComponent(1.4f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Swept accelerated return passes filter"), Shot->IsPoolActive() && !Shot->IsFadingOut() && Shot->GetActorLocation().X > 1000);
	TestTrue(TEXT("Actual field crossing grants charge"), Shot->GetFieldResponse()->HasAccelerationCharge());
	Shots->ReleaseProjectile(Shot);
	for (const auto& Center : Data->FieldLayout.FieldCenters)
	{
		const FVector Direction = Center.X < 0 ? -FVector::ForwardVector : FVector::ForwardVector;
		Shot = Fire(FVector(0,Center.Y,80), Direction, true);
		Shot->FindComponentByClass<UProjectileMovementComponent>()->TickComponent(1.4f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Every authored field has a usable return route through its filter"),
			Shot->IsPoolActive() && !Shot->IsFadingOut() && FMath::Abs(Shot->GetActorLocation().X)>1000 && Shot->GetFieldResponse()->HasAccelerationCharge());
		Shots->ReleaseProjectile(Shot);
	}
	Shot = Fire(FVector(0,1600,80), FVector::ForwardVector, true);
	TestFalse(TEXT("Pooled charge does not leak"), Shot->GetFieldResponse()->HasAccelerationCharge());
	Shots->ReleaseProjectile(Shot);
	Field->ResetExperiment();
	Data->FieldLayout.FieldCenters = {FVector2D(1300,0)}; // Beyond the filter, not a valid prerequisite.
	TestTrue(TEXT("Reconfigure field beyond filter"), Field->Configure(Data));
	Shot = Fire(FVector(0,0,80), FVector::ForwardVector, true);
	Shot->FindComponentByClass<UProjectileMovementComponent>()->TickComponent(1.4f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("A future field beyond the contact cannot qualify"), Shot->IsFadingOut() || !Shot->IsPoolActive());
	Shots->ReleaseProjectile(Shot);
	Field->ResetExperiment();
	int32 Remaining = 0;
	for (TActorIterator<ARLAccelerationBarrier> It(World); It; ++It) { ++Remaining; }
	TestEqual(TEXT("Cleanup removes filters"), Remaining, 0);
	// The wave-level entry must not teleport the player or rebuild filters again.
	auto* Player = World->SpawnActor<ACharacter>(FVector(0,200,96), FRotator::ZeroRotator);
	auto* Controller = World->SpawnActor<APlayerController>(); Controller->Possess(Player);
	auto* Experiment = NewObject<URLArenaExperimentComponent>(Owner); Experiment->RegisterComponent();
	Experiment->Configure(Data);
	Player->SetActorLocation(FVector(100,200,96));
	Experiment->Configure(Data);
	TestTrue(TEXT("Next acceleration wave preserves player position"), Player->GetActorLocation().Equals(FVector(100,200,96)));
	Experiment->ResetExperiment();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLCrossfireMazeTest, "ReflectionLab.Experiments.CrossfireMaze",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRLCrossfireMazeTest::RunTest(const FString& Parameters)
{
	FRevisionWorld Context; UWorld* World = Context.World;
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	auto* Data = LoadExperiment(TEXT("DA_CrossfireExperiment"));
	if (!TestNotNull(TEXT("Authored maze settings"), Data)) { return false; }
	TestEqual(TEXT("Eight maze walls authored"), Data->Crossfire.MazeWalls.Num(), 8);
	auto* Owner = World->SpawnActor<AActor>();
	auto* Maze = NewObject<URLArenaExperimentComponent>(Owner); Maze->RegisterComponent(); Maze->Configure(Data);
	if (!TestTrue(TEXT("Maze configured"), Maze->HasActiveExperiment())) { return false; }
	for (int32 Phase = 0; Phase < 2; ++Phase)
	{
		Maze->AdvanceExperiment(Phase == 0 ? Data->Crossfire.WarningDuration + Data->Crossfire.ShutterMoveDuration + 0.1f : Data->Crossfire.PhaseDuration);
		int32 Closed = 0;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			if (It->GetStaticMeshComponent()->GetCollisionEnabled() != ECollisionEnabled::NoCollision) { ++Closed; }
		}
		TestEqual(TEXT("Four walls close per phase"), Closed, 4);
		const auto Reachable = WalkableCells(World, 130); // A 260cm corridor envelope, not just one capsule.
		for (const FIntPoint Goal : {FIntPoint(-17,0), FIntPoint(17,0), FIntPoint(0,-17), FIntPoint(0,17)})
		{
			TestTrue(TEXT("Wide routes remain to all four outside lanes"), Reachable.Contains(Goal));
		}
		for (int32 Index = 0; Index < 16; ++Index)
		{
			FTransform Spawn;
			TestTrue(TEXT("Maze keeps enemy spawns free"), Maze->FindTeamSpawn(Index, 16, Spawn));
		}
	}
	Maze->AdvanceExperiment(Data->Crossfire.PhaseDuration); // Group 0 closes again.
	const auto& Panel = Data->Crossfire.MazeWalls[0];
	auto* Shots = World->GetSubsystem<URLProjectilePoolSubsystem>();
	auto* Definition = NewObject<URLProjectileDefinitionDataAsset>(); Definition->ProjectileSpeed = 1000;
	const FVector Launch(Panel.Center.X-150, Panel.Center.Y, 80);
	auto* Bullet = Shots->AcquireProjectile(ARLProjectile::StaticClass(), FTransform(Launch), Owner, nullptr);
	Bullet->InitializeFromDefinition(Definition, nullptr);
	Bullet->FindComponentByClass<UProjectileMovementComponent>()->TickComponent(0.3f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Actual projectile stops at closed maze panel"), Bullet->IsFadingOut() || !Bullet->IsPoolActive());
	Shots->ReleaseProjectile(Bullet);
	Maze->AdvanceExperiment(Data->Crossfire.PhaseDuration);
	Bullet = Shots->AcquireProjectile(ARLProjectile::StaticClass(), FTransform(Launch), Owner, nullptr);
	Bullet->InitializeFromDefinition(Definition, nullptr);
	Bullet->FindComponentByClass<UProjectileMovementComponent>()->TickComponent(0.3f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Actual projectile traverses opened maze panel"), Bullet->IsPoolActive() && !Bullet->IsFadingOut() && Bullet->GetActorLocation().X > Panel.Center.X);
	Shots->ReleaseProjectile(Bullet);
	Maze->ResetExperiment();
	// First group must never rise under a standing player.
	auto* Player = World->SpawnActor<ACharacter>(FVector(Panel.Center.X, Panel.Center.Y, 96), FRotator::ZeroRotator);
	auto* Controller = World->SpawnActor<APlayerController>(); Controller->Possess(Player);
	Maze->Configure(Data); Maze->AdvanceExperiment(Data->Crossfire.WarningDuration + Data->Crossfire.ShutterMoveDuration + 0.1f);
	FHitResult Hit;
	TestFalse(TEXT("Occupied panel remains open"), World->LineTraceSingleByChannel(Hit,
		FVector(Panel.Center.X-150, Panel.Center.Y, 80), FVector(Panel.Center.X+150, Panel.Center.Y, 80), ECC_Visibility));
	Maze->ResetExperiment();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLExpandedMirrorTest, "ReflectionLab.Experiments.ExpandedMirrorRoutes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRLExpandedMirrorTest::RunTest(const FString& Parameters)
{
	FRevisionWorld Context; UWorld* World = Context.World;
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	auto* Data = LoadExperiment(TEXT("DA_MirrorArena"));
	if (!TestNotNull(TEXT("Authored mirror settings"), Data)) { return false; }
	TestTrue(TEXT("Cover extended beyond old 270cm"), Data->Mirror.ShotBarrierLength > 270);
	auto* Owner = World->SpawnActor<AActor>();
	auto* Arena = NewObject<URLMirrorArenaComponent>(Owner); Arena->RegisterComponent();
	if (!TestTrue(TEXT("Expanded mirror arena configured"), Arena->Configure(Data))) { return false; }
	auto* Shots = World->GetSubsystem<URLProjectilePoolSubsystem>();
	auto* Definition = NewObject<URLProjectileDefinitionDataAsset>(); Definition->ProjectileSpeed = 1000;
	auto* Reflector = World->SpawnActor<ACharacter>(FVector(3000,3000,96), FRotator::ZeroRotator);
	for (int32 Index = 0; Index < 8; ++Index)
	{
		FTransform Spawn;
		if (!TestTrue(TEXT("Enemy spawn free"), Arena->FindSpawn(Index, Spawn))) { continue; }
		FVector Enemy = Spawn.GetLocation(); Enemy.Z = 80;
		ARLProjectileMirrorWall* Wall = nullptr;
		for (TActorIterator<ARLProjectileMirrorWall> It(World); It; ++It)
		{
			if (It->GetActorLocation().X * Enemy.X > 0 && It->GetActorLocation().Y * (Enemy.Y >= 0 ? 1 : -1) > 0) { Wall = *It; break; }
		}
		if (!TestNotNull(TEXT("Quadrant mirror exists"), Wall)) { continue; }
		for (float Y : {-180.0f, 0.0f, 180.0f})
		{
			FHitResult Block;
			TestTrue(TEXT("Direct route from central fighting area is covered"), World->SweepSingleByChannel(Block,
				FVector(0,Y,80), Enemy, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(16)));
		}
		for (bool bReturn : {false, true})
		{
			const FVector Start = bReturn ? FVector(0,0,80) : Enemy;
			const FVector Target = bReturn ? Enemy : FVector(0,0,80);
			const FVector Aim = Wall->GetMirroredTarget(Target,16);
			const FVector Direction = (Aim-Start).GetSafeNormal();
			auto* Shot = Shots->AcquireProjectile(ARLProjectile::StaticClass(), FTransform(Direction.Rotation(), Start), Owner, nullptr);
			Shot->InitializeFromDefinition(Definition, nullptr);
			if (bReturn) { FRLProjectileReflectionParams Params; Params.SpeedMultiplier = 1; Shot->Reflect(Reflector, Reflector, Direction, Params); }
			auto* Movement = Shot->FindComponentByClass<UProjectileMovementComponent>();
			const int32 Steps = FMath::RoundToInt(FVector::Dist2D(Start,Aim)/20);
			for (int32 Step = 0; Step < Steps && Shot->IsPoolActive() && !Shot->IsFadingOut(); ++Step) { Movement->TickComponent(0.02f, LEVELTICK_All, nullptr); }
			TestTrue(TEXT("Expanded cover preserves real swept mirror route"), Shot->IsPoolActive() && !Shot->IsFadingOut() && FVector::Dist2D(Shot->GetActorLocation(),Target)<80);
			TestTrue(TEXT("Route actually bounced"), Shot->GetFieldResponse()->GetMirrorBounceCount() > 0);
			Shots->ReleaseProjectile(Shot);
		}
	}
	Arena->ResetArena();
	return true;
}
#endif
