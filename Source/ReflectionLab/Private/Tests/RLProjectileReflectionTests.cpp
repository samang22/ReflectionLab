#include "Combat/RLProjectile.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/RLEnemyPoolSubsystem.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/Material.h"
#include "Misc/AutomationTest.h"
#include "Player/Components/RLHealthComponent.h"
#include "Player/RLPlayerCharacter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLProjectileReflectionTest, "ReflectionLab.Combat.SpecialProjectileReflection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLProjectileReflectionTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Settings = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Test world"), World)) { return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	const FURL TestURL(nullptr, TEXT("?game=/Script/Engine.GameModeBase"), TRAVEL_Absolute);
	World->SetGameMode(TestURL);
	World->InitializeActorsForPlay(TestURL);
	World->BeginPlay();
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARLPlayerCharacter* Player = World->SpawnActor<ARLPlayerCharacter>(
		ARLPlayerCharacter::StaticClass(), FVector(-500.0f, 0.0f, 100.0f), FRotator::ZeroRotator, Spawn);
	ARLEnemyCharacter* NearEnemy = World->SpawnActor<ARLEnemyCharacter>(
		ARLEnemyCharacter::StaticClass(), FVector(300.0f, 0.0f, 100.0f), FRotator::ZeroRotator, Spawn);
	ARLEnemyCharacter* FarEnemy = World->SpawnActor<ARLEnemyCharacter>(
		ARLEnemyCharacter::StaticClass(), FVector(0.0f, 600.0f, 100.0f), FRotator::ZeroRotator, Spawn);
	ARLProjectile* Projectile = World->SpawnActor<ARLProjectile>();
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	if (TestNotNull(TEXT("Player"), Player) && TestNotNull(TEXT("Near enemy"), NearEnemy) &&
		TestNotNull(TEXT("Far enemy"), FarEnemy) && TestNotNull(TEXT("Projectile"), Projectile) &&
		TestNotNull(TEXT("Controller"), Controller))
	{
		Controller->Possess(Player);
		NearEnemy->SetTutorialCombatControlled(true);
		FarEnemy->SetTutorialCombatControlled(true);
		URLHealthComponent* EnemyHealth = NearEnemy->FindComponentByClass<URLHealthComponent>();
		EnemyHealth->InitializeHealth(10.0f);
		FarEnemy->FindComponentByClass<URLHealthComponent>()->InitializeHealth(10.0f);
		URLProjectileDefinitionDataAsset* Definition = NewObject<URLProjectileDefinitionDataAsset>();
		Definition->Behavior = ERLProjectileBehavior::Explosive;
		Definition->ExplosiveFuseDuration = 1.0f;
		Definition->ExplosionRadius = 1000.0f;
		Definition->ExplosionDamage = 1.0f;
		Definition->ExplosionSound = nullptr;
		Definition->HostileMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
		Definition->ReflectedMaterial = Definition->HostileMaterial;
		const FTransform Origin(FRotator::ZeroRotator, FVector(0.0f, 0.0f, 100.0f));
		Projectile->ActivateProjectile(Origin, NearEnemy, NearEnemy);
		Projectile->InitializeFromDefinition(Definition, Player);
		UStaticMeshComponent* ProjectileVisual = Projectile->FindComponentByClass<UStaticMeshComponent>();
		const FVector HostileBombScale = ProjectileVisual->GetRelativeScale3D();
		TestTrue(TEXT("Translucent projectile visuals disallow Nanite"), ProjectileVisual->bDisallowNanite != 0);
		TestTrue(TEXT("Explosives can be parried"), Projectile->CanBeReflected());
		Projectile->Tick(0.25f);
		TestTrue(TEXT("Hostile fuse progresses"), FMath::IsNearlyEqual(
			Projectile->GetExplosiveFuseRemainingSeconds(), 0.75f));
		UProjectileMovementComponent* Movement = Projectile->FindComponentByClass<UProjectileMovementComponent>();
		const double HostileBombSpeed = Movement->Velocity.Size();
		FRLProjectileReflectionParams Reflection;
		TestTrue(TEXT("Explosive reflection succeeds"), Projectile->Reflect(Player, Player, FVector::ForwardVector, Reflection));
		TestTrue(TEXT("Reflection preserves explosive behavior"), Projectile->IsExplosive());
		TestTrue(TEXT("Reflection preserves the large bomb silhouette"),
			ProjectileVisual->GetRelativeScale3D().Equals(HostileBombScale));
		TestEqual(TEXT("Parry restarts the full fuse"), Projectile->GetExplosiveFuseRemainingSeconds(), 1.0f);
		TestTrue(TEXT("Bomb return keeps its hostile speed without rewards"),
			FMath::IsNearlyEqual(Movement->Velocity.Size(), HostileBombSpeed));
		USphereComponent* Collision = Projectile->FindComponentByClass<USphereComponent>();
		const float PlayerHealth = Player->GetCurrentHealth();
		Collision->OnComponentBeginOverlap.Broadcast(Collision, Player, nullptr, 0, false, FHitResult());
		TestEqual(TEXT("Reflected bomb ignores player contact"), Projectile->GetExplosiveFuseRemainingSeconds(), 1.0f);
		Collision->OnComponentBeginOverlap.Broadcast(Collision, NearEnemy, nullptr, 0, false, FHitResult());
		TestEqual(TEXT("Enemy contact explodes immediately before the fuse"), EnemyHealth->GetCurrentHealth(), 9.0f);
		TestEqual(TEXT("Friendly explosion never damages player"), Player->GetCurrentHealth(), PlayerHealth);
		TestFalse(TEXT("Exploded bombs cannot be reflected again"), Projectile->CanBeReflected());

		// Tutorial enemies remain invulnerable, but still trigger the explosion.
		NearEnemy->SetTutorialInvulnerable(true);
		FarEnemy->SetTutorialInvulnerable(true);
		Projectile->ActivateProjectile(Origin, NearEnemy, NearEnemy);
		Projectile->InitializeFromDefinition(Definition, Player);
		Reflection.SpeedMultiplier = 1.2f;
		TestTrue(TEXT("Reward-speed bomb reflects"), Projectile->Reflect(Player, Player, FVector::ForwardVector, Reflection));
		TestTrue(TEXT("A 0.2 speed reward adds twenty percent to hostile bomb speed"),
			FMath::IsNearlyEqual(Movement->Velocity.Size(), HostileBombSpeed * 1.2, 0.001));
		Collision->OnComponentBeginOverlap.Broadcast(Collision, NearEnemy, nullptr, 0, false, FHitResult());
		TestFalse(TEXT("Tutorial contact still explodes"), Projectile->CanBeReflected());
		TestEqual(TEXT("Tutorial enemy loses no HP"), EnemyHealth->GetCurrentHealth(), 9.0f);
		NearEnemy->SetTutorialInvulnerable(false);
		FarEnemy->SetTutorialInvulnerable(false);
		Projectile->ActivateProjectile(Origin, NearEnemy, NearEnemy);
		Projectile->InitializeFromDefinition(Definition, Player);
		Reflection.SpeedMultiplier = 0.0f;
		Projectile->Reflect(Player, Player, FVector::ForwardVector, Reflection);
		Projectile->Tick(1.01f);
		TestEqual(TEXT("Fuse still detonates if no enemy is hit"), EnemyHealth->GetCurrentHealth(), 8.0f);

		// Reuse the same actor: no stopped collision, fuse, or target may leak.
		Movement->SetUpdatedComponent(nullptr); // A blocking wall hit clears this in ProjectileMovement.
		Projectile->ActivateProjectile(Origin, NearEnemy, NearEnemy);
		Definition->Behavior = ERLProjectileBehavior::Rally;
		Projectile->InitializeFromDefinition(Definition, Player);
		TestFalse(TEXT("Reuse clears explosive behavior"), Projectile->IsExplosive());
		TestTrue(TEXT("Reuse restores collision"), Collision->IsQueryCollisionEnabled());
		TestTrue(TEXT("Reuse restores the movement target after a wall hit"), Movement->UpdatedComponent == Collision);
		Reflection.PierceCount = 5;
		TestTrue(TEXT("Rally reflection succeeds"), Projectile->Reflect(Player, Player, FVector::BackwardVector, Reflection));
		TestEqual(TEXT("Rally reflection caps piercing upgrades at one"), Projectile->GetRemainingPierces(), 1);
		TestTrue(TEXT("Reflection preserves rally behavior"), Projectile->IsRallyProjectile());
		TestTrue(TEXT("Closest target includes the original shooter"), Movement->Velocity.GetSafeNormal().Equals(FVector::ForwardVector));
		// Test real arrival logic without manually supplying an overlap event.
		Collision->SetGenerateOverlapEvents(false);
		Projectile->SetActorLocation(NearEnemy->GetActorLocation() - FVector(20.0f, 0.0f, 0.0f));
		const float HealthBeforeArrival = EnemyHealth->GetCurrentHealth();
		Projectile->Tick(0.01f);
		TestEqual(TEXT("Rally arrival damages even without an overlap event"),
			EnemyHealth->GetCurrentHealth(), HealthBeforeArrival - Definition->DamageAmount);
		Collision->OnComponentBeginOverlap.Broadcast(Collision, NearEnemy, nullptr, 0, false, FHitResult());
		Collision->OnComponentHit.Broadcast(Collision, NearEnemy, nullptr, FVector::ZeroVector, FHitResult());
		TestEqual(TEXT("Arrival, overlap and hit cannot damage the same enemy twice"),
			EnemyHealth->GetCurrentHealth(), HealthBeforeArrival - Definition->DamageAmount);
		TestTrue(TEXT("Piercing rally survives its first hit"), Projectile->CanBeReflected());
		TestEqual(TEXT("First rally hit consumes the single pierce"), Projectile->GetRemainingPierces(), 0);
		const float FarHealthBeforeHit = FarEnemy->FindComponentByClass<URLHealthComponent>()->GetCurrentHealth();
		Collision->OnComponentBeginOverlap.Broadcast(Collision, FarEnemy, nullptr, 0, false, FHitResult());
		TestEqual(TEXT("Piercing rally damages its second enemy"),
			FarEnemy->FindComponentByClass<URLHealthComponent>()->GetCurrentHealth(), FarHealthBeforeHit - Definition->DamageAmount);
		TestFalse(TEXT("Second rally hit starts fade-out even with five requested pierces"), Projectile->CanBeReflected());
		Collision->OnComponentHit.Broadcast(Collision, FarEnemy, nullptr, FVector::ZeroVector, FHitResult());
		TestEqual(TEXT("Hit events during fade-out cannot apply damage again"),
			FarEnemy->FindComponentByClass<URLHealthComponent>()->GetCurrentHealth(), FarHealthBeforeHit - Definition->DamageAmount);
		Projectile->ActivateProjectile(Origin, NearEnemy, NearEnemy);
		Projectile->InitializeFromDefinition(Definition, Player);
		Projectile->Reflect(Player, Player, FVector::ForwardVector, Reflection);
		Collision->SetGenerateOverlapEvents(true);
		NearEnemy->ReturnToPool();
		Projectile->Tick(0.01f);
		TestTrue(TEXT("Inactive target is replaced by the next enemy"), Movement->Velocity.GetSafeNormal().Equals(FVector::RightVector));
		FarEnemy->ReturnToPool();
		Projectile->Tick(0.01f);
		TestTrue(TEXT("An unhit return with no remaining enemy stays active"), Projectile->CanBeReflected());
		// A fresh reflection with no enemies still keeps its original straight-flight fallback.
		Projectile->ActivateProjectile(Origin, Player, Player);
		Projectile->InitializeFromDefinition(Definition, Player);
		Reflection.PierceCount = 0;
		Projectile->Reflect(Player, Player, FVector::ForwardVector, Reflection);
		const FVector LastVelocity = Movement->Velocity;
		Projectile->Tick(0.01f);
		TestTrue(TEXT("Without any hit or target the return flies straight"), Movement->Velocity.Equals(LastVelocity));
		TestTrue(TEXT("An unhit return with no target remains active"), Projectile->CanBeReflected());
		ARLEnemyCharacter* HitEnemy = World->SpawnActor<ARLEnemyCharacter>(
			ARLEnemyCharacter::StaticClass(), FVector(600.0f, 0.0f, 100.0f), FRotator::ZeroRotator, Spawn);
		if (TestNotNull(TEXT("Rally impact enemy"), HitEnemy))
		{
			HitEnemy->SetTutorialCombatControlled(true);
			URLHealthComponent* HitHealth = HitEnemy->FindComponentByClass<URLHealthComponent>();
			HitHealth->InitializeHealth(10.0f);
			Collision->OnComponentBeginOverlap.Broadcast(Collision, HitEnemy, nullptr, 0, false, FHitResult());
			TestEqual(TEXT("Reflected rally damages enemies rather than relaying"), HitHealth->GetCurrentHealth(), 9.0f);
			TestFalse(TEXT("A non-piercing rally impact starts fade-out"), Projectile->CanBeReflected());
			HitEnemy->SetTutorialInvulnerable(true);
			HitEnemy->ReturnToPool();
			ARLEnemyCharacter* ReusedEnemy = World->GetSubsystem<URLEnemyPoolSubsystem>()->AcquireEnemy(
				ARLEnemyCharacter::StaticClass(), FTransform(FRotator::ZeroRotator, FVector(600.0f, 0.0f, 100.0f)));
			if (TestNotNull(TEXT("Enemy reused after tutorial protection"), ReusedEnemy))
			{
				TestTrue(TEXT("Pool reused the previously invulnerable enemy"), ReusedEnemy == HitEnemy);
				ReusedEnemy->SetTutorialCombatControlled(true);
				TestEqual(TEXT("Tutorial invulnerability does not survive enemy pool reuse"),
					ReusedEnemy->TakeDamage(1.0f, FDamageEvent(), nullptr, nullptr), 1.0f);
				ReusedEnemy->ReturnToPool();
			}
		}

		Projectile->ActivateProjectile(Origin, Player, Player);
		Definition->Behavior = ERLProjectileBehavior::DelayedExplosive;
		Definition->DelayedExplosiveTriggerDistance = 1000.0f;
		Projectile->InitializeFromDefinition(Definition, Player);
		Projectile->Tick(0.01f);
		TestTrue(TEXT("Delayed shot pauses before reflection"), Movement->Velocity.IsNearlyZero());
		TestTrue(TEXT("Delayed explosive can be reflected"), Projectile->Reflect(Player, Player, FVector::ForwardVector, Reflection));
		Projectile->Tick(0.1f);
		TestTrue(TEXT("Reflected delayed bomb no longer resumes toward player"), Movement->Velocity.X > 0.0f);
		Projectile->ReturnToPool();
		TestFalse(TEXT("Fading projectiles cannot be reflected"), Projectile->Reflect(Player, Player, FVector::ForwardVector, Reflection));
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
