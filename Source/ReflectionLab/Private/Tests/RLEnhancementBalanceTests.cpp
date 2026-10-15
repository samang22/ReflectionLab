#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Data/RLRunDefinitionDataAsset.h"
#include "Data/RLPlayerStatsDataAsset.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLProjectileContactComponent.h"
#include "Combat/RLProjectileSpecialComponent.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/RLEnemyPoolSubsystem.h"
#include "Player/Components/RLHealthComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLEnhancementBalanceTest, "ReflectionLab.Combat.EnhancementBalance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLEnhancementBalanceTest::RunTest(const FString& Parameters)
{
	FRLEnemyHealthMix Mix;
	Mix.HeavyRatio = 0.25f;
	Mix.ReinforcedRatio = 0.5f;
	int32 Normal = 0, Reinforced = 0, Heavy = 0;
	for (int32 Index = 0; Index < 8; ++Index)
	{
		const float Health = Mix.GetHealthForSpawn(Index, 8);
		Normal += Health == 2.0f;
		Reinforced += Health == 4.0f;
		Heavy += Health == 5.0f;
	}
	TestEqual(TEXT("Normal proportion"), Normal, 2);
	TestEqual(TEXT("Reinforced proportion"), Reinforced, 4);
	TestEqual(TEXT("Heavy proportion"), Heavy, 2);
	URLRunDefinitionDataAsset* Run = NewObject<URLRunDefinitionDataAsset>();
	Run->RoundEnemyHealthMixes.Add(10, Mix);
	TestNull(TEXT("No premature health profile"), Run->GetEnemyHealthMix(1));
	TestNotNull(TEXT("Endless rounds retain final health mix"), Run->GetEnemyHealthMix(11));
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false)
		.ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	const FURL TestURL(nullptr, TEXT("?game=/Script/Engine.GameModeBase"), TRAVEL_Absolute);
	World->SetGameMode(TestURL);
	World->InitializeActorsForPlay(TestURL);
	World->BeginPlay();
	ACharacter* Owner = World->SpawnActor<ACharacter>();
	ARLProjectile* Projectile = World->SpawnActor<ARLProjectile>();
	ARLEnemyCharacter* Enemy = World->GetSubsystem<URLEnemyPoolSubsystem>()->AcquireEnemy(
		ARLEnemyCharacter::StaticClass(), FTransform(FVector(1000.0f, 0.0f, 100.0f)));
	if (Owner && Projectile && Enemy)
	{
		Enemy->SetTutorialCombatControlled(true);
		URLHealthComponent* Health = Enemy->FindComponentByClass<URLHealthComponent>();
		URLProjectileDefinitionDataAsset* Definition = NewObject<URLProjectileDefinitionDataAsset>();
		Definition->DamageAmount = 1.0f;
		FRLProjectileReflectionParams Reflection;
		for (const float Multiplier : {1.0f, 2.0f, 3.0f, 5.0f})
		{
			Projectile->ActivateProjectile(FTransform(FVector(0, 0, 100)), Owner, Owner);
			Projectile->InitializeFromDefinition(Definition, Owner);
			TestEqual(TEXT("Pool activation resets multiplier"), Projectile->GetReflectionDamageMultiplier(), 1.0f);
			Reflection.DamageMultiplier = Multiplier;
			TestTrue(TEXT("Reflect"), Projectile->Reflect(Owner, Owner, FVector::ForwardVector, Reflection));
			TestEqual(TEXT("Stage contact damage"), Projectile->GetDamageAmount(), Multiplier);
			Health->InitializeHealth(20.0f);
			Projectile->GetContactComponent()->ResolveProjectileContact(Enemy);
			TestEqual(TEXT("Enemy receives stage damage"), Health->GetCurrentHealth(), 20.0f - Multiplier);
		}
		Definition->Behavior = ERLProjectileBehavior::Explosive;
		Definition->ExplosionDamage = 1.0f;
		Definition->ReflectedExplosionRadius = 300.0f;
		Projectile->ActivateProjectile(FTransform(FVector(800, 0, 100)), Owner, Owner);
		Projectile->InitializeFromDefinition(Definition, Owner);
		Reflection.DamageMultiplier = 5.0f;
		Projectile->Reflect(Owner, Owner, FVector::ForwardVector, Reflection);
		Health->InitializeHealth(20.0f);
		Projectile->GetContactComponent()->ResolveProjectileContact(Enemy);
		TestEqual(TEXT("Explosion scaled once without additional direct hit"), Health->GetCurrentHealth(), 15.0f);
	}
	else { AddError(TEXT("Could not spawn test actors")); }
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
