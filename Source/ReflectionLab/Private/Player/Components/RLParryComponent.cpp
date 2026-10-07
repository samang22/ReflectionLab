#include "Player/Components/RLParryComponent.h"

#include "Animation/AnimMontage.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Data/RLPlayerStatsDataAsset.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Player/Components/RLParryFeedbackComponent.h"
#include "Player/Components/RLParryProgressionComponent.h"
#include "Player/Components/RLRunRewardComponent.h"
#include "TimerManager.h"

URLParryComponent::URLParryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// Assign montages on the player Blueprint component. Loading them here can
	// re-enter player class construction through the montage's Blueprint notifies.
}

void URLParryComponent::Initialize(URLParryProgressionComponent* Progression,
	URLRunRewardComponent* Rewards, URLParryFeedbackComponent* Feedback)
{
	ProgressionComponent = Progression;
	RewardComponent = Rewards;
	FeedbackComponent = Feedback;
}

void URLParryComponent::Configure(const URLPlayerStatsDataAsset* PlayerStatsData)
{
	FRLParryStats NewStats;
	if (PlayerStatsData)
	{
		NewStats.ReflectionCooldown = FMath::Max(0.0f, PlayerStatsData->FailedParryCooldown);
		NewStats.SuccessfulParryCooldown = FMath::Max(0.0f, PlayerStatsData->SuccessfulParryCooldown);
		NewStats.ReflectionRange = FMath::Max(1.0f, PlayerStatsData->ParryRange);
		NewStats.PerfectParryOuterBandWidth = FMath::Clamp(
			PlayerStatsData->PerfectParryOuterBandWidth,
			0.0f,
			NewStats.ReflectionRange);
		NewStats.PerfectSplitProjectileCount = FMath::Clamp(
			PlayerStatsData->PerfectSplitProjectileCount,
			1,
			8);
		NewStats.PerfectSplitAngleDegrees = FMath::Clamp(
			PlayerStatsData->PerfectSplitAngleDegrees,
			0.0f,
			90.0f);
		NewStats.BasePierceCount = FMath::Max(0, PlayerStatsData->BasePierceCount);
		NewStats.MaxReflectedSpeedMultiplier = FMath::Max(
			1.0f,
			PlayerStatsData->MaxReflectedSpeedMultiplier);
		NewStats.BaseReflectedProjectileScale = FMath::Max(
			1.0f,
			PlayerStatsData->BaseReflectedProjectileScale);
		NewStats.CloseRangeThreshold = FMath::Max(0.0f, PlayerStatsData->CloseRangeThreshold);
		NewStats.CloseRangePierceCount = FMath::Max(0, PlayerStatsData->CloseRangePierceCount);
		NewStats.CloseRangeProjectileScale = FMath::Max(1.0f, PlayerStatsData->CloseRangeProjectileScale);
		NewStats.ComboExtraProjectileSpreadAngle = FMath::Clamp(
			PlayerStatsData->ComboExtraProjectileSpreadAngle,
			0.0f,
			90.0f);
		NewStats.OverdriveProjectileCount = FMath::Clamp(PlayerStatsData->OverdriveProjectileCount, 1, 5);
		NewStats.OverdriveSpreadAngleDegrees = FMath::Clamp(
			PlayerStatsData->OverdriveSpreadAngleDegrees,
			0.0f,
			180.0f);
		NewStats.OverdriveProjectileScale = FMath::Max(1.0f, PlayerStatsData->OverdriveProjectileScale);
		NewStats.OverdrivePierceCount = FMath::Max(0, PlayerStatsData->OverdrivePierceCount);
		NewStats.ReflectionHalfAngleDegrees = FMath::Clamp(
			PlayerStatsData->ParryHalfAngleDegrees,
			0.0f,
			180.0f);
	}
	if (RewardComponent) { RewardComponent->ApplyModifiers(NewStats); }
	Stats = NewStats;
	OnStateChanged.Broadcast();
}

bool URLParryComponent::CanParry() const
{
	return IsValid(GetOwner()) && Cast<ACharacter>(GetOwner()) && ProgressionComponent && RewardComponent && FeedbackComponent
		&& CanPerformParry.IsBound() && CanPerformParry.Execute();
}

FRLParryViewState URLParryComponent::GetViewState() const
{
	FRLParryViewState State;
	State.Stats = Stats;
	State.EnhancementLevel = ProgressionComponent ? ProgressionComponent->GetEnhancementLevel() : 1;
	State.bAttemptInProgress = bParryAttemptInProgress;
	State.bActive = bParryActive;
	State.bOnCooldown = bParryOnCooldown;
	return State;
}

void URLParryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bParryActive && CanParry()) { UpdateParry(); }
}

void URLParryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetComponentTickEnabled(false);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ParryAttemptTimerHandle);
		World->GetTimerManager().ClearTimer(ParryCooldownTimerHandle);
	}
	bParryAttemptInProgress = false;
	bParryActive = false;
	bParryOnCooldown = false;
	CanPerformParry.Unbind();
	Super::EndPlay(EndPlayReason);
}

void URLParryComponent::CancelParry(bool bDowngradeEnhancement)
{
	if (bParryAttemptInProgress || bParryActive)
	{
		EndParry(false);
	}
	else if (ProgressionComponent)
	{
		if (bDowngradeEnhancement) { ProgressionComponent->DowngradeParryEnhancement(); }
		ResetParryChain();
	}
}

void URLParryComponent::ResetParryChain()
{
	bPlayMirroredParryNext = false;
	ProgressionComponent->ResetParryChain();
}

bool URLParryComponent::TryStartParry()
{
	if (!CanParry() || bParryAttemptInProgress || bParryOnCooldown || !GetWorld())
	{
		return false;
	}

	if (!ParryMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot start parry: ParryMontage is not assigned."));
		return false;
	}

	// Alternate once per successful parry attempt. This is intentionally separate
	// from ProgressionComponent->GetChainCount() because one swing can reflect multiple projectiles.
	UAnimMontage* MontageToPlay = ParryMontage;
	if (bPlayMirroredParryNext && MirroredParryMontage)
	{
		MontageToPlay = MirroredParryMontage;
	}

	const float MontageDuration = CastChecked<ACharacter>(GetOwner())->PlayAnimMontage(MontageToPlay);
	if (MontageDuration <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to play the parry montage."));
		return false;
	}

	bParryAttemptInProgress = true;
	FeedbackComponent->PlayParrySwingSound();
	OnStateChanged.Broadcast();

	// A notify state normally ends the attempt. This timer prevents a missing or
	// interrupted notify from leaving parry input permanently locked.
	GetWorld()->GetTimerManager().SetTimer(
		ParryAttemptTimerHandle,
		this,
		&ThisClass::EndParryWindow,
		MontageDuration + 0.1f,
		false);
	return true;
}

void URLParryComponent::BeginParryWindow()
{
	if (!GetWorld() || !CanParry() || !bParryAttemptInProgress || bParryActive)
	{
		return;
	}

	bParryActive = true;
	SetComponentTickEnabled(true);
	OnStateChanged.Broadcast();
}

void URLParryComponent::EndParryWindow()
{
	if (!bParryAttemptInProgress)
	{
		return;
	}

	EndParry(false);
}

void URLParryComponent::UpdateParry()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 ParriedProjectileCount = 0;
	bool bPerfectParry = false;
	bool bCloseRangeParry = false;
	FVector ParrySoundLocation = GetOwner()->GetActorLocation();
	const int32 ResultingCombo = ProgressionComponent->GetChainCount() + 1;
	const bool bOverdrive = ProgressionComponent->GetEnhancementLevel() >= 4;
	// Collision overlap queries have a Z extent. Enumerate projectiles instead
	// and let IsProjectileWithinParryArc perform the exact XY-only filtering.
	for (TActorIterator<ARLProjectile> Iterator(World); Iterator; ++Iterator)
	{
		if (!CanParry() || !bParryActive) { return; }
		ARLProjectile* Projectile = *Iterator;
		if (Projectile->IsPoolActive() && !Projectile->IsFadingOut())
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
		const FRLParryResult Result{ParriedProjectileCount, bPerfectParry, bCloseRangeParry, bOverdrive};
		RewardComponent->ApplyPerfectRecovery(Result);
		ProgressionComponent->RegisterSuccess(Result);
		FeedbackComponent->PlayParryImpactSound(ParrySoundLocation, ProgressionComponent->GetEnhancementLevel());
		FeedbackComponent->TriggerParryHitStop(bPerfectParry, bOverdrive);
		EndParry(true);
		if (bOverdrive)
		{
			ProgressionComponent->ConsumeOverdriveEnhancement();
		}
	}
}

bool URLParryComponent::IsProjectileWithinParryArc(const ARLProjectile* Projectile) const
{
	if (!IsValid(Projectile))
	{
		return false;
	}

	const FVector OffsetToProjectile = Projectile->GetActorLocation() - GetOwner()->GetActorLocation();
	if (OffsetToProjectile.SizeSquared2D() > FMath::Square(Stats.ReflectionRange))
	{
		return false;
	}

	const FVector DirectionToProjectile = OffsetToProjectile.GetSafeNormal2D();
	const FVector ForwardDirection = GetOwner()->GetActorForwardVector().GetSafeNormal2D();
	const float MinimumForwardDot =
		FMath::Cos(FMath::DegreesToRadians(Stats.ReflectionHalfAngleDegrees));
	return FVector::DotProduct(ForwardDirection, DirectionToProjectile) >= MinimumForwardDot;
}

bool URLParryComponent::TryParryProjectile(
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
		GetOwner()->GetActorLocation());

	if (!Projectile->CanBeReflected())
	{
		return false;
	}

	const float PerfectBandInnerRadius = FMath::Max(
		0.0f,
		Stats.ReflectionRange - Stats.PerfectParryOuterBandWidth);
	const bool bPerfectParry = Stats.PerfectParryOuterBandWidth > 0.0f &&
		DistanceToProjectile >= PerfectBandInnerRadius &&
		DistanceToProjectile <= Stats.ReflectionRange;
	const bool bCloseRangeParry = Stats.CloseRangeThreshold > 0.0f &&
		DistanceToProjectile <= Stats.CloseRangeThreshold;
	const int32 RewardLevel = ProgressionComponent->PreviewNextSuccess();
	const bool bOverdrive = ProgressionComponent->GetEnhancementLevel() >= 4;
	const bool bMaximumSpeed = bCloseRangeParry || RewardLevel >= 2;

	// Bombs retain their hostile travel speed; only purchased run rewards
	// accelerate them, not normal reflection or close/combo speed upgrades.
	const float SpeedMultiplier = Projectile->IsExplosive()
		? 1.0f + (RewardComponent ? FMath::Max(0.0f, RewardComponent->GetReflectedSpeedBonus()) : 0.0f)
		: (bMaximumSpeed ? Stats.MaxReflectedSpeedMultiplier : Projectile->GetBaseReflectedSpeedMultiplier());

	FRLProjectileReflectionParams ReflectionParams;
	ReflectionParams.SpeedMultiplier = SpeedMultiplier;
	ReflectionParams.VisualScaleMultiplier = bOverdrive
		? Stats.OverdriveProjectileScale
		: (bCloseRangeParry ? Stats.CloseRangeProjectileScale : Stats.BaseReflectedProjectileScale);
	ReflectionParams.PierceCount = bOverdrive
		? Stats.OverdrivePierceCount
		: (bCloseRangeParry ? Stats.CloseRangePierceCount : Stats.BasePierceCount);
	ReflectionParams.ReflectionChain = ResultingCombo;
	ReflectionParams.bPerfectParry = bPerfectParry;
	ReflectionParams.bCloseRangeParry = bCloseRangeParry;
	ReflectionParams.bOverdrive = bOverdrive;

	int32 SplitCount = bPerfectParry ? Stats.PerfectSplitProjectileCount : 1;
	float SplitSpreadAngle = bPerfectParry ? Stats.PerfectSplitAngleDegrees : 0.0f;
	const bool bGuardProjectile = Projectile->IsGuardProjectile();
	if (!bGuardProjectile && RewardLevel >= 3)
	{
		++SplitCount;
		if (SplitSpreadAngle <= 0.0f)
		{
			SplitSpreadAngle = Stats.ComboExtraProjectileSpreadAngle;
		}
	}
	if (!bGuardProjectile && bOverdrive)
	{
		SplitCount = Stats.OverdriveProjectileCount;
		SplitSpreadAngle = Stats.OverdriveSpreadAngleDegrees;
	}
	if (bGuardProjectile)
	{
		SplitCount = 1;
		SplitSpreadAngle = 0.0f;
		ReflectionParams.PierceCount = 0;
	}
	if (Projectile->IsRallyProjectile())
	{
		SplitCount = 1;
		SplitSpreadAngle = 0.0f;
		ReflectionParams.PierceCount = FMath::Clamp(ReflectionParams.PierceCount, 0, 1);
	}
	TArray<FVector> SplitDirections;
	SplitDirections.Reserve(SplitCount);
	const FVector ParryDirection = GetOwner()->GetActorForwardVector().GetSafeNormal();
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

	if (!Projectile->Reflect(GetOwner(), CastChecked<ACharacter>(GetOwner()), SplitDirections[0], ReflectionParams))
	{
		return false;
	}
	SpawnAdditionalReflectedProjectiles(Projectile, SplitDirections, ReflectionParams);
	bOutPerfectParry = bPerfectParry;
	bOutCloseRangeParry = bCloseRangeParry;

	return true;
}

void URLParryComponent::SpawnAdditionalReflectedProjectiles(
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
			GetOwner(),
			CastChecked<ACharacter>(GetOwner()));
		if (SplitProjectile)
		{
			SplitProjectile->InitializeFromDefinition(
				SourceProjectile->GetProjectileDefinition(),
				GetOwner(),
				SourceProjectile->IsExplosive() || SourceProjectile->IsRallyProjectile());
		}
		if (!SplitProjectile ||
			!SplitProjectile->Reflect(GetOwner(), CastChecked<ACharacter>(GetOwner()), SplitDirection, ReflectionParams))
		{
			if (SplitProjectile)
			{
				SplitProjectile->ReturnToPool();
			}
		}
	}
}

void URLParryComponent::EndParry(bool bSucceeded)
{
	if (!bParryAttemptInProgress && !bParryActive)
	{
		return;
	}

	bParryAttemptInProgress = false;
	bParryActive = false;
	SetComponentTickEnabled(false);
	GetWorld()->GetTimerManager().ClearTimer(ParryAttemptTimerHandle);

	if (!bSucceeded)
	{
		ProgressionComponent->DowngradeParryEnhancement();
		ResetParryChain();
	}
	else
	{
		bPlayMirroredParryNext = !bPlayMirroredParryNext;
	}

	const float CooldownDuration = bSucceeded
		? Stats.SuccessfulParryCooldown
		: Stats.ReflectionCooldown;

	GetWorld()->GetTimerManager().ClearTimer(ParryCooldownTimerHandle);
	if (CooldownDuration <= 0.0f)
	{
		bParryOnCooldown = false;
		OnStateChanged.Broadcast();
		if (bSucceeded)
		{
			FeedbackComponent->ShowParrySuccessIndicator();
		}
		else
		{
			FeedbackComponent->ClearParrySuccessIndicator();
		}
		return;
	}

	bParryOnCooldown = true;
	OnStateChanged.Broadcast();
	if (bSucceeded)
	{
		FeedbackComponent->ShowParrySuccessIndicator();
	}
	else
	{
		FeedbackComponent->ClearParrySuccessIndicator();
	}
	GetWorld()->GetTimerManager().SetTimer(
		ParryCooldownTimerHandle,
		this,
		&ThisClass::ResetParryCooldown,
		CooldownDuration,
		false);
}

void URLParryComponent::ResetParryCooldown()
{
	bParryOnCooldown = false;
	OnStateChanged.Broadcast();
}

