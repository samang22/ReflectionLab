#include "Player/Components/RLParryFeedbackComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "Data/RLPlayerStatsDataAsset.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

URLParryFeedbackComponent::URLParryFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<USoundBase> ImpactSound(
		TEXT("/Game/ReflectionLab/Audio/SFX/Combat/Parry/SC_ParryImpact.SC_ParryImpact"));
	ParryImpactSound = ImpactSound.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> IndicatorMaterial(
		TEXT("/Game/ReflectionLab/Art/Materials/M_ParryRangeIndicator.M_ParryRangeIndicator"));
	ParryRangeIndicatorMaterial = IndicatorMaterial.Object;
	ParryRangeIndicator = CreateDefaultSubobject<UDecalComponent>(TEXT("ParryRangeIndicator"));
	ParryRangeIndicator->SetupAttachment(this);
	// The decal material's vertical UV axis maps to the component's local right
	// axis after pitching it toward the floor, so rotate it back onto character forward.
	ParryRangeIndicator->SetRelativeRotation(FRotator(-90.0f, -90.0f, 0.0f));
	ParryRangeIndicator->DecalSize = FVector(64.0f, View.Stats.ReflectionRange, View.Stats.ReflectionRange);
	ParryRangeIndicator->SetSortOrder(5);
	ParryRangeIndicator->FadeScreenSize = 0.0f;
	ParryRangeIndicator->SetDecalMaterial(ParryRangeIndicatorMaterial);

	OverdriveAuraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("OverdriveAuraComponent"));
	OverdriveAuraComponent->SetupAttachment(this);
	OverdriveAuraComponent->SetAutoActivate(false);
	OverdriveAuraComponent->SetAutoDestroy(false);
}

void URLParryFeedbackComponent::OnRegister()
{
	Super::OnRegister();
	if (IsTemplate())
	{
		return;
	}

	// Blueprint instancing can leave nested subobjects attached to the component
	// template. Rebind them to this instance before their own registration.
	USceneComponent* const FeedbackChildren[] = {ParryRangeIndicator.Get(), OverdriveAuraComponent.Get()};
	for (USceneComponent* Child : FeedbackChildren)
	{
		if (Child && !Child->IsTemplate() && Child->GetAttachParent() != this)
		{
			ensureMsgf(
				Child->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform),
				TEXT("Failed to attach parry feedback component %s to %s."),
				*Child->GetName(),
				*GetName());
		}
	}
}

void URLParryFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();
	if (ParryRangeIndicatorMaterial)
	{
		ParryRangeIndicator->SetDecalMaterial(ParryRangeIndicatorMaterial);
		ParryRangeIndicatorMaterialInstance = ParryRangeIndicator->CreateDynamicMaterialInstance();
	}
	UpdateParryRangeIndicator();
	UpdateOverdriveAura();
}

void URLParryFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearParrySuccessIndicator();
	if (IsValid(PerfectParryAudioComponent)) { PerfectParryAudioComponent->Stop(); }
	PerfectParryAudioComponent = nullptr;
	if (IsValid(PowerLevelUpAudioComponent)) { PowerLevelUpAudioComponent->Stop(); }
	PowerLevelUpAudioComponent = nullptr;
	RestoreTimeDilation();
	OverdriveAuraComponent->Deactivate();
	Super::EndPlay(EndPlayReason);
}

void URLParryFeedbackComponent::Configure(const URLPlayerStatsDataAsset* PlayerStatsData)
{
	if (!PlayerStatsData) { return; }
	PerfectHitStopDurationMultiplier = FMath::Max(
		1.0f,
		PlayerStatsData->PerfectHitStopDurationMultiplier);
	OverdriveHitStopDurationMultiplier = FMath::Max(
		1.0f,
		PlayerStatsData->OverdriveHitStopDurationMultiplier);
	ParryIndicatorIdleOpacity = FMath::Clamp(PlayerStatsData->IndicatorIdleOpacity, 0.0f, 1.0f);
	ParryIndicatorActiveOpacity = FMath::Clamp(PlayerStatsData->IndicatorActiveOpacity, 0.0f, 1.0f);
	ParryIndicatorSuccessOpacity = FMath::Clamp(PlayerStatsData->IndicatorSuccessOpacity, 0.0f, 1.0f);
	ParryIndicatorUnavailableOpacity = FMath::Clamp(
		PlayerStatsData->IndicatorUnavailableOpacity,
		0.0f,
		1.0f);
	ParryImpactSoundVolume = FMath::Clamp(PlayerStatsData->ImpactSoundVolume, 0.0f, 1.0f);
	PerfectParrySound = PlayerStatsData->PerfectParrySound;
	PerfectParrySoundVolume = FMath::Clamp(PlayerStatsData->PerfectParrySoundVolume, 0.0f, 2.0f);
	PerfectImpactVolumeMultiplier = FMath::Clamp(PlayerStatsData->PerfectImpactVolumeMultiplier, 0.0f, 1.0f);
	PowerLevelUpSound = PlayerStatsData->PowerLevelUpSound;
	PowerLevelUpSoundVolume = FMath::Clamp(PlayerStatsData->PowerLevelUpSoundVolume, 0.0f, 2.0f);
	ParrySwingSound = PlayerStatsData->SwingSound;
	ParrySwingSoundVolume = FMath::Clamp(PlayerStatsData->SwingSoundVolume, 0.0f, 1.0f);
	ParryComboImpactSounds = PlayerStatsData->ComboImpactSounds;
	ParryHitStopDuration = FMath::Max(0.0f, PlayerStatsData->HitStopDuration);
	ParryHitStopTimeDilation = FMath::Clamp(PlayerStatsData->HitStopTimeDilation, 0.01f, 1.0f);
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
	ParryComboImpactSoundVolumes.Reset(PlayerStatsData->ComboImpactSoundVolumes.Num());
	for (const float Volume : PlayerStatsData->ComboImpactSoundVolumes)
	{
		ParryComboImpactSoundVolumes.Add(FMath::Clamp(Volume, 0.0f, 2.0f));
	}

	OverdriveAuraComponent->SetAsset(PlayerStatsData->OverdriveAuraVFX);
}

void URLParryFeedbackComponent::UpdateView(const FRLParryViewState& State)
{
	// The progression broadcasts twice on a level-up; only the actual rising
	// edge should sound. Initial synchronization and downgrades remain silent.
	const bool bLevelIncreased = bHasViewState && State.EnhancementLevel > View.EnhancementLevel;
	View = State;
	bHasViewState = true;
	if (bLevelIncreased && PowerLevelUpSound && PowerLevelUpSoundVolume > 0.0f && GetWorld())
	{
		if (IsValid(PowerLevelUpAudioComponent)) { PowerLevelUpAudioComponent->Stop(); }
		PowerLevelUpAudioComponent = UGameplayStatics::SpawnSound2D(this, PowerLevelUpSound, PowerLevelUpSoundVolume);
	}
	UpdateParryRangeIndicator();
	UpdateOverdriveAura();
}

void URLParryFeedbackComponent::UpdateOverdriveAura()
{
	if (!OverdriveAuraComponent)
	{
		return;
	}

	float AuraScaleMultiplier = 0.0f;
	switch (View.EnhancementLevel)
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
		const bool bScaleChanged = !OverdriveAuraComponent->GetRelativeScale3D().Equals(FVector(AuraScale));
		OverdriveAuraComponent->SetRelativeScale3D(FVector(AuraScale));
		if (!OverdriveAuraComponent->IsActive() || bScaleChanged)
		{
			OverdriveAuraComponent->Activate(true);
		}
	}
	else
	{
		OverdriveAuraComponent->Deactivate();
	}
}

void URLParryFeedbackComponent::UpdateParryRangeIndicator()
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!ParryRangeIndicator || !Character)
	{
		return;
	}

	ParryRangeIndicator->SetVisibility(bShowParryRangeIndicator, true);
	ParryRangeIndicator->DecalSize = FVector(64.0f, View.Stats.ReflectionRange, View.Stats.ReflectionRange);
	ParryRangeIndicator->SetRelativeLocation(FVector(
		0.0f,
		0.0f,
		-Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + 30.0f));
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
			View.Stats.ReflectionHalfAngleDegrees,
			1.0f,
			MaxIndicatorHalfAngleDegrees);
		const float ConeSlope = FMath::Tan(
			FMath::DegreesToRadians(VisualHalfAngleDegrees));
		ParryRangeIndicatorMaterialInstance->SetScalarParameterValue(
			TEXT("ConeSlope"),
			ConeSlope);
		const float PerfectBandInnerRadiusUv = 0.5f *
			(1.0f - View.Stats.PerfectParryOuterBandWidth / View.Stats.ReflectionRange);
		ParryRangeIndicatorMaterialInstance->SetScalarParameterValue(
			TEXT("PerfectBandInnerRadiusUV"),
			PerfectBandInnerRadiusUv);
		UpdateParryIndicatorColor();
		ParryRangeIndicatorMaterialInstance->SetScalarParameterValue(
			TEXT("CloseRadiusUV"), 0.5f * FMath::Clamp(
				View.Stats.CloseRangeThreshold / FMath::Max(1.0f, View.Stats.ReflectionRange), 0.0f, 1.0f));
	}
}

void URLParryFeedbackComponent::UpdateParryIndicatorColor()
{
	if (!ParryRangeIndicatorMaterialInstance)
	{
		return;
	}

	const FLinearColor IndicatorColor = View.bInHitRecovery
		? ParryCooldownIndicatorColor
		: (bShowingParrySuccessIndicator
			? ParrySuccessIndicatorColor
			: (View.bOnCooldown ? ParryCooldownIndicatorColor : ParryAvailableIndicatorColor));
	const FLinearColor PerfectIndicatorColor = View.bInHitRecovery
		? PerfectParryCooldownIndicatorColor
		: (bShowingParrySuccessIndicator
			? PerfectParrySuccessIndicatorColor
			: (View.bOnCooldown
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
		(View.bInHitRecovery || View.bOnCooldown)
			? FLinearColor(0.5f, 0.08f, 0.22f, 1.0f)
			: (bShowingParrySuccessIndicator
				? FLinearColor(1.0f, 0.35f, 0.9f, 1.0f)
				: FLinearColor(0.75f, 0.12f, 1.0f, 1.0f)));

	const float IndicatorOpacity = bShowingParrySuccessIndicator
		? ParryIndicatorSuccessOpacity
		: ((View.bInHitRecovery || View.bOnCooldown)
			? ParryIndicatorUnavailableOpacity
			: (View.bAttemptInProgress || View.bActive)
				? ParryIndicatorActiveOpacity
				: ParryIndicatorIdleOpacity);
	ParryRangeIndicatorMaterialInstance->SetScalarParameterValue(
		TEXT("IndicatorOpacity"),
		IndicatorOpacity);
}

void URLParryFeedbackComponent::ShowParrySuccessIndicator()
{
	if (!GetWorld()) { return; }
	bShowingParrySuccessIndicator = true;
	UpdateParryIndicatorColor();
	GetWorld()->GetTimerManager().ClearTimer(ParrySuccessIndicatorTimerHandle);

	if (ParrySuccessIndicatorDuration <= 0.0f)
	{
		ClearParrySuccessIndicator();
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		ParrySuccessIndicatorTimerHandle,
		this,
		&ThisClass::ClearParrySuccessIndicator,
		ParrySuccessIndicatorDuration,
		false);
}

void URLParryFeedbackComponent::ClearParrySuccessIndicator()
{
	if (!GetWorld()) { return; }
	GetWorld()->GetTimerManager().ClearTimer(ParrySuccessIndicatorTimerHandle);
	bShowingParrySuccessIndicator = false;
	UpdateParryIndicatorColor();
}

void URLParryFeedbackComponent::TriggerParryHitStop(bool bPerfectParry, bool bOverdrive)
{
	UWorld* World = GetWorld();
	if (!World || ParryHitStopDuration <= 0.0f || ParryHitStopTimeDilation >= 1.0f)
	{
		return;
	}

	if (!bParryHitStopActive)
	{
		PreviousTimeDilation = UGameplayStatics::GetGlobalTimeDilation(World);
	}
	bParryHitStopActive = true;
	const float EffectiveTimeDilation = ParryHitStopTimeDilation;
	UGameplayStatics::SetGlobalTimeDilation(World, EffectiveTimeDilation);
	const float DurationMultiplier = bOverdrive
		? OverdriveHitStopDurationMultiplier
		: (bPerfectParry ? PerfectHitStopDurationMultiplier : 1.0f);
	const float EffectiveHitStopDuration = ParryHitStopDuration * DurationMultiplier;
	GetWorld()->GetTimerManager().ClearTimer(ParryHitStopTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		ParryHitStopTimerHandle,
		this,
		&ThisClass::RestoreTimeDilation,
		FMath::Max(KINDA_SMALL_NUMBER, EffectiveHitStopDuration * EffectiveTimeDilation),
		false);
}

void URLParryFeedbackComponent::RestoreTimeDilation()
{
	if (!GetWorld()) { return; }
	GetWorld()->GetTimerManager().ClearTimer(ParryHitStopTimerHandle);
	if (!bParryHitStopActive)
	{
		return;
	}

	bParryHitStopActive = false;
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, PreviousTimeDilation);
	}
}

void URLParryFeedbackComponent::PlayParrySwingSound() const
{
	if (!GetOwner() || !ParrySwingSound)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(
		this,
		ParrySwingSound,
		GetOwner()->GetActorLocation(),
		FRotator::ZeroRotator,
		ParrySwingSoundVolume);
}

void URLParryFeedbackComponent::PlayParryImpactSound(
	const FVector& SoundLocation,
	int32 EnhancementLevel,
	bool bPerfectParry)
{
	bool bPerfectSoundPlaying = false;
	if (bPerfectParry && PerfectParrySound && PerfectParrySoundVolume > 0.0f && GetWorld())
	{
		// Restart the result sound instead of stacking long power-up tails.
		if (IsValid(PerfectParryAudioComponent)) { PerfectParryAudioComponent->Stop(); }
		PerfectParryAudioComponent = UGameplayStatics::SpawnSound2D(this, PerfectParrySound, PerfectParrySoundVolume);
		bPerfectSoundPlaying = IsValid(PerfectParryAudioComponent);
	}
	USoundBase* SoundToPlay = ParryImpactSound;
	float VolumeMultiplier = ParryImpactSoundVolume;
	// Retain the legacy array slots: combo 3/5/8 map to enhancement stages 2/3/4.
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

	if (bPerfectSoundPlaying) { VolumeMultiplier *= PerfectImpactVolumeMultiplier; }
	if (!SoundToPlay || VolumeMultiplier <= 0.0f)
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

