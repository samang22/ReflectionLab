#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Combat/RLProjectileEffectPoolSubsystem.h"
#include "Combat/RLExplosionVisual.h"
#include "Framework/RLPoolDiagnosticsSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLProjectileEffectPoolTest, "ReflectionLab.Diagnostics.ProjectileEffectPoolTenCycles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRLProjectileEffectPoolTest::RunTest(const FString& Parameters)
{
	const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* Pool = World->GetSubsystem<URLProjectileEffectPoolSubsystem>();
	auto* Diagnostics = World->GetSubsystem<URLPoolDiagnosticsSubsystem>();
	auto* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	auto* Source = UMaterialInstanceDynamic::Create(UMaterial::GetDefaultMaterial(MD_Surface), World);
	if (!TestNotNull(TEXT("Pool subsystem"), Pool) || !TestNotNull(TEXT("Diagnostics subsystem"), Diagnostics) ||
		!TestNotNull(TEXT("Shard mesh"), Mesh) || !TestNotNull(TEXT("Source material"), Source))
	{
		World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return false;
	}
	Diagnostics->StartMeasurement();
	ARLExplosionVisual* First = nullptr;
	TArray<UStaticMeshComponent*> FirstShards;
	UMaterialInterface* FirstMaterial = nullptr;
	for (int32 Cycle = 0; Cycle < 10; ++Cycle)
	{
		const FVector Location(Cycle * 100.0f, 0, 20);
		auto* Effect = Pool->PlayEffect(Location, Mesh, Source);
		if (!TestNotNull(TEXT("Acquire effect"), Effect)) { break; }
		TArray<UStaticMeshComponent*> Shards;
		Effect->GetComponents(Shards);
		if (Cycle == 0)
		{
			First = Effect; FirstShards = Shards;
			FirstMaterial = Shards.IsEmpty() ? nullptr : Shards[0]->GetMaterial(0);
		}
		TestEqual(TEXT("Same actor reused"), Effect, First);
		TestEqual(TEXT("Location reset"), Effect->GetActorLocation(), Location);
		TestEqual(TEXT("Only six retained shards"), Shards.Num(), 6);
		TestTrue(TEXT("Same shard components retained"), Shards == FirstShards);
		if (!Shards.IsEmpty()) { TestEqual(TEXT("Same dynamic material retained"), Shards[0]->GetMaterial(0), FirstMaterial); }
		TestTrue(TEXT("Tick restored"), Effect->IsActorTickEnabled());
		Effect->Tick(0.1f);
		TestEqual(TEXT("Still active before duration"), Diagnostics->GetSnapshot(TEXT("ProjectileEffect")).Active, 1);
		Effect->Tick(0.21f);
		TestTrue(TEXT("Hidden after expiration"), Effect->IsHidden());
		TestFalse(TEXT("Tick disabled on return"), Effect->IsActorTickEnabled());
		Pool->ReleaseEffect(Effect);
	}
	auto Snapshot = Diagnostics->GetSnapshot(TEXT("ProjectileEffect"));
	TestEqual(TEXT("One actor created"), Snapshot.Created, int64(1));
	TestEqual(TEXT("Nine reuses"), Snapshot.Reused, int64(9));
	TestEqual(TEXT("Ten returns, duplicate ignored"), Snapshot.Released, int64(10));
	TestEqual(TEXT("No active effects"), Snapshot.Active, 0);
	TestEqual(TEXT("One inactive effect"), Snapshot.Inactive, 1);
	Pool->PlayEffect(FVector::ZeroVector, Mesh, Source);
	auto* Additional = Pool->PlayEffect(FVector::ZeroVector, Mesh, Source);
	TestNotNull(TEXT("Exhaustion creates another actor"), Additional);
	TestEqual(TEXT("Exactly two allocations"), Diagnostics->GetSnapshot(TEXT("ProjectileEffect")).Created, int64(2));
	Pool->ReleaseEffect(First); Pool->ReleaseEffect(Additional);
	if (First) { First->Destroy(); }
	auto* Replacement = Pool->PlayEffect(FVector::ZeroVector, Mesh, Source);
	TestTrue(TEXT("Destroyed inactive entry is never reused"), IsValid(Replacement));
	Pool->ReleaseEffect(Replacement);
	TestNull(TEXT("Null mesh is rejected"), Pool->PlayEffect(FVector::ZeroVector, nullptr, Source));
	Diagnostics->StopMeasurement();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
