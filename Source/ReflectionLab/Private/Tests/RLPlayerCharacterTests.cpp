#include "Player/RLPlayerCharacter.h"
#include "Player/Components/RLHealthComponent.h"
#include "Player/Components/RLDodgeRollComponent.h"
#include "Player/Components/RLParryComponent.h"
#include "Data/RLPlayerStatsDataAsset.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/DamageEvents.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Components/BoxComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLPlayerRollTest, "ReflectionLab.Player.Roll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLPlayerHealthTest, "ReflectionLab.Player.HealthIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLPlayerHealthTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Settings = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	const FURL TestURL(nullptr, TEXT("?game=/Script/Engine.GameModeBase"), TRAVEL_Absolute);
	World->SetGameMode(TestURL);
	World->InitializeActorsForPlay(TestURL);
	World->BeginPlay();
	ARLPlayerCharacter* Character = World->SpawnActor<ARLPlayerCharacter>();
	if (TestNotNull(TEXT("Player"), Character))
	{
		TestEqual(TEXT("BeginPlay initializes HP"), Character->GetCurrentHealth(), Character->GetMaxHealth());
		Character->TakeDamage(2.0f, FDamageEvent(), nullptr, nullptr);
		TestEqual(TEXT("Damage forwarded"), Character->GetCurrentHealth(), Character->GetMaxHealth() - 2.0f);
		TestTrue(TEXT("Damage starts hit recovery component"), Character->IsInHitRecovery());
		TestEqual(TEXT("Hit recovery blocks damage"), Character->TakeDamage(1.0f, FDamageEvent(), nullptr, nullptr), 0.0f);
		Character->RestoreHealth(1.0f);
		TestEqual(TEXT("Recovery forwarded"), Character->GetCurrentHealth(), Character->GetMaxHealth() - 1.0f);
		Character->GetHealthComponent()->ApplyDamage(Character->GetMaxHealth());
		TestFalse(TEXT("Death ends hit recovery"), Character->IsInHitRecovery());
		TestTrue(TEXT("Death disables collision through event"), !Character->GetActorEnableCollision());
		TestTrue(TEXT("Death disables movement through event"), Character->GetCharacterMovement()->MovementMode == MOVE_None);
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

bool FRLPlayerRollTest::RunTest(const FString& Parameters)
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
	UClass* Class = LoadClass<ARLPlayerCharacter>(nullptr,
		TEXT("/Game/ReflectionLab/Gameplay/Player/BP_RLPlayerCharacter.BP_RLPlayerCharacter_C"));
	ARLPlayerCharacter* Character = Class ? World->SpawnActor<ARLPlayerCharacter>(Class) : nullptr;
	if (TestNotNull(TEXT("Player blueprint"), Character))
	{
		APlayerController* Controller = World->SpawnActor<APlayerController>();
		if (!TestNotNull(TEXT("Roll player controller"), Controller))
		{
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
			return false;
		}
		Controller->Possess(Character);
		// A commandlet has no LocalPlayer. Detach its remote-style controller so
		// CharacterMovement can simulate this pawn using the no-controller path.
		Controller->UnPossess();
		Character->GetCharacterMovement()->bRunPhysicsWithNoController = true;
		TestTrue(TEXT("Player collision enabled"), Character->GetActorEnableCollision());
		TestTrue(TEXT("Capsule query collision enabled"), Character->GetCapsuleComponent()->IsQueryCollisionEnabled());
		// A real floor is required: the player intentionally cannot walk off ledges.
		AActor* Floor = World->SpawnActor<AActor>();
		UBoxComponent* FloorCollision = NewObject<UBoxComponent>(Floor);
		Floor->SetRootComponent(FloorCollision);
		FloorCollision->SetBoxExtent(FVector(10000.0f, 10000.0f, 50.0f));
		FloorCollision->SetCollisionProfileName(TEXT("BlockAll"));
		FloorCollision->SetWorldLocation(FVector(0.0f, 0.0f,
			-Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 50.0f));
		FloorCollision->RegisterComponent();
		auto AdvanceWorld = [World](int32 FrameCount)
		{
			for (int32 Frame = 0; Frame < FrameCount; ++Frame)
			{
				++GFrameCounter;
				World->Tick(LEVELTICK_All, 0.05f);
			}
		};
		Character->GetHealthComponent()->InitializeHealth(Character->GetMaxHealth());
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		URLDodgeRollComponent* RollComponent = Character->GetDodgeRollComponent();
		URLPlayerStatsDataAsset* ConfiguredTuning = Character->GetPlayerStatsData();
		TestNotNull(TEXT("Player blueprint assigns player stats asset"), ConfiguredTuning);
		TestTrue(TEXT("Roll component uses the same player stats"), RollComponent->PlayerStatsData == ConfiguredTuning);
		// Use an isolated configuration so balancing edits do not affect test timing.
		URLPlayerStatsDataAsset* TestTuning = NewObject<URLPlayerStatsDataAsset>(Character);
		UAnimMontage* ConfiguredMontage = RollComponent->RollMontage;
		RollComponent->SetPlayerStats(nullptr);
		Character->StartRoll(FVector::RightVector);
		TestFalse(TEXT("Missing roll tuning rejected"), Character->IsRolling());
		RollComponent->SetPlayerStats(TestTuning);
		TestTuning->RollPlayRate = 0.0f;
		Character->StartRoll(FVector::RightVector);
		TestFalse(TEXT("Invalid roll play rate rejected"), Character->IsRolling());
		TestTuning->RollPlayRate = 1.0f;
		TestTuning->RollCooldown = -1.0f;
		Character->StartRoll(FVector::RightVector);
		TestFalse(TEXT("Negative roll cooldown rejected"), Character->IsRolling());
		TestTuning->RollCooldown = 1.0f;
		Character->StartRoll(FVector::ZeroVector);
		TestFalse(TEXT("Zero direction rejected"), Character->IsRolling());
		Character->StartParry();
		TestTrue(TEXT("Parry attempt starts"), Character->GetParryComponent()->IsAttemptInProgress());
		Character->StartRoll(FVector::RightVector);
		TestFalse(TEXT("Parry blocks roll"), Character->IsRolling());
		Character->EndParryWindow();
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		Character->StartRoll(FVector::RightVector);
		TestFalse(TEXT("Airborne roll rejected"), Character->IsRolling());
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		const FVector Start = Character->GetActorLocation();
		Character->StartRoll(FVector::RightVector);
		TestTrue(TEXT("Roll started"), Character->IsRolling());
		TestEqual(TEXT("Rolling blocks damage"), Character->TakeDamage(1.0f, FDamageEvent(), nullptr, nullptr), 0.0f);
		Character->StartParry();
		TestFalse(TEXT("Rolling blocks parry"), Character->GetParryComponent()->IsAttemptInProgress());
		// A running roll must keep its own montage and cooldown if tuning is edited.
		RollComponent->RollMontage = nullptr;
		TestTuning->RollCooldown = 0.0f;
		AdvanceWorld(3);
		AddInfo(FString::Printf(TEXT("Root-motion roll delta: %s, mesh rotation: %s, actor rotation: %s"),
			*(Character->GetActorLocation() - Start).ToString(),
			*Character->GetMesh()->GetRelativeRotation().ToString(), *Character->GetActorRotation().ToString()));
		TestTrue(TEXT("Roll moves in requested direction"),
			Character->GetActorLocation().Y > Start.Y && FMath::IsNearlyEqual(Character->GetActorLocation().X, Start.X, 0.1f));
		const UAnimMontage* ActiveMontage = Character->GetMesh()->GetAnimInstance()->GetCurrentActiveMontage();
		TestTrue(TEXT("Roll montage extracts root motion"), ActiveMontage && ActiveMontage->HasRootMotion());
		AdvanceWorld(30);
		TestFalse(TEXT("Roll ends"), Character->IsRolling());
		TestTrue(TEXT("Movement restored"), Character->GetCharacterMovement()->MovementMode != MOVE_None);
		RollComponent->RollMontage = ConfiguredMontage;
		TestTuning->RollCooldown = 1.0f;
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		Character->StartRoll(FVector::ForwardVector);
		TestFalse(TEXT("Cooldown blocks another roll"), Character->IsRolling());
		AdvanceWorld(22);
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		const FVector WallStart = Character->GetActorLocation();
		AActor* Wall = World->SpawnActor<AActor>();
		UBoxComponent* WallCollision = NewObject<UBoxComponent>(Wall);
		Wall->SetRootComponent(WallCollision);
		WallCollision->SetBoxExtent(FVector(1000.0f, 20.0f, 300.0f));
		WallCollision->SetCollisionProfileName(TEXT("BlockAll"));
		WallCollision->SetWorldLocation(WallStart + FVector(0.0f, 150.0f, 0.0f));
		WallCollision->RegisterComponent();
		Character->StartRoll(FVector::RightVector);
		TestTrue(TEXT("Roll available after cooldown"), Character->IsRolling());
		AdvanceWorld(30);
		TestTrue(TEXT("Root motion respects wall collision"),
			Character->GetActorLocation().Y > WallStart.Y &&
			Character->GetActorLocation().Y <= WallStart.Y + 130.0f - Character->GetCapsuleComponent()->GetScaledCapsuleRadius() + 1.0f);
		TestFalse(TEXT("Blocked roll still ends"), Character->IsRolling());
		Wall->Destroy();
		AdvanceWorld(22);
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		Character->StartRoll(FVector::RightVector);
		TestTrue(TEXT("Roll before interruption"), Character->IsRolling());
		Character->StopAnimMontage();
		AdvanceWorld(1);
		TestFalse(TEXT("Montage interruption ends roll"), Character->IsRolling());
		TestTrue(TEXT("Interruption restores movement"), Character->GetCharacterMovement()->MovementMode != MOVE_None);
		AdvanceWorld(30);
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		Character->StartRoll(FVector::RightVector);
		TestTrue(TEXT("Roll before death"), Character->IsRolling());
		Character->GetHealthComponent()->ApplyDamage(Character->GetMaxHealth());
		TestFalse(TEXT("Death cancels roll"), Character->IsRolling());
		TestTrue(TEXT("Death keeps movement disabled"), Character->GetCharacterMovement()->MovementMode == MOVE_None);
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
