// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/RLPlayerCharacter.h"
#include "Player/Components/RLHealthComponent.h"
#include "Player/Components/RLDodgeRollComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"

#include "Camera/CameraComponent.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Data/RLPlayerStatsDataAsset.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Framework/GameMode/RLGameModeBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

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
		TestEqual(TEXT("Hit recovery blocks damage"), Character->TakeDamage(1.0f, FDamageEvent(), nullptr, nullptr), 0.0f);
		Character->RestoreHealth(1.0f);
		TestEqual(TEXT("Recovery forwarded"), Character->GetCurrentHealth(), Character->GetMaxHealth() - 1.0f);
		Character->GetHealthComponent()->ApplyDamage(Character->GetMaxHealth());
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
		URLPlayerStatsDataAsset* ConfiguredTuning = Character->PlayerStatsData;
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
		Character->bParryAttemptInProgress = true;
		Character->StartRoll(FVector::RightVector);
		TestFalse(TEXT("Parry blocks roll"), Character->IsRolling());
		Character->bParryAttemptInProgress = false;
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		Character->StartRoll(FVector::RightVector);
		TestFalse(TEXT("Airborne roll rejected"), Character->IsRolling());
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		const FVector Start = Character->GetActorLocation();
		Character->StartRoll(FVector::RightVector);
		TestTrue(TEXT("Roll started"), Character->IsRolling());
		TestEqual(TEXT("Rolling blocks damage"), Character->TakeDamage(1.0f, FDamageEvent(), nullptr, nullptr), 0.0f);
		Character->StartParry();
		TestFalse(TEXT("Rolling blocks parry"), Character->bParryAttemptInProgress);
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

// Sets default values
ARLPlayerCharacter::ARLPlayerCharacter()
{
	HealthComponent = CreateDefaultSubobject<URLHealthComponent>(TEXT("HealthComponent"));
	DodgeRollComponent = CreateDefaultSubobject<URLDodgeRollComponent>(TEXT("DodgeRollComponent"));
	static ConstructorHelpers::FObjectFinder<USoundBase> ParryImpactSoundFinder(
		TEXT("/Game/ReflectionLab/Audio/SFX/Combat/Parry/SC_ParryImpact.SC_ParryImpact"));
	ParryImpactSound = ParryImpactSoundFinder.Object;
	static ConstructorHelpers::FObjectFinder<USoundBase> HitSoundFinder(
		TEXT("/Game/ReflectionLab/Audio/SFX/Combat/PlayerHit/SFX_PlayerHit.SFX_PlayerHit"));
	HitSound = HitSoundFinder.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ParryIndicatorMaterialFinder(
		TEXT("/Game/ReflectionLab/Art/Materials/M_ParryRangeIndicator.M_ParryRangeIndicator"));
	ParryRangeIndicatorMaterial = ParryIndicatorMaterialFinder.Object;

	// Set this character to call Tick() every frame. You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	GetMesh()->SetReceivesDecals(false);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 1950.0f;
	CameraBoom->SetRelativeRotation(FRotator(-75.0f, 45.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;
	TopDownCamera->FieldOfView = 60.0f;

	ReflectionZone = CreateDefaultSubobject<USphereComponent>(TEXT("ReflectionZone"));
	ReflectionZone->SetupAttachment(RootComponent);
	ReflectionZone->InitSphereRadius(ReflectionRange);
	ReflectionZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ReflectionZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	ReflectionZone->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	ReflectionZone->SetCanEverAffectNavigation(false);

	ParryRangeIndicator = CreateDefaultSubobject<UDecalComponent>(TEXT("ParryRangeIndicator"));
	ParryRangeIndicator->SetupAttachment(RootComponent);
	// The decal material's vertical UV axis maps to the component's local right
	// axis after pitching it toward the floor, so rotate it back onto character forward.
	ParryRangeIndicator->SetRelativeRotation(FRotator(-90.0f, -90.0f, 0.0f));
	ParryRangeIndicator->DecalSize = FVector(64.0f, ReflectionRange, ReflectionRange);
	ParryRangeIndicator->SetSortOrder(5);
	ParryRangeIndicator->FadeScreenSize = 0.0f;
	ParryRangeIndicator->SetDecalMaterial(ParryRangeIndicatorMaterial);

	OverdriveAuraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("OverdriveAuraComponent"));
	OverdriveAuraComponent->SetupAttachment(RootComponent);
	OverdriveAuraComponent->SetAutoActivate(false);
	OverdriveAuraComponent->SetAutoDestroy(false);
}

// Called when the game starts or when spawned
void ARLPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent->OnHealthChanged.AddUniqueDynamic(this, &ThisClass::HandleHealthChanged);
	HealthComponent->OnDeath.AddUniqueDynamic(this, &ThisClass::HandleDeath);

	ApplyPlayerStats();
	ApplyParryTuning();
	HealthComponent->InitializeHealth(GetMaxHealth());
	ReflectionZone->SetSphereRadius(ReflectionRange);
	if (ParryRangeIndicator && ParryRangeIndicatorMaterial)
	{
		ParryRangeIndicator->SetDecalMaterial(ParryRangeIndicatorMaterial);
		ParryRangeIndicatorMaterialInstance = ParryRangeIndicator->CreateDynamicMaterialInstance();
	}
	UpdateParryRangeIndicator();
	UpdateOverdriveAura();
}

void ARLPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestoreTimeDilation();

	Super::EndPlay(EndPlayReason);
}

void ARLPlayerCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyPlayerStats();
	ApplyParryTuning();
	UpdateParryRangeIndicator();
	UpdateOverdriveAura();
}

void ARLPlayerCharacter::ApplyPlayerStats()
{
	DodgeRollComponent->SetPlayerStats(PlayerStatsData);
	if (!PlayerStatsData)
	{
		return;
	}

	HealthComponent->SetMaxHealth(FMath::Max(1.0f, PlayerStatsData->MaxHealth) + RunRewardMaxHealthBonus);
	HitRecoveryDuration = FMath::Max(0.0f, PlayerStatsData->HitRecoveryDuration);
	HitRecoveryMovementSpeedMultiplier = FMath::Clamp(
		PlayerStatsData->HitRecoveryMovementSpeedMultiplier,
		0.0f,
		1.0f);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = FMath::Max(0.0f, PlayerStatsData->MaxWalkSpeed);
	}
}

void ARLPlayerCharacter::ApplyParryTuning()
{
	if (!PlayerStatsData)
	{
		return;
	}

	ReflectionCooldown = FMath::Max(0.0f, PlayerStatsData->FailedParryCooldown);
	SuccessfulParryCooldown = FMath::Max(0.0f, PlayerStatsData->SuccessfulParryCooldown);
	PerfectSplitProjectileCount = FMath::Clamp(
		PlayerStatsData->PerfectSplitProjectileCount,
		1,
		8);
	PerfectSplitAngleDegrees = FMath::Clamp(
		PlayerStatsData->PerfectSplitAngleDegrees,
		0.0f,
		90.0f);
	PerfectHitStopDurationMultiplier = FMath::Max(
		1.0f,
		PlayerStatsData->PerfectHitStopDurationMultiplier);
	BasePierceCount = FMath::Max(0, PlayerStatsData->BasePierceCount);
	MaxReflectedSpeedMultiplier = FMath::Max(
		1.0f,
		PlayerStatsData->MaxReflectedSpeedMultiplier);
	BaseReflectedProjectileScale = FMath::Max(
		1.0f,
		PlayerStatsData->BaseReflectedProjectileScale);
	CloseRangeThreshold = FMath::Max(0.0f, PlayerStatsData->CloseRangeThreshold);
	CloseRangePierceCount = FMath::Max(0, PlayerStatsData->CloseRangePierceCount);
	CloseRangeProjectileScale = FMath::Max(1.0f, PlayerStatsData->CloseRangeProjectileScale);
	ComboSpeedMilestone = FMath::Max(1, PlayerStatsData->ComboSpeedMilestone);
	ComboExtraProjectileMilestone = FMath::Max(
		ComboSpeedMilestone,
		PlayerStatsData->ComboExtraProjectileMilestone);
	EnhancementStage2Combo = FMath::Max(1, PlayerStatsData->EnhancementStage2Combo);
	EnhancementStage3Combo = FMath::Max(
		EnhancementStage2Combo,
		PlayerStatsData->EnhancementStage3Combo);
	EnhancementStage4Combo = FMath::Max(
		EnhancementStage3Combo,
		PlayerStatsData->EnhancementStage4Combo);
	ComboExtraProjectileSpreadAngle = FMath::Clamp(
		PlayerStatsData->ComboExtraProjectileSpreadAngle,
		0.0f,
		90.0f);
	OverdriveComboThreshold = FMath::Max(
		ComboExtraProjectileMilestone,
		PlayerStatsData->OverdriveComboThreshold);
	OverdriveProjectileCount = FMath::Clamp(PlayerStatsData->OverdriveProjectileCount, 1, 5);
	OverdriveSpreadAngleDegrees = FMath::Clamp(
		PlayerStatsData->OverdriveSpreadAngleDegrees,
		0.0f,
		180.0f);
	OverdriveProjectileScale = FMath::Max(1.0f, PlayerStatsData->OverdriveProjectileScale);
	OverdrivePierceCount = FMath::Max(0, PlayerStatsData->OverdrivePierceCount);
	OverdriveHitStopDurationMultiplier = FMath::Max(
		1.0f,
		PlayerStatsData->OverdriveHitStopDurationMultiplier);
	ReflectionRange = FMath::Max(1.0f, PlayerStatsData->ParryRange);
	PerfectParryOuterBandWidth = FMath::Clamp(
		PlayerStatsData->PerfectParryOuterBandWidth,
		0.0f,
		ReflectionRange);
	ReflectionHalfAngleDegrees = FMath::Clamp(
		PlayerStatsData->ParryHalfAngleDegrees,
		0.0f,
		180.0f);
	ParryIndicatorIdleOpacity = FMath::Clamp(PlayerStatsData->IndicatorIdleOpacity, 0.0f, 1.0f);
	ParryIndicatorActiveOpacity = FMath::Clamp(PlayerStatsData->IndicatorActiveOpacity, 0.0f, 1.0f);
	ParryIndicatorSuccessOpacity = FMath::Clamp(PlayerStatsData->IndicatorSuccessOpacity, 0.0f, 1.0f);
	ParryIndicatorUnavailableOpacity = FMath::Clamp(
		PlayerStatsData->IndicatorUnavailableOpacity,
		0.0f,
		1.0f);
	ParryImpactSoundVolume = FMath::Clamp(PlayerStatsData->ImpactSoundVolume, 0.0f, 1.0f);
	ParrySwingSound = PlayerStatsData->SwingSound;
	ParrySwingSoundVolume = FMath::Clamp(PlayerStatsData->SwingSoundVolume, 0.0f, 1.0f);
	ParryComboImpactSounds = PlayerStatsData->ComboImpactSounds;
	ParryComboImpactSoundVolumes.Reset(PlayerStatsData->ComboImpactSoundVolumes.Num());
	for (const float Volume : PlayerStatsData->ComboImpactSoundVolumes)
	{
		ParryComboImpactSoundVolumes.Add(FMath::Clamp(Volume, 0.0f, 2.0f));
	}
	ParryHitStopDuration = FMath::Max(0.0f, PlayerStatsData->HitStopDuration);
	ParryHitStopTimeDilation = FMath::Clamp(PlayerStatsData->HitStopTimeDilation, 0.01f, 1.0f);
	if (OverdriveAuraComponent)
	{
		OverdriveAuraComponent->SetAsset(PlayerStatsData->OverdriveAuraVFX);
	}
	OverdriveAuraBaseScale = FMath::Clamp(PlayerStatsData->OverdriveAuraScale, 0.1f, 5.0f);
	EnhancementAuraStage2ScaleMultiplier = FMath::Clamp(
		PlayerStatsData->EnhancementAuraStage2ScaleMultiplier,
		0.1f,
		4.0f);
	EnhancementAuraStage3ScaleMultiplier = FMath::Clamp(
		PlayerStatsData->EnhancementAuraStage3ScaleMultiplier,
		EnhancementAuraStage2ScaleMultiplier,
		4.0f);
	EnhancementAuraStage4ScaleMultiplier = FMath::Clamp(
		PlayerStatsData->EnhancementAuraStage4ScaleMultiplier,
		EnhancementAuraStage3ScaleMultiplier,
		4.0f);
	ApplyRunRewardModifiers();
}

void ARLPlayerCharacter::RestoreHealth(float Amount)
{
	HealthComponent->RestoreHealth(Amount);
}

float ARLPlayerCharacter::GetCurrentHealth() const
{
	return HealthComponent->GetCurrentHealth();
}

float ARLPlayerCharacter::GetMaxHealth() const
{
	return HealthComponent->GetMaxHealth();
}

bool ARLPlayerCharacter::IsDead() const
{
	return HealthComponent->IsDead();
}

void ARLPlayerCharacter::HandleHealthChanged(float CurrentHealth, float MaxHealth)
{
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void ARLPlayerCharacter::HandleDeath()
{
	Die();
}

void ARLPlayerCharacter::ApplyRunReward(ERLRunRewardType RewardType)
{
	switch (RewardType)
	{
	case ERLRunRewardType::WiderArc:
		RunRewardArcBonusDegrees += 12.0f;
		break;
	case ERLRunRewardType::ExtendedRange:
		RunRewardRangeMultiplier += 0.18f;
		break;
	case ERLRunRewardType::PiercingReturn:
		++RunRewardPierceBonus;
		break;
	case ERLRunRewardType::PerfectFocus:
		++RunRewardPerfectSplitBonus;
		break;
	case ERLRunRewardType::VelocityDrive:
		RunRewardReflectedSpeedBonus += 0.2f;
		break;
	case ERLRunRewardType::CloseCall:
		RunRewardCloseRangeBonus += 12.0f;
		break;
	case ERLRunRewardType::Vitality:
		RunRewardMaxHealthBonus += 2.0f;
		HealthComponent->SetMaxHealth(GetMaxHealth() + 2.0f, true);
		break;
	case ERLRunRewardType::PerfectRecovery:
		++RunRewardPerfectRecoveryAmount;
		break;
	default:
		return;
	}

	ApplyParryTuning();
	if (ReflectionZone)
	{
		ReflectionZone->SetSphereRadius(ReflectionRange);
	}
	UpdateParryRangeIndicator();
}

void ARLPlayerCharacter::ResetRunRewards()
{
	RunRewardRangeMultiplier = 1.0f;
	RunRewardArcBonusDegrees = 0.0f;
	RunRewardPierceBonus = 0;
	RunRewardPerfectSplitBonus = 0;
	RunRewardReflectedSpeedBonus = 0.0f;
	RunRewardCloseRangeBonus = 0.0f;
	HealthComponent->SetMaxHealth(GetMaxHealth() - RunRewardMaxHealthBonus);
	RunRewardMaxHealthBonus = 0.0f;
	RunRewardPerfectRecoveryAmount = 0;
	ApplyParryTuning();
	if (ReflectionZone)
	{
		ReflectionZone->SetSphereRadius(ReflectionRange);
	}
	UpdateParryRangeIndicator();
}

void ARLPlayerCharacter::ApplyRunRewardModifiers()
{
	ReflectionRange = FMath::Max(1.0f, ReflectionRange * RunRewardRangeMultiplier);
	ReflectionHalfAngleDegrees = FMath::Clamp(
		ReflectionHalfAngleDegrees + RunRewardArcBonusDegrees,
		0.0f,
		180.0f);
	BasePierceCount = FMath::Max(0, BasePierceCount + RunRewardPierceBonus);
	PerfectSplitProjectileCount = FMath::Clamp(
		PerfectSplitProjectileCount + RunRewardPerfectSplitBonus,
		1,
		8);
	MaxReflectedSpeedMultiplier = FMath::Max(
		1.0f,
		MaxReflectedSpeedMultiplier + RunRewardReflectedSpeedBonus);
	CloseRangeThreshold = FMath::Max(0.0f, CloseRangeThreshold + RunRewardCloseRangeBonus);
}

void ARLPlayerCharacter::UpdateOverdriveAura()
{
	if (!OverdriveAuraComponent)
	{
		return;
	}

	float AuraScaleMultiplier = 0.0f;
	switch (ParryEnhancementLevel)
	{
	case 4:
		AuraScaleMultiplier = EnhancementAuraStage4ScaleMultiplier;
		break;
	case 3:
		AuraScaleMultiplier = EnhancementAuraStage3ScaleMultiplier;
		break;
	case 2:
		AuraScaleMultiplier = EnhancementAuraStage2ScaleMultiplier;
		break;
	default:
		break;
	}

	if (AuraScaleMultiplier > 0.0f && OverdriveAuraComponent->GetAsset())
	{
		const float AuraScale = OverdriveAuraBaseScale * AuraScaleMultiplier;
		OverdriveAuraComponent->SetRelativeScale3D(FVector(AuraScale));
		OverdriveAuraComponent->Activate(true);
	}
	else
	{
		OverdriveAuraComponent->Deactivate();
	}
}

void ARLPlayerCharacter::UpdateParryRangeIndicator()
{
	if (!ParryRangeIndicator)
	{
		return;
	}

	ParryRangeIndicator->SetVisibility(bShowParryRangeIndicator, true);
	ParryRangeIndicator->DecalSize = FVector(64.0f, ReflectionRange, ReflectionRange);
	ParryRangeIndicator->SetRelativeLocation(FVector(
		0.0f,
		0.0f,
		-GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + 30.0f));
	UMaterialInterface* const IndicatorMaterial = ParryRangeIndicatorMaterialInstance
		? static_cast<UMaterialInterface*>(ParryRangeIndicatorMaterialInstance)
		: ParryRangeIndicatorMaterial.Get();
	if (ParryRangeIndicator->GetDecalMaterial() != IndicatorMaterial)
	{
		ParryRangeIndicator->SetDecalMaterial(IndicatorMaterial);
	}

	if (ParryRangeIndicatorMaterialInstance)
	{
		// The decal material expects a tangent slope. Its masked UV fan starts
		// clipping at very steep slopes, so keep the visual value in its valid
		// range while leaving the gameplay parry angle unrestricted.
		constexpr float MaxIndicatorHalfAngleDegrees = 58.0f;
		const float VisualHalfAngleDegrees = FMath::Clamp(
			ReflectionHalfAngleDegrees,
			1.0f,
			MaxIndicatorHalfAngleDegrees);
		const float ConeSlope = FMath::Tan(
			FMath::DegreesToRadians(VisualHalfAngleDegrees));
		ParryRangeIndicatorMaterialInstance->SetScalarParameterValue(
			TEXT("ConeSlope"),
			ConeSlope);
		const float PerfectBandInnerRadiusUv = 0.5f *
			(1.0f - PerfectParryOuterBandWidth / ReflectionRange);
		ParryRangeIndicatorMaterialInstance->SetScalarParameterValue(
			TEXT("PerfectBandInnerRadiusUV"),
			PerfectBandInnerRadiusUv);
		UpdateParryIndicatorColor();
		ParryRangeIndicatorMaterialInstance->SetScalarParameterValue(
			TEXT("CloseRadiusUV"), 0.5f * FMath::Clamp(
				CloseRangeThreshold / FMath::Max(1.0f, ReflectionRange), 0.0f, 1.0f));
	}
}

void ARLPlayerCharacter::UpdateParryIndicatorColor()
{
	if (!ParryRangeIndicatorMaterialInstance)
	{
		return;
	}

	const FLinearColor IndicatorColor = bHitRecoveryActive
		? ParryCooldownIndicatorColor
		: (bShowingParrySuccessIndicator
			? ParrySuccessIndicatorColor
			: (bParryOnCooldown ? ParryCooldownIndicatorColor : ParryAvailableIndicatorColor));
	const FLinearColor PerfectIndicatorColor = bHitRecoveryActive
		? PerfectParryCooldownIndicatorColor
		: (bShowingParrySuccessIndicator
			? PerfectParrySuccessIndicatorColor
			: (bParryOnCooldown
				? PerfectParryCooldownIndicatorColor
				: PerfectParryAvailableIndicatorColor));
	ParryRangeIndicatorMaterialInstance->SetVectorParameterValue(
		TEXT("IndicatorColor"),
		IndicatorColor);
	ParryRangeIndicatorMaterialInstance->SetVectorParameterValue(
		TEXT("PerfectIndicatorColor"),
		PerfectIndicatorColor);
	ParryRangeIndicatorMaterialInstance->SetVectorParameterValue(
		TEXT("CloseIndicatorColor"),
		(bHitRecoveryActive || bParryOnCooldown)
			? FLinearColor(0.5f, 0.08f, 0.22f, 1.0f)
			: (bShowingParrySuccessIndicator
				? FLinearColor(1.0f, 0.35f, 0.9f, 1.0f)
				: FLinearColor(0.75f, 0.12f, 1.0f, 1.0f)));

	const float IndicatorOpacity = bShowingParrySuccessIndicator
		? ParryIndicatorSuccessOpacity
		: ((bHitRecoveryActive || bParryOnCooldown)
			? ParryIndicatorUnavailableOpacity
			: (bParryAttemptInProgress || bParryActive)
				? ParryIndicatorActiveOpacity
				: ParryIndicatorIdleOpacity);
	ParryRangeIndicatorMaterialInstance->SetScalarParameterValue(
		TEXT("IndicatorOpacity"),
		IndicatorOpacity);
}

void ARLPlayerCharacter::ShowParrySuccessIndicator()
{
	bShowingParrySuccessIndicator = true;
	UpdateParryIndicatorColor();
	GetWorldTimerManager().ClearTimer(ParrySuccessIndicatorTimerHandle);

	if (ParrySuccessIndicatorDuration <= 0.0f)
	{
		ClearParrySuccessIndicator();
		return;
	}

	GetWorldTimerManager().SetTimer(
		ParrySuccessIndicatorTimerHandle,
		this,
		&ThisClass::ClearParrySuccessIndicator,
		ParrySuccessIndicatorDuration,
		false);
}

void ARLPlayerCharacter::ClearParrySuccessIndicator()
{
	GetWorldTimerManager().ClearTimer(ParrySuccessIndicatorTimerHandle);
	bShowingParrySuccessIndicator = false;
	UpdateParryIndicatorColor();
}

void ARLPlayerCharacter::TriggerParryHitStop(bool bPerfectParry, bool bOverdrive)
{
	UWorld* World = GetWorld();
	if (!World || ParryHitStopDuration <= 0.0f || ParryHitStopTimeDilation >= 1.0f)
	{
		return;
	}

	bParryHitStopActive = true;
	const float EffectiveTimeDilation = ParryHitStopTimeDilation;
	UGameplayStatics::SetGlobalTimeDilation(World, EffectiveTimeDilation);
	const float DurationMultiplier = bOverdrive
		? OverdriveHitStopDurationMultiplier
		: (bPerfectParry ? PerfectHitStopDurationMultiplier : 1.0f);
	const float EffectiveHitStopDuration = ParryHitStopDuration * DurationMultiplier;
	GetWorldTimerManager().ClearTimer(ParryHitStopTimerHandle);
	GetWorldTimerManager().SetTimer(
		ParryHitStopTimerHandle,
		this,
		&ThisClass::RestoreTimeDilation,
		FMath::Max(KINDA_SMALL_NUMBER, EffectiveHitStopDuration * EffectiveTimeDilation),
		false);
}

void ARLPlayerCharacter::RestoreTimeDilation()
{
	GetWorldTimerManager().ClearTimer(ParryHitStopTimerHandle);
	if (!bParryHitStopActive)
	{
		return;
	}

	bParryHitStopActive = false;
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
	}
}

float ARLPlayerCharacter::TakeDamage(
	float DamageAmount,
	const FDamageEvent& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (!FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f || IsDead() || bHitRecoveryActive || IsRolling())
	{
		return 0.0f;
	}

	const float AppliedDamage = Super::TakeDamage(
		DamageAmount,
		DamageEvent,
		EventInstigator,
		DamageCauser);
	if (!FMath::IsFinite(AppliedDamage) || AppliedDamage <= 0.0f)
	{
		return 0.0f;
	}

	HealthComponent->ApplyDamage(AppliedDamage);
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			HitSound,
			GetActorLocation(),
			FMath::Max(0.0f, HitSoundVolume),
			FMath::FRandRange(
				FMath::Min(HitSoundPitchMin, HitSoundPitchMax),
				FMath::Max(HitSoundPitchMin, HitSoundPitchMax)));
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Player took %.1f damage. Health: %.1f / %.1f"),
		AppliedDamage,
		GetCurrentHealth(),
		GetMaxHealth());

	if (!IsDead())
	{
		BeginHitRecovery();
	}

	return AppliedDamage;
}

void ARLPlayerCharacter::BeginHitRecovery()
{
	if (bHitRecoveryActive || IsDead())
	{
		return;
	}

	bHitRecoveryActive = true;
	ClearParrySuccessIndicator();
	StartHitFlash();
	if (bParryAttemptInProgress || bParryActive)
	{
		EndParry(false);
	}
	else
	{
		DowngradeParryEnhancement();
		ResetParryChain();
	}
	StopAnimMontage();

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		PreHitRecoveryMaxWalkSpeed = MovementComponent->MaxWalkSpeed;
		MovementComponent->MaxWalkSpeed = PreHitRecoveryMaxWalkSpeed
			* FMath::Clamp(HitRecoveryMovementSpeedMultiplier, 0.0f, 1.0f);
	}
	UpdateParryIndicatorColor();

	GetWorldTimerManager().ClearTimer(HitRecoveryTimerHandle);
	if (HitRecoveryDuration <= 0.0f)
	{
		EndHitRecovery();
		return;
	}

	GetWorldTimerManager().SetTimer(
		HitRecoveryTimerHandle,
		this,
		&ThisClass::EndHitRecovery,
		HitRecoveryDuration,
		false);
}

void ARLPlayerCharacter::EndHitRecovery()
{
	GetWorldTimerManager().ClearTimer(HitRecoveryTimerHandle);
	if (!bHitRecoveryActive)
	{
		return;
	}

	bHitRecoveryActive = false;
	StopHitFlash();
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		if (PreHitRecoveryMaxWalkSpeed > 0.0f)
		{
			MovementComponent->MaxWalkSpeed = PreHitRecoveryMaxWalkSpeed;
		}
	}
	PreHitRecoveryMaxWalkSpeed = 0.0f;
	UpdateParryIndicatorColor();
}

void ARLPlayerCharacter::StartHitFlash()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh || !HitFlashMaterial)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(HitFlashTimerHandle);
	PreviousOverlayMaterial = CharacterMesh->GetOverlayMaterial();
	bHitFlashVisible = true;
	CharacterMesh->SetOverlayMaterial(HitFlashMaterial);

	if (HitFlashInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			HitFlashTimerHandle,
			this,
			&ThisClass::ToggleHitFlash,
			HitFlashInterval,
			true);
	}
}

void ARLPlayerCharacter::ToggleHitFlash()
{
	if (!bHitRecoveryActive)
	{
		StopHitFlash();
		return;
	}

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		bHitFlashVisible = !bHitFlashVisible;
		CharacterMesh->SetOverlayMaterial(
			bHitFlashVisible ? HitFlashMaterial.Get() : PreviousOverlayMaterial.Get());
	}
}

void ARLPlayerCharacter::StopHitFlash()
{
	GetWorldTimerManager().ClearTimer(HitFlashTimerHandle);
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetOverlayMaterial(PreviousOverlayMaterial);
	}
	PreviousOverlayMaterial = nullptr;
	bHitFlashVisible = false;
}

void ARLPlayerCharacter::Die_Implementation()
{
	DodgeRollComponent->StopRoll();
	GetWorldTimerManager().ClearTimer(HitRecoveryTimerHandle);
	StopHitFlash();
	bHitRecoveryActive = false;
	PreHitRecoveryMaxWalkSpeed = 0.0f;

	if (bParryAttemptInProgress || bParryActive)
	{
		EndParry(false);
	}
	else
	{
		ResetParryChain();
	}

	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);
	if (ARLGameModeBase* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ARLGameModeBase>()
		: nullptr)
	{
		GameMode->NotifyPlayerDied();
	}

	UE_LOG(LogTemp, Display, TEXT("Player died."));
}

// Called every frame
void ARLPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bParryActive)
	{
		UpdateParry();
	}
}

// Called to bind functionality to input
void ARLPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ARLPlayerCharacter::StartRoll(const FVector& Direction)
{
	if (IsDead() || bHitRecoveryActive || bParryAttemptInProgress || bParryActive)
	{
		return;
	}
	DodgeRollComponent->TryStartRoll(Direction);
}

bool ARLPlayerCharacter::IsRolling() const
{
	return DodgeRollComponent->IsRolling();
}

void ARLPlayerCharacter::StartParry()
{
	if (IsRolling())
	{
		return;
	}
	if (IsDead() || bHitRecoveryActive || bParryAttemptInProgress || bParryOnCooldown || !GetWorld())
	{
		return;
	}

	if (!ParryMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot start parry: ParryMontage is not assigned."));
		return;
	}

	if (DetonateExplosiveOnParryAttempt())
	{
		return;
	}

	// Alternate once per successful parry attempt. This is intentionally separate
	// from ParryChainCount because one swing can reflect multiple projectiles.
	UAnimMontage* MontageToPlay = ParryMontage;
	if (bPlayMirroredParryNext && MirroredParryMontage)
	{
		MontageToPlay = MirroredParryMontage;
	}

	const float MontageDuration = PlayAnimMontage(MontageToPlay);
	if (MontageDuration <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to play the parry montage."));
		return;
	}

	bParryAttemptInProgress = true;
	PlayParrySwingSound();
	UpdateParryIndicatorColor();

	// A notify state normally ends the attempt. This timer prevents a missing or
	// interrupted notify from leaving parry input permanently locked.
	GetWorldTimerManager().SetTimer(
		ParryAttemptTimerHandle,
		this,
		&ThisClass::EndParryWindow,
		MontageDuration + 0.1f,
		false);
}

void ARLPlayerCharacter::BeginParryWindow()
{
	if (!GetWorld() || IsDead() || bHitRecoveryActive || !bParryAttemptInProgress || bParryActive)
	{
		return;
	}

	bParryActive = true;
	UpdateParryIndicatorColor();
}

void ARLPlayerCharacter::EndParryWindow()
{
	if (!bParryAttemptInProgress)
	{
		return;
	}

	EndParry(false);
}

void ARLPlayerCharacter::UpdateParry()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerParry), false, this);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(ReflectionRange),
		QueryParams);

	int32 ParriedProjectileCount = 0;
	bool bPerfectParry = false;
	bool bCloseRangeParry = false;
	FVector ParrySoundLocation = GetActorLocation();
	const int32 ResultingCombo = ParryChainCount + 1;
	const bool bOverdrive = ParryEnhancementLevel >= 4;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (ARLProjectile* Projectile = Cast<ARLProjectile>(Overlap.GetActor()))
		{
			bool bProjectilePerfectParry = false;
			bool bProjectileCloseRangeParry = false;
			if (TryParryProjectile(
				Projectile,
				ResultingCombo,
				bProjectilePerfectParry,
				bProjectileCloseRangeParry))
			{
				if (ParriedProjectileCount == 0)
				{
					ParrySoundLocation = Projectile->GetActorLocation();
				}
				++ParriedProjectileCount;
				bPerfectParry |= bProjectilePerfectParry;
				bCloseRangeParry |= bProjectileCloseRangeParry;
			}
		}
	}

	if (ParriedProjectileCount > 0)
	{
		RegisterSuccessfulParry(
			ParriedProjectileCount,
			bPerfectParry,
			bCloseRangeParry,
			bOverdrive);
		PlayParryImpactSound(ParrySoundLocation, ParryEnhancementLevel);
		TriggerParryHitStop(bPerfectParry, bOverdrive);
		EndParry(true);
		if (bOverdrive)
		{
			ConsumeOverdriveEnhancement();
		}
	}
}

bool ARLPlayerCharacter::IsProjectileWithinParryArc(const ARLProjectile* Projectile) const
{
	if (!IsValid(Projectile))
	{
		return false;
	}

	const FVector OffsetToProjectile = Projectile->GetActorLocation() - GetActorLocation();
	if (OffsetToProjectile.SizeSquared2D() > FMath::Square(ReflectionRange))
	{
		return false;
	}

	const FVector DirectionToProjectile = OffsetToProjectile.GetSafeNormal2D();
	const FVector ForwardDirection = GetActorForwardVector().GetSafeNormal2D();
	const float MinimumForwardDot =
		FMath::Cos(FMath::DegreesToRadians(ReflectionHalfAngleDegrees));
	return FVector::DotProduct(ForwardDirection, DirectionToProjectile) >= MinimumForwardDot;
}

bool ARLPlayerCharacter::DetonateExplosiveOnParryAttempt()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerParryExplosiveCheck), false, this);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(ReflectionRange),
		QueryParams);

	ARLProjectile* ClosestExplosive = nullptr;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		ARLProjectile* Projectile = Cast<ARLProjectile>(Overlap.GetActor());
		if (!Projectile ||
			!Projectile->IsExplosive() ||
			Projectile->IsReflected() ||
			!IsProjectileWithinParryArc(Projectile))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(
			Projectile->GetActorLocation(),
			GetActorLocation());
		if (DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestExplosive = Projectile;
		}
	}

	if (!ClosestExplosive)
	{
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("Parry attempt immediately triggered an explosive projectile."));
	return ClosestExplosive->Detonate();
}

bool ARLPlayerCharacter::TryParryProjectile(
	ARLProjectile* Projectile,
	int32 ResultingCombo,
	bool& bOutPerfectParry,
	bool& bOutCloseRangeParry)
{
	bOutPerfectParry = false;
	bOutCloseRangeParry = false;
	if (!IsValid(Projectile) || Projectile->IsReflected() || !IsProjectileWithinParryArc(Projectile))
	{
		return false;
	}

	const float DistanceToProjectile = FVector::Dist2D(
		Projectile->GetActorLocation(),
		GetActorLocation());

	if (Projectile->IsExplosive())
	{
		UE_LOG(LogTemp, Display, TEXT("Parry attempt triggered an explosive projectile."));
		Projectile->Detonate();
		return false;
	}

	if (!Projectile->CanBeReflected())
	{
		return false;
	}

	const float PerfectBandInnerRadius = FMath::Max(
		0.0f,
		ReflectionRange - PerfectParryOuterBandWidth);
	const bool bPerfectParry = PerfectParryOuterBandWidth > 0.0f &&
		DistanceToProjectile >= PerfectBandInnerRadius &&
		DistanceToProjectile <= ReflectionRange;
	const bool bCloseRangeParry = CloseRangeThreshold > 0.0f &&
		DistanceToProjectile <= CloseRangeThreshold;
	const int32 RewardLevel = ParryEnhancementLevel < 4 &&
		EnhancementComboProgress + 1 >= GetEnhancementComboRequirement()
		? ParryEnhancementLevel + 1
		: ParryEnhancementLevel;
	const bool bOverdrive = ParryEnhancementLevel >= 4;
	const bool bMaximumSpeed = bCloseRangeParry || RewardLevel >= 2;

	const float SpeedMultiplier = bMaximumSpeed
		? MaxReflectedSpeedMultiplier
		: Projectile->GetBaseReflectedSpeedMultiplier();

	FRLProjectileReflectionParams ReflectionParams;
	ReflectionParams.SpeedMultiplier = SpeedMultiplier;
	ReflectionParams.VisualScaleMultiplier = bOverdrive
		? OverdriveProjectileScale
		: (bCloseRangeParry ? CloseRangeProjectileScale : BaseReflectedProjectileScale);
	ReflectionParams.PierceCount = bOverdrive
		? OverdrivePierceCount
		: (bCloseRangeParry ? CloseRangePierceCount : BasePierceCount);
	ReflectionParams.ReflectionChain = ResultingCombo;
	ReflectionParams.bPerfectParry = bPerfectParry;
	ReflectionParams.bCloseRangeParry = bCloseRangeParry;
	ReflectionParams.bOverdrive = bOverdrive;

	int32 SplitCount = bPerfectParry ? PerfectSplitProjectileCount : 1;
	float SplitSpreadAngle = bPerfectParry ? PerfectSplitAngleDegrees : 0.0f;
	const bool bGuardProjectile = Projectile->IsGuardProjectile();
	if (!bGuardProjectile && RewardLevel >= 3)
	{
		++SplitCount;
		if (SplitSpreadAngle <= 0.0f)
		{
			SplitSpreadAngle = ComboExtraProjectileSpreadAngle;
		}
	}
	if (!bGuardProjectile && bOverdrive)
	{
		SplitCount = OverdriveProjectileCount;
		SplitSpreadAngle = OverdriveSpreadAngleDegrees;
	}
	if (bGuardProjectile)
	{
		SplitCount = 1;
		SplitSpreadAngle = 0.0f;
		ReflectionParams.PierceCount = 0;
	}
	TArray<FVector> SplitDirections;
	SplitDirections.Reserve(SplitCount);
	const FVector ParryDirection = GetActorForwardVector().GetSafeNormal();
	for (int32 SplitIndex = 0; SplitIndex < SplitCount; ++SplitIndex)
	{
		const float SplitAlpha = SplitCount > 1
			? static_cast<float>(SplitIndex) / static_cast<float>(SplitCount - 1)
			: 0.5f;
		const float SplitAngle = FMath::Lerp(
			-SplitSpreadAngle * 0.5f,
			SplitSpreadAngle * 0.5f,
			SplitAlpha);
		SplitDirections.Add(ParryDirection.RotateAngleAxis(SplitAngle, FVector::UpVector));
	}

	if (!Projectile->Reflect(this, this, SplitDirections[0], ReflectionParams))
	{
		return false;
	}
	SpawnAdditionalReflectedProjectiles(Projectile, SplitDirections, ReflectionParams);
	bOutPerfectParry = bPerfectParry;
	bOutCloseRangeParry = bCloseRangeParry;

	return true;
}

void ARLPlayerCharacter::PlayParrySwingSound() const
{
	if (!ParrySwingSound)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(
		this,
		ParrySwingSound,
		GetActorLocation(),
		FRotator::ZeroRotator,
		ParrySwingSoundVolume);
}

void ARLPlayerCharacter::PlayParryImpactSound(
	const FVector& SoundLocation,
	int32 EnhancementLevel) const
{
	USoundBase* SoundToPlay = ParryImpactSound;
	float VolumeMultiplier = ParryImpactSoundVolume;
	// Preserve the authored milestone sounds: old combo 3/5/8 become enhancement stages 2/3/4.
	static constexpr int32 StageSoundIndices[] = {0, 2, 4, 7};
	const int32 StageIndex = FMath::Clamp(EnhancementLevel, 1, 4) - 1;
	const int32 ComboSoundIndex = StageSoundIndices[StageIndex];
	if (ParryComboImpactSounds.IsValidIndex(ComboSoundIndex) &&
		ParryComboImpactSounds[ComboSoundIndex])
	{
		SoundToPlay = ParryComboImpactSounds[ComboSoundIndex];
		if (ParryComboImpactSoundVolumes.IsValidIndex(ComboSoundIndex))
		{
			VolumeMultiplier = ParryComboImpactSoundVolumes[ComboSoundIndex];
		}
	}

	if (!SoundToPlay)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(
		this,
		SoundToPlay,
		SoundLocation,
		FRotator::ZeroRotator,
		VolumeMultiplier);
}

void ARLPlayerCharacter::RegisterSuccessfulParry(
	int32 MultiParryCount,
	bool bPerfectParry,
	bool bCloseRangeParry,
	bool bOverdrive)
{
	++ParryChainCount;
	if (bPerfectParry && RunRewardPerfectRecoveryAmount > 0)
	{
		RestoreHealth(RunRewardPerfectRecoveryAmount);
	}
	if (!bOverdrive)
	{
		++EnhancementComboProgress;
		if (ParryEnhancementLevel < 4 &&
			EnhancementComboProgress >= GetEnhancementComboRequirement())
		{
			SetParryEnhancementLevel(ParryEnhancementLevel + 1);
			EnhancementComboProgress = 0;
		}
	}
	OnParryChainChanged.Broadcast(ParryChainCount);
	OnParryComboChanged.Broadcast(
		ParryChainCount,
		FMath::Max(1, MultiParryCount),
		ParryEnhancementLevel,
		bPerfectParry,
		bCloseRangeParry);
	UpdateOverdriveAura();

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Parry combo: %d, Multi: %d, Perfect: %s, Close: %s, Overdrive: %s"),
		ParryChainCount,
		MultiParryCount,
		bPerfectParry ? TEXT("true") : TEXT("false"),
		bCloseRangeParry ? TEXT("true") : TEXT("false"),
		bOverdrive ? TEXT("true") : TEXT("false"));
}

void ARLPlayerCharacter::SpawnAdditionalReflectedProjectiles(
	ARLProjectile* SourceProjectile,
	const TArray<FVector>& SplitDirections,
	const FRLProjectileReflectionParams& ReflectionParams)
{
	if (!IsValid(SourceProjectile) || SplitDirections.Num() <= 1)
	{
		return;
	}

	UWorld* World = GetWorld();
	URLProjectilePoolSubsystem* PoolSubsystem = World
		? World->GetSubsystem<URLProjectilePoolSubsystem>()
		: nullptr;
	if (!PoolSubsystem)
	{
		return;
	}

	const FVector SpawnLocation = SourceProjectile->GetActorLocation();
	for (int32 SplitIndex = 1; SplitIndex < SplitDirections.Num(); ++SplitIndex)
	{
		const FVector SplitDirection = SplitDirections[SplitIndex].GetSafeNormal();
		const FTransform SpawnTransform(SplitDirection.Rotation(), SpawnLocation);
		ARLProjectile* SplitProjectile = PoolSubsystem->AcquireProjectile(
			SourceProjectile->GetClass(),
			SpawnTransform,
			this,
			this);
		if (SplitProjectile)
		{
			SplitProjectile->InitializeFromDefinition(
				SourceProjectile->GetProjectileDefinition(),
				nullptr,
				false);
		}
		if (!SplitProjectile ||
			!SplitProjectile->Reflect(this, this, SplitDirection, ReflectionParams))
		{
			if (SplitProjectile)
			{
				SplitProjectile->ReturnToPool();
			}
		}
	}
}

void ARLPlayerCharacter::EndParry(bool bSucceeded)
{
	if (!bParryAttemptInProgress && !bParryActive)
	{
		return;
	}

	bParryAttemptInProgress = false;
	bParryActive = false;
	GetWorldTimerManager().ClearTimer(ParryAttemptTimerHandle);

	if (!bSucceeded)
	{
		DowngradeParryEnhancement();
		ResetParryChain();
	}
	else
	{
		bPlayMirroredParryNext = !bPlayMirroredParryNext;
	}

	const float CooldownDuration = bSucceeded
		? SuccessfulParryCooldown
		: ReflectionCooldown;

	GetWorldTimerManager().ClearTimer(ParryCooldownTimerHandle);
	if (CooldownDuration <= 0.0f)
	{
		bParryOnCooldown = false;
		if (bSucceeded)
		{
			ShowParrySuccessIndicator();
		}
		else
		{
			ClearParrySuccessIndicator();
		}
		return;
	}

	bParryOnCooldown = true;
	if (bSucceeded)
	{
		ShowParrySuccessIndicator();
	}
	else
	{
		ClearParrySuccessIndicator();
	}
	GetWorldTimerManager().SetTimer(
		ParryCooldownTimerHandle,
		this,
		&ThisClass::ResetParryCooldown,
		CooldownDuration,
		false);
}

void ARLPlayerCharacter::ResetParryCooldown()
{
	bParryOnCooldown = false;
	UpdateParryIndicatorColor();
}

void ARLPlayerCharacter::ResetParryChain()
{
	bPlayMirroredParryNext = false;
	if (ParryChainCount == 0)
	{
		UpdateOverdriveAura();
		return;
	}

	ParryChainCount = 0;
	UpdateOverdriveAura();
	OnParryChainChanged.Broadcast(ParryChainCount);
	OnParryComboChanged.Broadcast(0, 0, ParryEnhancementLevel, false, false);
	UE_LOG(LogTemp, Display, TEXT("Parry chain reset."));
}

void ARLPlayerCharacter::ConsumeOverdriveEnhancement()
{
	SetParryEnhancementLevel(3);
	EnhancementComboProgress = 0;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Overdrive enhancement consumed; stage reduced to 3 while combo remains %d."),
		ParryChainCount);
}

void ARLPlayerCharacter::DowngradeParryEnhancement()
{
	SetParryEnhancementLevel(ParryEnhancementLevel - 1);
	EnhancementComboProgress = 0;
}

void ARLPlayerCharacter::SetParryEnhancementLevel(int32 NewLevel)
{
	const int32 ClampedLevel = FMath::Clamp(NewLevel, 1, 4);
	if (ParryEnhancementLevel == ClampedLevel)
	{
		return;
	}

	ParryEnhancementLevel = ClampedLevel;
	UpdateOverdriveAura();
	OnParryComboChanged.Broadcast(
		ParryChainCount,
		0,
		ParryEnhancementLevel,
		false,
		false);
	UE_LOG(LogTemp, Display, TEXT("Parry enhancement stage: %d"), ParryEnhancementLevel);
}

int32 ARLPlayerCharacter::GetEnhancementComboRequirement() const
{
	switch (ParryEnhancementLevel)
	{
	case 1:
		return FMath::Max(1, EnhancementStage2Combo);
	case 2:
		return FMath::Max(1, EnhancementStage3Combo - EnhancementStage2Combo);
	case 3:
		return FMath::Max(1, EnhancementStage4Combo - EnhancementStage3Combo);
	default:
		return MAX_int32;
	}
}
