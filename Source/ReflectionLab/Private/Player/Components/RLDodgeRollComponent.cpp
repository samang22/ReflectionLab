#include "Player/Components/RLDodgeRollComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/RLPlayerStatsDataAsset.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/RLRollAfterimagePoolSubsystem.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

URLDodgeRollComponent::URLDodgeRollComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	static ConstructorHelpers::FObjectFinder<UAnimMontage> RollFinder(
		TEXT("/Game/ReflectionLab/Gameplay/Player/Animations/AM_Player_Roll.AM_Player_Roll"));
	RollMontage = RollFinder.Object;
}

bool URLDodgeRollComponent::TryStartRoll(const FVector& Direction)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	UWorld* World = GetWorld();
	UAnimInstance* AnimInstance = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	const URLPlayerStatsDataAsset* Tuning = PlayerStatsData;
	if (!World || !Movement || bRolling || World->GetTimeSeconds() < NextRollTime || !Movement->IsMovingOnGround()
		|| !AnimInstance || !Tuning || !RollMontage || Direction.ContainsNaN()
		|| !FMath::IsFinite(Tuning->RollPlayRate) || Tuning->RollPlayRate <= 0.0f
		|| !FMath::IsFinite(Tuning->RollCooldown) || Tuning->RollCooldown < 0.0f)
	{
		return false;
	}
	const FVector FlatDirection = Direction.GetSafeNormal2D();
	if (FlatDirection.IsNearlyZero())
	{
		return false;
	}
	if (!RollMontage->HasRootMotion() ||
		(AnimInstance->RootMotionMode != ERootMotionMode::RootMotionFromMontagesOnly &&
		 AnimInstance->RootMotionMode != ERootMotionMode::RootMotionFromEverything))
	{
		UE_LOG(LogTemp, Warning, TEXT("Roll requires a root-motion montage and root-motion extraction in its AnimInstance."));
		return false;
	}
	if (Character->PlayAnimMontage(RollMontage, Tuning->RollPlayRate) <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Roll montage could not be played."));
		return false;
	}
	RollingCharacter = Character;
	ActiveRollMontage = RollMontage;
	ActiveRollCooldown = Tuning->RollCooldown;
	bRolling = true;
	bPreviousUseControllerDesiredRotation = Movement->bUseControllerDesiredRotation;
	Movement->bUseControllerDesiredRotation = false;
	Character->SetActorRotation(FlatDirection.Rotation());
	Movement->StopMovementImmediately();
	Character->ConsumeMovementInputVector();
	FOnMontageBlendingOutStarted BlendingOutDelegate;
	BlendingOutDelegate.BindUObject(this, &ThisClass::HandleMontageBlendingOut);
	AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, RollMontage);
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &ThisClass::HandleMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, RollMontage);
	SetComponentTickEnabled(true);
	AddTickPrerequisiteComponent(Character->GetMesh());
	StartRollFeedback(*Character, *Tuning);
	return true;
}

void URLDodgeRollComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ACharacter* Character = RollingCharacter.Get();
	if (!bRolling || !Character)
	{
		StopRoll();
		return;
	}
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	// Movement and collision are handled exclusively by CharacterMovement root motion.
	// Montage callbacks normally end the roll; this guards against an AnimInstance replacement.
	if (!AnimInstance || !AnimInstance->Montage_IsActive(ActiveRollMontage))
	{
		StopRoll();
		return;
	}
	if (ActiveAfterimageMaterial && GetWorld() && GetWorld()->GetTimeSeconds() >= NextAfterimageTime)
	{
		SpawnAfterimage(*Character);
		// Emit at most one snapshot per frame; don't catch up after a hitch.
		NextAfterimageTime = GetWorld()->GetTimeSeconds() + ActiveAfterimageInterval;
	}
}

void URLDodgeRollComponent::StopRoll()
{
	if (!bRolling)
	{
		SetComponentTickEnabled(false);
		return;
	}
	bRolling = false;
	ActiveAfterimageMaterial = nullptr;
	if (UWorld* World = GetWorld())
	{
		NextRollTime = World->GetTimeSeconds() + ActiveRollCooldown;
	}
	SetComponentTickEnabled(false);
	if (ACharacter* Character = RollingCharacter.Get())
	{
		RemoveTickPrerequisiteComponent(Character->GetMesh());
		if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
		{
			// Cancellation must not leave root motion active after roll invulnerability ends.
			AnimInstance->Montage_Stop(0.0f, ActiveRollMontage);
		}
		UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		Movement->StopMovementImmediately();
		Movement->bUseControllerDesiredRotation = bPreviousUseControllerDesiredRotation;
	}
	RollingCharacter.Reset();
	ActiveRollMontage = nullptr;
}

void URLDodgeRollComponent::StartRollFeedback(ACharacter& Character, const URLPlayerStatsDataAsset& Tuning)
{
	if (Tuning.RollSound && FMath::IsFinite(Tuning.RollSoundVolume) && FMath::IsFinite(Tuning.RollSoundPitch))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Tuning.RollSound, Character.GetActorLocation(),
			FMath::Clamp(Tuning.RollSoundVolume, 0.0f, 1.0f), FMath::Clamp(Tuning.RollSoundPitch, 0.1f, 2.0f));
	}
	ActiveAfterimageMaterial = nullptr;
	if (!Tuning.bRollAfterimagesEnabled || !Tuning.RollAfterimageMaterial ||
		!FMath::IsFinite(Tuning.RollAfterimageInterval) || !FMath::IsFinite(Tuning.RollAfterimageLifetime) ||
		!FMath::IsFinite(Tuning.RollAfterimageOpacity))
	{
		return;
	}
	ActiveAfterimageMaterial = Tuning.RollAfterimageMaterial;
	ActiveAfterimageColor = Tuning.RollAfterimageColor;
	ActiveAfterimageInterval = FMath::Clamp(Tuning.RollAfterimageInterval, 0.03f, 0.5f);
	ActiveAfterimageLifetime = FMath::Clamp(Tuning.RollAfterimageLifetime, 0.02f, 1.0f);
	ActiveAfterimageOpacity = FMath::Clamp(Tuning.RollAfterimageOpacity, 0.0f, 1.0f);
	if (URLRollAfterimagePoolSubsystem* Pool = GetWorld()->GetSubsystem<URLRollAfterimagePoolSubsystem>())
	{
		const int32 PoolSize = FMath::CeilToInt(ActiveAfterimageLifetime / ActiveAfterimageInterval) + 1;
		Pool->PrewarmPool(PoolSize, Character.GetMesh(), ActiveAfterimageMaterial);
	}
	// Wait for the first evaluated roll pose instead of snapshotting the idle pose.
	NextAfterimageTime = GetWorld()->GetTimeSeconds() + ActiveAfterimageInterval;
}

void URLDodgeRollComponent::SpawnAfterimage(ACharacter& Character)
{
	USkeletalMeshComponent* Mesh = Character.GetMesh();
	UWorld* World = GetWorld();
	if (!World || !Mesh || !ActiveAfterimageMaterial) { return; }
	if (URLRollAfterimagePoolSubsystem* Pool = World->GetSubsystem<URLRollAfterimagePoolSubsystem>())
	{
		Pool->AcquireAfterimage(Mesh, ActiveAfterimageMaterial, ActiveAfterimageColor,
			ActiveAfterimageLifetime, ActiveAfterimageOpacity);
	}
}

void URLDodgeRollComponent::HandleMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted && Montage == ActiveRollMontage)
	{
		StopRoll();
	}
}

void URLDodgeRollComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopRoll();
	Super::EndPlay(EndPlayReason);
}

void URLDodgeRollComponent::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == ActiveRollMontage)
	{
		StopRoll();
	}
}
