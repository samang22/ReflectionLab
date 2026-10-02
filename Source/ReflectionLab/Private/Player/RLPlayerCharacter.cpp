#include "Player/RLPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/RLPlayerStatsDataAsset.h"
#include "Engine/World.h"
#include "Framework/GameMode/RLGameModeBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/Components/RLDodgeRollComponent.h"
#include "Player/Components/RLHealthComponent.h"
#include "Player/Components/RLHitRecoveryComponent.h"
#include "Player/Components/RLParryComponent.h"
#include "Player/Components/RLParryFeedbackComponent.h"
#include "Player/Components/RLParryProgressionComponent.h"
#include "Player/Components/RLRunRewardComponent.h"

ARLPlayerCharacter::ARLPlayerCharacter()
{
	HealthComponent = CreateDefaultSubobject<URLHealthComponent>(TEXT("HealthComponent"));
	HitRecoveryComponent = CreateDefaultSubobject<URLHitRecoveryComponent>(TEXT("HitRecoveryComponent"));
	DodgeRollComponent = CreateDefaultSubobject<URLDodgeRollComponent>(TEXT("DodgeRollComponent"));
	RunRewardComponent = CreateDefaultSubobject<URLRunRewardComponent>(TEXT("RunRewardComponent"));
	ParryProgressionComponent = CreateDefaultSubobject<URLParryProgressionComponent>(TEXT("ParryProgressionComponent"));
	ParryComponent = CreateDefaultSubobject<URLParryComponent>(TEXT("ParryComponent"));
	ParryFeedbackComponent = CreateDefaultSubobject<URLParryFeedbackComponent>(TEXT("ParryFeedbackComponent"));
	ParryFeedbackComponent->SetupAttachment(RootComponent);
	// Preserve Blueprint Event Tick support; parry detection now ticks independently.
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
}

void ARLPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent->OnHealthChanged.AddUniqueDynamic(this, &ThisClass::HandleHealthChanged);
	HealthComponent->OnDeath.AddUniqueDynamic(this, &ThisClass::HandleDeath);
	HitRecoveryComponent->OnRecoveryStarted.AddUObject(this, &ThisClass::HandleHitRecoveryStarted);
	HitRecoveryComponent->OnRecoveryEnded.AddUObject(this, &ThisClass::HandleHitRecoveryEnded);
	ParryProgressionComponent->OnParryChainChanged.AddUObject(this, &ThisClass::HandleParryChainChanged);
	ParryProgressionComponent->OnParryComboChanged.AddUObject(this, &ThisClass::HandleParryComboChanged);
	RunRewardComponent->OnRewardsChanged.AddUObject(this, &ThisClass::HandleRewardsChanged);
	ParryComponent->OnStateChanged.AddUObject(this, &ThisClass::RefreshParryFeedback);
	ParryComponent->CanPerformParry.BindUObject(this, &ThisClass::CanPerformParry);
	ApplyPlayerStats();
	HealthComponent->InitializeHealth(GetMaxHealth());
}

void ARLPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HealthComponent->OnHealthChanged.RemoveAll(this);
	HealthComponent->OnDeath.RemoveAll(this);
	HitRecoveryComponent->OnRecoveryStarted.RemoveAll(this);
	HitRecoveryComponent->OnRecoveryEnded.RemoveAll(this);
	ParryProgressionComponent->OnParryChainChanged.RemoveAll(this);
	ParryProgressionComponent->OnParryComboChanged.RemoveAll(this);
	RunRewardComponent->OnRewardsChanged.RemoveAll(this);
	ParryComponent->OnStateChanged.RemoveAll(this);
	ParryComponent->CanPerformParry.Unbind();
	HitRecoveryComponent->EndRecovery();
	Super::EndPlay(EndPlayReason);
}

void ARLPlayerCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyPlayerStats();
}

void ARLPlayerCharacter::ApplyPlayerStats()
{
	RunRewardComponent->Initialize(HealthComponent);
	ParryComponent->Initialize(ParryProgressionComponent, RunRewardComponent, ParryFeedbackComponent);
	DodgeRollComponent->SetPlayerStats(PlayerStatsData);
	HitRecoveryComponent->SetPlayerStats(PlayerStatsData);
	if (PlayerStatsData)
	{
		HealthComponent->SetMaxHealth(FMath::Max(1.0f, PlayerStatsData->MaxHealth)
			+ RunRewardComponent->GetMaxHealthBonus());
		GetCharacterMovement()->MaxWalkSpeed = FMath::Max(0.0f, PlayerStatsData->MaxWalkSpeed);
	}
	ParryProgressionComponent->Configure(PlayerStatsData);
	ParryFeedbackComponent->Configure(PlayerStatsData);
	ParryComponent->Configure(PlayerStatsData);
	RefreshParryFeedback();
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

bool ARLPlayerCharacter::IsInHitRecovery() const
{
	return HitRecoveryComponent->IsRecovering();
}

bool ARLPlayerCharacter::IsRolling() const
{
	return DodgeRollComponent->IsRolling();
}

float ARLPlayerCharacter::TakeDamage(
	float DamageAmount,
	const FDamageEvent& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (!FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f || IsDead() || IsInHitRecovery() || IsRolling())
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
	HitRecoveryComponent->PlayHitSound();

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Player took %.1f damage. Health: %.1f / %.1f"),
		AppliedDamage,
		GetCurrentHealth(),
		GetMaxHealth());

	if (!IsDead())
	{
		HitRecoveryComponent->BeginRecovery();
	}

	return AppliedDamage;
}

void ARLPlayerCharacter::Die_Implementation()
{
	DodgeRollComponent->StopRoll();
	HitRecoveryComponent->EndRecovery();

	ParryComponent->CancelParry(false);

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

void ARLPlayerCharacter::StartRoll(const FVector& Direction)
{
	if (IsDead() || IsInHitRecovery() || ParryComponent->IsAttemptInProgress() || IsParryActive())
	{
		return;
	}
	DodgeRollComponent->TryStartRoll(Direction);
}

bool ARLPlayerCharacter::CanPerformParry() const
{
	return !IsDead() && !IsInHitRecovery() && !IsRolling();
}

void ARLPlayerCharacter::StartParry()
{
	if (CanPerformParry()) { ParryComponent->TryStartParry(); }
}

void ARLPlayerCharacter::BeginParryWindow()
{
	if (CanPerformParry()) { ParryComponent->BeginParryWindow(); }
}

void ARLPlayerCharacter::EndParryWindow()
{
	ParryComponent->EndParryWindow();
}

bool ARLPlayerCharacter::IsParryActive() const { return ParryComponent->IsActive(); }
bool ARLPlayerCharacter::IsParryOnCooldown() const { return ParryComponent->IsOnCooldown(); }
int32 ARLPlayerCharacter::GetParryChainCount() const { return ParryProgressionComponent->GetChainCount(); }
int32 ARLPlayerCharacter::GetParryEnhancementLevel() const { return ParryProgressionComponent->GetEnhancementLevel(); }
bool ARLPlayerCharacter::HasPerfectRecoveryReward() const { return RunRewardComponent->HasPerfectRecoveryReward(); }

void ARLPlayerCharacter::ApplyRunReward(ERLRunRewardType RewardType)
{
	// Also supports reward previews/tests before BeginPlay.
	RunRewardComponent->Initialize(HealthComponent);
	RunRewardComponent->ApplyRunReward(RewardType);
	if (!HasActorBegunPlay()) { HandleRewardsChanged(); }
}

void ARLPlayerCharacter::ResetRunRewards()
{
	RunRewardComponent->Initialize(HealthComponent);
	RunRewardComponent->ResetRunRewards();
	if (!HasActorBegunPlay()) { HandleRewardsChanged(); }
}

void ARLPlayerCharacter::HandleRewardsChanged()
{
	ParryComponent->Initialize(ParryProgressionComponent, RunRewardComponent, ParryFeedbackComponent);
	ParryComponent->Configure(PlayerStatsData);
	RefreshParryFeedback();
}

void ARLPlayerCharacter::HandleParryChainChanged(int32 ChainCount)
{
	OnParryChainChanged.Broadcast(ChainCount);
}

void ARLPlayerCharacter::HandleParryComboChanged(int32 ComboCount, int32 MultiParryCount,
	int32 EnhancementLevel, bool bPerfectParry, bool bCloseRangeParry)
{
	RefreshParryFeedback();
	OnParryComboChanged.Broadcast(ComboCount, MultiParryCount, EnhancementLevel, bPerfectParry, bCloseRangeParry);
}

void ARLPlayerCharacter::RefreshParryFeedback()
{
	FRLParryViewState State = ParryComponent->GetViewState();
	State.bInHitRecovery = IsInHitRecovery();
	ParryFeedbackComponent->UpdateView(State);
}

void ARLPlayerCharacter::HandleHitRecoveryStarted()
{
	ParryFeedbackComponent->ClearParrySuccessIndicator();
	ParryComponent->CancelParry(true);
	RefreshParryFeedback();
}

void ARLPlayerCharacter::HandleHitRecoveryEnded()
{
	RefreshParryFeedback();
}
