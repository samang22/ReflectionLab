#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Framework/RLMirrorArenaComponent.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLProjectileMirrorWall.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Data/RLArenaExperimentDataAsset.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/RLEnemyPoolSubsystem.h"
#include "Enemies/Components/RLEnemyShieldComponent.h"
#include "Player/Components/RLHealthComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLMirrorAndSupportTest,"ReflectionLab.Experiments.MirrorAndSupportShields",
	EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRLMirrorAndSupportTest::RunTest(const FString& Parameters)
{
	const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
	if (!TestNotNull(TEXT("World"),World)) { return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	const FURL URL(nullptr,TEXT("?game=/Script/Engine.GameModeBase"),TRAVEL_Absolute);
	World->SetGameMode(URL); World->InitializeActorsForPlay(URL); World->BeginPlay();
	auto* Owner=World->SpawnActor<AActor>();
	auto* Pool=World->GetSubsystem<URLEnemyPoolSubsystem>();
	auto* Source=Pool->AcquireEnemy(ARLEnemyCharacter::StaticClass(),FTransform(FVector(1500,600,96)));
	auto* Recipient=Pool->AcquireEnemy(ARLEnemyCharacter::StaticClass(),FTransform(FVector(1500,900,96)));
	auto* Shield=Recipient->FindComponentByClass<URLEnemyShieldComponent>();
	Recipient->FindComponentByClass<URLHealthComponent>()->InitializeHealth(5);
	Shield->SetSupportSource(Source);
	TestTrue(TEXT("Source-bound protection enabled"),Shield->IsSupportProtected());
	TestEqual(TEXT("All damage blocked while source lives"),UGameplayStatics::ApplyDamage(Recipient,1,nullptr,Owner,nullptr),0.0f);
	TestTrue(TEXT("First guard hit cannot break support protection"),Shield->TryAbsorbReflectedProjectile());
	TestTrue(TEXT("Second guard hit still cannot break protection"),Shield->TryAbsorbReflectedProjectile());
	Source->ReturnToPool();
	TestFalse(TEXT("Source death immediately disables protection before next arena tick"),Shield->IsSupportProtected());
	TestEqual(TEXT("Health damage resumes"),UGameplayStatics::ApplyDamage(Recipient,1,nullptr,Owner,nullptr),1.0f);
	auto* ReusedSource=Pool->AcquireEnemy(ARLEnemyCharacter::StaticClass(),FTransform(FVector(1500,600,96)));
	TestEqual(TEXT("Source actor recycled"),ReusedSource,Source);
	TestFalse(TEXT("Recycled source cannot resurrect old protection"),Shield->IsSupportProtected());
	Recipient->ReturnToPool(); ReusedSource->ReturnToPool();
	auto* Arena=NewObject<URLMirrorArenaComponent>(Owner); Arena->RegisterComponent();
	auto* Data=NewObject<URLArenaExperimentDataAsset>(); Data->Type=ERLArenaExperimentType::MirrorArena;
	TestTrue(TEXT("Mirror arena config"),Arena->Configure(Data));
	FTransform EnemySpawn;
	TestTrue(TEXT("Side pocket spawn"),Arena->FindSpawn(1,EnemySpawn));
	FHitResult Hit;
	TestTrue(TEXT("Direct fire lane blocked"),World->LineTraceSingleByChannel(Hit,FVector(1100,0,80),FVector(0,0,80),ECC_Visibility));
	TestTrue(TEXT("Full-height collision remains above the low visual"),World->LineTraceSingleByChannel(Hit,FVector(1100,0,150),FVector(0,0,150),ECC_Visibility));
	TestTrue(TEXT("Low cyan rails still block pawn movement"),World->LineTraceSingleByChannel(Hit,FVector(0,0,100),FVector(0,800,100),ECC_Pawn));
	for (TActorIterator<AStaticMeshActor> It(World);It;++It)
	{
		auto* Visual=It->GetStaticMeshComponent();
		const auto* Collision=Cast<UPrimitiveComponent>(It->GetRootComponent());
		const bool bShotBlocker=Collision && Collision->GetCollisionObjectType()==ECC_WorldStatic;
		const float ExpectedHeight=bShotBlocker ? FMath::Max(Data->ShotBarrierVisualHeight,Data->LaneBarrierVisualHeight+5.0f) : Data->LaneBarrierVisualHeight;
		TestTrue(TEXT("Barrier visual matches its role-specific height"),FMath::IsNearlyEqual(Visual->Bounds.GetBox().Max.Z,ExpectedHeight,0.1f));
		if (bShotBlocker) { TestTrue(TEXT("Bullet cover sits above movement rails"),Visual->Bounds.GetBox().Max.Z>Data->LaneBarrierVisualHeight); }
	}
	ARLProjectileMirrorWall* Mirror=nullptr;
	ARLProjectileMirrorWall* BottomMirror=nullptr;
	int32 MirrorCount=0;
	for(TActorIterator<ARLProjectileMirrorWall> It(World);It;++It)
	{
		++MirrorCount;
		if(It->GetActorLocation().X>0 && It->GetActorLocation().Y>0) { Mirror=*It; }
		if(It->GetActorLocation().X>0 && It->GetActorLocation().Y<0) { BottomMirror=*It; }
	}
	TestEqual(TEXT("Four reflector walls created"),MirrorCount,4);
	if(TestNotNull(TEXT("Reflecting wall"),Mirror))
	{
		auto* Shots=World->GetSubsystem<URLProjectilePoolSubsystem>();
		auto* Definition=NewObject<URLProjectileDefinitionDataAsset>(); Definition->ProjectileSpeed=1000;
		auto* Shot=Shots->AcquireProjectile(ARLProjectile::StaticClass(),FTransform((Mirror->GetMirroredTarget(FVector(0,0,80),16)-FVector(1100,0,80)).Rotation(),FVector(1100,0,80)),Owner,nullptr);
		Shot->InitializeFromDefinition(Definition,nullptr);
		auto* Movement=Shot->FindComponentByClass<UProjectileMovementComponent>();
		const int32 RouteSteps=FMath::RoundToInt(FVector::Dist2D(Mirror->GetMirroredTarget(FVector(0,0,80),16),FVector(1100,0,80))/100);
		for(int32 Step=0;Step<RouteSteps-5;++Step) { Movement->TickComponent(0.1f,LEVELTICK_All,nullptr); }
		AddInfo(FString::Printf(TEXT("Diagonal shot steps=%d location=%s velocity=%s"),RouteSteps,*Shot->GetActorLocation().ToString(),*Movement->Velocity.ToString()));
		TestTrue(TEXT("Hostile shot survives real mirror collision"),Shot->IsPoolActive()&&!Shot->IsFadingOut());
		TestTrue(TEXT("Engine bounce turns velocity back into arena"),Movement->Velocity.Y<0);
		TestFalse(TEXT("Wall bounce does not mark a hostile as player-reflected"),Shot->IsReflected());
		for(int32 Step=0;Step<5;++Step) { Movement->TickComponent(0.1f,LEVELTICK_All,nullptr); }
		TestTrue(TEXT("Mirrored enemy aim reaches player lane"),Shot->GetActorLocation().Size2D()<100);
		Shots->ReleaseProjectile(Shot);
		if (TestNotNull(TEXT("Additional bottom mirror"),BottomMirror))
		{
			Shot=Shots->AcquireProjectile(ARLProjectile::StaticClass(),FTransform((BottomMirror->GetMirroredTarget(FVector(0,0,80),16)-FVector(1100,0,80)).Rotation(),FVector(1100,0,80)),Owner,nullptr);
			Shot->InitializeFromDefinition(Definition,nullptr);
			Movement=Shot->FindComponentByClass<UProjectileMovementComponent>();
			for(int32 Step=0;Step<RouteSteps;++Step) { Movement->TickComponent(0.1f,LEVELTICK_All,nullptr); }
			TestTrue(TEXT("Additional bottom route reaches central lane"),Shot->IsPoolActive()&&!Shot->IsFadingOut()&&Shot->GetActorLocation().Size2D()<100);
			Shots->ReleaseProjectile(Shot);
		}
		Shot=Shots->AcquireProjectile(ARLProjectile::StaticClass(),FTransform(FRotator(0,45,0),FVector(0,0,80)),Owner,nullptr);
		Shot->InitializeFromDefinition(Definition,nullptr);
		Movement=Shot->FindComponentByClass<UProjectileMovementComponent>();
		for(int32 Step=0;Step<180 && !Shot->IsFadingOut();++Step) { Movement->TickComponent(0.1f,LEVELTICK_All,nullptr); }
		TestTrue(TEXT("Mirror bounce limit prevents indefinite ricochets"),Shot->IsFadingOut());
		Shots->ReleaseProjectile(Shot);
		auto* Victim=Pool->AcquireEnemy(ARLEnemyCharacter::StaticClass(),FTransform(FVector(1100,0,96)));
		Victim->FindComponentByClass<URLHealthComponent>()->InitializeHealth(4);
		Shot=Shots->AcquireProjectile(ARLProjectile::StaticClass(),FTransform(FVector(0,0,80)),Owner,nullptr);
		Shot->InitializeFromDefinition(Definition,nullptr);
		auto* Reflector=World->SpawnActor<ACharacter>(FVector(0,-150,96),FRotator::ZeroRotator);
		FRLProjectileReflectionParams Params; Params.SpeedMultiplier=1;
		const FVector EnemyLocation(1100,0,80);
		TestTrue(TEXT("Player can send a return via mirror"),Shot->Reflect(Reflector,Reflector,(Mirror->GetMirroredTarget(EnemyLocation,Shot->GetDefaultCollisionRadius())-Shot->GetActorLocation()).GetSafeNormal(),Params));
		Movement=Shot->FindComponentByClass<UProjectileMovementComponent>();
		for(int32 Step=0;Step<RouteSteps;++Step) { Movement->TickComponent(0.1f,LEVELTICK_All,nullptr); }
		TestTrue(TEXT("Return arrives in side pocket after a real bounce"),FVector::Dist2D(Shot->GetActorLocation(),EnemyLocation)<120);
		TestTrue(TEXT("Return ownership preserved"),Shot->IsReflected()&&Shot->GetOwner()==Reflector);
		TestTrue(TEXT("Mirror return deals real enemy damage"),Victim->FindComponentByClass<URLHealthComponent>()->GetCurrentHealth()<4);
		Shots->ReleaseProjectile(Shot);
	}
	Arena->ResetArena();
	TestFalse(TEXT("Cleanup removes direct-fire obstruction"),World->LineTraceSingleByChannel(Hit,FVector(1100,0,80),FVector(0,0,80),ECC_Visibility));
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
