#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Framework/RLPoolDiagnosticsSubsystem.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLProjectileRallyComponent.h"
#include "Enemies/RLEnemyPoolSubsystem.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/Components/RLEnemyShieldComponent.h"
#include "Player/RLRollAfterimagePoolSubsystem.h"
#include "Player/RLRollAfterimage.h"
#include "Player/Components/RLHealthComponent.h"
#include "Player/Components/RLRunRewardComponent.h"
#include "Data/RLRunRewardDataAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/Material.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLPoolLifecycleTest,"ReflectionLab.Diagnostics.PoolLifecycleTenCycles",
	EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRLPoolLifecycleTest::RunTest(const FString& Parameters)
{
	const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
	if (!TestNotNull(TEXT("World"),World)) { return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	const FURL URL(nullptr,TEXT("?game=/Script/Engine.GameModeBase"),TRAVEL_Absolute);
	World->SetGameMode(URL); World->InitializeActorsForPlay(URL); World->BeginPlay();
	auto* D=World->GetSubsystem<URLPoolDiagnosticsSubsystem>();
	auto* Shots=World->GetSubsystem<URLProjectilePoolSubsystem>();
	auto* Enemies=World->GetSubsystem<URLEnemyPoolSubsystem>();
	auto* Ghosts=World->GetSubsystem<URLRollAfterimagePoolSubsystem>();
	D->StartMeasurement();
	Shots->PrewarmPool(ARLProjectile::StaticClass(),1);
	Enemies->PrewarmPool(ARLEnemyCharacter::StaticClass(),1);
	auto* Player=World->SpawnActor<ACharacter>(FVector(-2000,0,96),FRotator::ZeroRotator);
	auto* Health=NewObject<URLHealthComponent>(Player); Health->InitializeHealth(10);
	auto* Rewards=NewObject<URLRunRewardComponent>(Player); Rewards->Initialize(Health);
	auto* Reward=NewObject<URLRunRewardDataAsset>(); Reward->RewardType=ERLRunRewardType::Vitality; Reward->Amount=2;
	auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	if (!TestNotNull(TEXT("Afterimage test mesh"),Mesh))
	{
		D->StopMeasurement(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return false;
	}
	Player->GetMesh()->SetSkeletalMeshAsset(Mesh);
	auto* Material=UMaterial::GetDefaultMaterial(MD_Surface);
	Ghosts->PrewarmPool(1,Player->GetMesh(),Material);
	ARLProjectile* FirstShot=nullptr;
	ARLEnemyCharacter* FirstEnemy=nullptr;
	ARLRollAfterimage* FirstGhost=nullptr;
	for (int32 Cycle=0;Cycle<10;++Cycle)
	{
		auto* Enemy=Enemies->AcquireEnemy(ARLEnemyCharacter::StaticClass(),FTransform(FVector(1500,0,96)));
		auto* Shot=Shots->AcquireProjectile(ARLProjectile::StaticClass(),FTransform(FVector(0,0,100)),Enemy,Enemy);
		auto* Ghost=Ghosts->AcquireAfterimage(Player->GetMesh(),Material,FLinearColor(0,1,1),0.2f,0.25f);
		if (!TestNotNull(TEXT("Acquire shot"),Shot)||!TestNotNull(TEXT("Acquire enemy"),Enemy)||!TestNotNull(TEXT("Acquire ghost"),Ghost))
		{
			D->StopMeasurement(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return false;
		}
		if (Cycle==0) { FirstShot=Shot; FirstEnemy=Enemy; FirstGhost=Ghost; }
		TestEqual(TEXT("Reuse same projectile"),Shot,FirstShot);
		TestEqual(TEXT("Reuse same enemy"),Enemy,FirstEnemy);
		TestEqual(TEXT("Reuse same afterimage"),Ghost,FirstGhost);
		TestFalse(TEXT("Projectile is hostile on reuse"),Shot->IsReflected());
		TestFalse(TEXT("Rally damage history cleared"),Shot->GetRallyComponent()->HasDamagedEnemy(Enemy));
		TestNull(TEXT("Rally target cleared"),Shot->GetRallyComponent()->GetTargetActor());
		TestEqual(TEXT("Owner reset to shooter"),Shot->GetOwner(),static_cast<AActor*>(Enemy));
		TestEqual(TEXT("Instigator reset to shooter"),Shot->GetInstigator(),static_cast<APawn*>(Enemy));
		TestFalse(TEXT("Previous mirror bounce state cleared"),Shot->FindComponentByClass<UProjectileMovementComponent>()->bShouldBounce);
		TestTrue(TEXT("Active collision restored"),Shot->GetCollisionSphere()->GetCollisionEnabled()!=ECollisionEnabled::NoCollision);
		TestFalse(TEXT("Previous support shield cleared"),Enemy->FindComponentByClass<URLEnemyShieldComponent>()->IsSupportProtected());
		TestEqual(TEXT("Enemy health restored"),Enemy->FindComponentByClass<URLHealthComponent>()->GetCurrentHealth(),Enemy->FindComponentByClass<URLHealthComponent>()->GetMaxHealth());
		const FGuid Swing=FGuid::NewGuid(); Shot->ExcludeParryAttempt(Swing);
		Shot->GetRallyComponent()->RecordEnemyHit(Enemy);
		Shot->ConfigureAsRally(Enemy,2,1.1f);
		FRLProjectileReflectionParams Reflection;
		TestTrue(TEXT("Reflect before recycling"),Shot->Reflect(Player,Player,FVector::ForwardVector,Reflection));
		Shot->GetRallyComponent()->RecordEnemyHit(Enemy);
		Shot->FindComponentByClass<UProjectileMovementComponent>()->bShouldBounce=true;
		TestTrue(TEXT("Apply reward before reset"),Rewards->TryApplyReward(Reward));
		Health->ApplyDamage(1);
		Rewards->ResetRunRewards(); Health->InitializeHealth(10);
		TestTrue(TEXT("Reward history cleared for next run"),Rewards->GetAcquiredRewards().IsEmpty());
		TestEqual(TEXT("Max health reset"),Health->GetMaxHealth(),10.0f);
		TestEqual(TEXT("Current health reset"),Health->GetCurrentHealth(),10.0f);
		Shots->ReleaseProjectile(Shot); Shots->ReleaseProjectile(Shot);
		Enemies->ReleaseEnemy(Enemy); Enemies->ReleaseEnemy(Enemy);
		Ghost->Tick(0.3f); Ghosts->ReleaseAfterimage(Ghost);
		TestFalse(TEXT("Pool release disables projectile collision"),Shot->GetActorEnableCollision() && Shot->GetCollisionSphere()->GetCollisionEnabled()!=ECollisionEnabled::NoCollision);
		TestFalse(TEXT("Ghost expires into pool"),Ghost->IsPoolActive());
		TestNull(TEXT("Ghost clears owner"),Ghost->GetOwner());
		TestFalse(TEXT("Reactivation clears previous swing exclusion"), [&] {
			auto* Again=Shots->AcquireProjectile(ARLProjectile::StaticClass(),FTransform(FVector(0,0,100)),Enemy,Enemy);
			const bool Excluded=Again->IsExcludedFromParryAttempt(Swing); Shots->ReleaseProjectile(Again); return Excluded;
		}());
	}
	for (FName Pool : {FName(TEXT("Projectile")),FName(TEXT("Enemy")),FName(TEXT("RollAfterimage"))})
	{
		const auto S=D->GetSnapshot(Pool);
		TestEqual(TEXT("One prewarmed allocation"),S.Created,int64(1));
		TestEqual(TEXT("No active actors after cycles"),S.Active,0);
		TestEqual(TEXT("Single inactive actor, duplicate return ignored"),S.Inactive,1);
		TestEqual(TEXT("Peak active"),S.PeakActive,1);
		TestEqual(TEXT("Every acquisition reused prewarmed actor"),S.Acquired,S.Reused);
		TestEqual(TEXT("Acquisitions balance unique returns"),S.Acquired,S.Released);
		TestEqual(TEXT("Exact cycle reuse count"),S.Reused,int64(Pool==TEXT("Projectile") ? 20 : 10));
	}
	D->StopMeasurement();
	D->StartMeasurement();
	TestEqual(TEXT("New window resets counters, not inventory"),D->GetSnapshot(TEXT("Projectile")).Created,int64(0));
	TestEqual(TEXT("New window retains inventory"),D->GetSnapshot(TEXT("Projectile")).Inactive,1);
	TestNull(TEXT("Invalid class does not allocate"),Shots->AcquireProjectile(nullptr,FTransform::Identity,nullptr,nullptr));
	TestEqual(TEXT("Invalid acquisition does not count"),D->GetSnapshot(TEXT("Projectile")).Acquired,int64(0));
	auto* Warm=Shots->AcquireProjectile(ARLProjectile::StaticClass(),FTransform(FVector(0,0,100)),nullptr,nullptr);
	auto* Cold=Shots->AcquireProjectile(ARLProjectile::StaticClass(),FTransform(FVector(1000,0,100)),nullptr,nullptr);
	TestEqual(TEXT("Exhausted pool allocates exactly one new actor"),D->GetSnapshot(TEXT("Projectile")).Created,int64(1));
	TestEqual(TEXT("Cold allocation is not reuse"),D->GetSnapshot(TEXT("Projectile")).Reused,int64(1));
	TestEqual(TEXT("Two simultaneous acquisitions raise peak"),D->GetSnapshot(TEXT("Projectile")).PeakActive,2);
	D->StartMeasurement();
	TestEqual(TEXT("Start during combat preserves active count"),D->GetSnapshot(TEXT("Projectile")).Active,2);
	TestEqual(TEXT("Start during combat seeds peak"),D->GetSnapshot(TEXT("Projectile")).PeakActive,2);
	Shots->ReleaseProjectile(Warm); Shots->ReleaseProjectile(Cold);
	D->StopMeasurement();
	Warm=Shots->AcquireProjectile(ARLProjectile::StaticClass(),FTransform(FVector(0,0,100)),nullptr,nullptr);
	TestEqual(TEXT("Stopped measurement does not count reuse"),D->GetSnapshot(TEXT("Projectile")).Reused,int64(0));
	Shots->ReleaseProjectile(Warm);
	FirstShot->Destroy();
	TestEqual(TEXT("Destroyed actors excluded from inventory"),D->GetSnapshot(TEXT("Projectile")).Inactive,1);
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
	return true;
}
#endif
