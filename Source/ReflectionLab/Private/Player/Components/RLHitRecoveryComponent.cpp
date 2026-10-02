#include "Player/Components/RLHitRecoveryComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Data/RLPlayerStatsDataAsset.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

URLHitRecoveryComponent::URLHitRecoveryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FlashMaterial(
		TEXT("/Game/ReflectionLab/Art/Materials/Characters/Instances/MI_HitFlash.MI_HitFlash"));
	Settings.FlashMaterial = FlashMaterial.Object;
	static ConstructorHelpers::FObjectFinder<USoundBase> HitSound(
		TEXT("/Game/ReflectionLab/Audio/SFX/Combat/PlayerHit/SFX_PlayerHit.SFX_PlayerHit"));
	Settings.HitSound = HitSound.Object;
}

void URLHitRecoveryComponent::SetPlayerStats(const URLPlayerStatsDataAsset* Stats)
{
	if (Stats && !bRecovering)
	{
		Settings.Duration = FMath::Max(0.0f, Stats->HitRecoveryDuration);
		Settings.MovementSpeedMultiplier = FMath::Clamp(Stats->HitRecoveryMovementSpeedMultiplier, 0.0f, 1.0f);
	}
}

void URLHitRecoveryComponent::Configure(const FRLHitRecoverySettings& NewSettings)
{
	// Do not replace the flash material or timing in the middle of recovery.
	if (!bRecovering)
	{
		Settings = NewSettings;
	}
}

void URLHitRecoveryComponent::PlayHitSound() const
{
	if (!GetOwner() || !Settings.HitSound || !FMath::IsFinite(Settings.SoundVolume)
		|| !FMath::IsFinite(Settings.SoundPitchMin) || !FMath::IsFinite(Settings.SoundPitchMax))
	{
		return;
	}
	UGameplayStatics::PlaySoundAtLocation(this, Settings.HitSound, GetOwner()->GetActorLocation(),
		FMath::Max(0.0f, Settings.SoundVolume), FMath::FRandRange(
			FMath::Max(0.1f, FMath::Min(Settings.SoundPitchMin, Settings.SoundPitchMax)),
			FMath::Max(0.1f, FMath::Max(Settings.SoundPitchMin, Settings.SoundPitchMax))));
}

bool URLHitRecoveryComponent::BeginRecovery()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!Character || !World || bRecovering || !FMath::IsFinite(Settings.Duration)
		|| !FMath::IsFinite(Settings.MovementSpeedMultiplier))
	{
		return false;
	}
	bRecovering = true;
	StartFlash();
	OnRecoveryStarted.Broadcast();
	// A listener may cancel recovery (for example, when the owner dies).
	if (!bRecovering)
	{
		return false;
	}
	Character->StopAnimMontage();
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		PreviousMaxWalkSpeed = Movement->MaxWalkSpeed;
		bRestoreWalkSpeed = true;
		Movement->MaxWalkSpeed *= FMath::Clamp(Settings.MovementSpeedMultiplier, 0.0f, 1.0f);
	}
	if (Settings.Duration <= 0.0f)
	{
		EndRecovery();
	}
	else
	{
		World->GetTimerManager().SetTimer(RecoveryTimerHandle, this,
			&ThisClass::EndRecovery, Settings.Duration, false);
	}
	return true;
}

void URLHitRecoveryComponent::EndRecovery()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RecoveryTimerHandle);
	}
	const bool bWasRecovering = bRecovering;
	bRecovering = false;
	StopFlash();
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement(); Movement && bRestoreWalkSpeed)
		{
			Movement->MaxWalkSpeed = PreviousMaxWalkSpeed;
		}
	}
	bRestoreWalkSpeed = false;
	PreviousMaxWalkSpeed = 0.0f;
	if (bWasRecovering)
	{
		OnRecoveryEnded.Broadcast();
	}
}

void URLHitRecoveryComponent::StartFlash()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!Mesh || !Settings.FlashMaterial)
	{
		return;
	}
	PreviousOverlayMaterial = Mesh->GetOverlayMaterial();
	bFlashActive = true;
	bFlashVisible = true;
	Mesh->SetOverlayMaterial(Settings.FlashMaterial);
	if (Settings.FlashInterval > 0.0f && FMath::IsFinite(Settings.FlashInterval))
	{
		GetWorld()->GetTimerManager().SetTimer(FlashTimerHandle, this,
			&ThisClass::ToggleFlash, Settings.FlashInterval, true);
	}
}

void URLHitRecoveryComponent::ToggleFlash()
{
	if (!bRecovering)
	{
		StopFlash();
		return;
	}
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			bFlashVisible = !bFlashVisible;
			Mesh->SetOverlayMaterial(bFlashVisible ? Settings.FlashMaterial.Get() : PreviousOverlayMaterial.Get());
		}
	}
}

void URLHitRecoveryComponent::StopFlash()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FlashTimerHandle);
	}
	if (bFlashActive)
	{
		if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
		{
			if (USkeletalMeshComponent* Mesh = Character->GetMesh())
			{
				Mesh->SetOverlayMaterial(PreviousOverlayMaterial);
			}
		}
	}
	PreviousOverlayMaterial = nullptr;
	bFlashActive = false;
	bFlashVisible = false;
}

void URLHitRecoveryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndRecovery();
	Super::EndPlay(EndPlayReason);
}
