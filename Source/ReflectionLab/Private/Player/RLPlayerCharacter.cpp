// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/RLPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLProjectilePoolSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Data/RLParryTuningDataAsset.h"
#include "Data/RLPlayerStatsDataAsset.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Framework/GameMode/RLGameModeBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ARLPlayerCharacter::ARLPlayerCharacter()
{
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
	CameraBoom->TargetArmLength = 1300.0f;
	CameraBoom->SetRelativeRotation(FRotator(-55.0f, 45.0f, 0.0f));
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
}

// Called when the game starts or when spawned
void ARLPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyPlayerStats();
	ApplyParryTuning();
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	ReflectionZone->SetSphereRadius(ReflectionRange);
	if (ParryRangeIndicator && ParryRangeIndicatorMaterial)
	{
		ParryRangeIndicator->SetDecalMaterial(ParryRangeIndicatorMaterial);
		ParryRangeIndicatorMaterialInstance = ParryRangeIndicator->CreateDynamicMaterialInstance();
	}
	UpdateParryRangeIndicator();
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
}

void ARLPlayerCharacter::ApplyPlayerStats()
{
	if (!PlayerStatsData)
	{
		return;
	}

	MaxHealth = FMath::Max(1.0f, PlayerStatsData->MaxHealth);
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
	if (!ParryTuningData)
	{
		return;
	}

	ReflectionCooldown = FMath::Max(0.0f, ParryTuningData->FailedParryCooldown);
	SuccessfulParryCooldown = FMath::Max(0.0f, ParryTuningData->SuccessfulParryCooldown);
	PerfectSplitProjectileCount = FMath::Clamp(
		ParryTuningData->PerfectSplitProjectileCount,
		1,
		8);
	PerfectSplitAngleDegrees = FMath::Clamp(
		ParryTuningData->PerfectSplitAngleDegrees,
		0.0f,
		90.0f);
	PerfectHitStopDurationMultiplier = FMath::Max(
		1.0f,
		ParryTuningData->PerfectHitStopDurationMultiplier);
	BasePierceCount = FMath::Max(0, ParryTuningData->BasePierceCount);
	MaxReflectedSpeedMultiplier = FMath::Max(
		1.0f,
		ParryTuningData->MaxReflectedSpeedMultiplier);
	BaseReflectedProjectileScale = FMath::Max(
		1.0f,
		ParryTuningData->BaseReflectedProjectileScale);
	CloseRangeThreshold = FMath::Max(0.0f, ParryTuningData->CloseRangeThreshold);
	CloseRangePierceCount = FMath::Max(0, ParryTuningData->CloseRangePierceCount);
	CloseRangeProjectileScale = FMath::Max(1.0f, ParryTuningData->CloseRangeProjectileScale);
	ComboSpeedMilestone = FMath::Max(1, ParryTuningData->ComboSpeedMilestone);
	ComboExtraProjectileMilestone = FMath::Max(
		ComboSpeedMilestone,
		ParryTuningData->ComboExtraProjectileMilestone);
	ComboExtraProjectileSpreadAngle = FMath::Clamp(
		ParryTuningData->ComboExtraProjectileSpreadAngle,
		0.0f,
		90.0f);
	OverdriveComboThreshold = FMath::Max(
		ComboExtraProjectileMilestone,
		ParryTuningData->OverdriveComboThreshold);
	OverdriveProjectileCount = FMath::Clamp(ParryTuningData->OverdriveProjectileCount, 1, 16);
	OverdriveSpreadAngleDegrees = FMath::Clamp(
		ParryTuningData->OverdriveSpreadAngleDegrees,
		0.0f,
		180.0f);
	OverdriveProjectileScale = FMath::Max(1.0f, ParryTuningData->OverdriveProjectileScale);
	OverdrivePierceCount = FMath::Max(0, ParryTuningData->OverdrivePierceCount);
	OverdriveHitStopDurationMultiplier = FMath::Max(
		1.0f,
		ParryTuningData->OverdriveHitStopDurationMultiplier);
	ReflectionRange = FMath::Max(1.0f, ParryTuningData->ParryRange);
	PerfectParryOuterBandWidth = FMath::Clamp(
		ParryTuningData->PerfectParryOuterBandWidth,
		0.0f,
		ReflectionRange);
	ReflectionHalfAngleDegrees = FMath::Clamp(
		ParryTuningData->ParryHalfAngleDegrees,
		0.0f,
		180.0f);
	ParryIndicatorIdleOpacity = FMath::Clamp(ParryTuningData->IndicatorIdleOpacity, 0.0f, 1.0f);
	ParryIndicatorActiveOpacity = FMath::Clamp(ParryTuningData->IndicatorActiveOpacity, 0.0f, 1.0f);
	ParryIndicatorSuccessOpacity = FMath::Clamp(ParryTuningData->IndicatorSuccessOpacity, 0.0f, 1.0f);
	ParryIndicatorUnavailableOpacity = FMath::Clamp(
		ParryTuningData->IndicatorUnavailableOpacity,
		0.0f,
		1.0f);
	ParryImpactSoundVolume = FMath::Clamp(ParryTuningData->ImpactSoundVolume, 0.0f, 1.0f);
	ParrySwingSound = ParryTuningData->SwingSound;
	ParrySwingSoundVolume = FMath::Clamp(ParryTuningData->SwingSoundVolume, 0.0f, 1.0f);
	ParryComboImpactSounds = ParryTuningData->ComboImpactSounds;
	ParryComboImpactSoundVolumes.Reset(ParryTuningData->ComboImpactSoundVolumes.Num());
	for (const float Volume : ParryTuningData->ComboImpactSoundVolumes)
	{
		ParryComboImpactSoundVolumes.Add(FMath::Clamp(Volume, 0.0f, 2.0f));
	}
	ParryHitStopDuration = FMath::Max(0.0f, ParryTuningData->HitStopDuration);
	ParryHitStopTimeDilation = FMath::Clamp(ParryTuningData->HitStopTimeDilation, 0.01f, 1.0f);
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
	ParryRangeIndicator->SetDecalMaterial(
		ParryRangeIndicatorMaterialInstance
			? ParryRangeIndicatorMaterialInstance
			: ParryRangeIndicatorMaterial);

	if (ParryRangeIndicatorMaterialInstance)
	{
		ParryRangeIndicatorMaterialInstance->SetScalarParameterValue(
			TEXT("ConeSlope"),
			FMath::Tan(FMath::DegreesToRadians(ReflectionHalfAngleDegrees)));
		const float PerfectBandInnerRadiusUv = 0.5f *
			(1.0f - PerfectParryOuterBandWidth / ReflectionRange);
		ParryRangeIndicatorMaterialInstance->SetScalarParameterValue(
			TEXT("PerfectBandInnerRadiusUV"),
			PerfectBandInnerRadiusUv);
		UpdateParryIndicatorColor();
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
	UGameplayStatics::SetGlobalTimeDilation(World, ParryHitStopTimeDilation);
	const float DurationMultiplier = bOverdrive
		? OverdriveHitStopDurationMultiplier
		: (bPerfectParry ? PerfectHitStopDurationMultiplier : 1.0f);
	const float EffectiveHitStopDuration = ParryHitStopDuration * DurationMultiplier;
	GetWorldTimerManager().ClearTimer(ParryHitStopTimerHandle);
	GetWorldTimerManager().SetTimer(
		ParryHitStopTimerHandle,
		this,
		&ThisClass::RestoreTimeDilation,
		FMath::Max(KINDA_SMALL_NUMBER, EffectiveHitStopDuration * ParryHitStopTimeDilation),
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
	if (DamageAmount <= 0.0f || IsDead() || bHitRecoveryActive)
	{
		return 0.0f;
	}

	const float AppliedDamage = Super::TakeDamage(
		DamageAmount,
		DamageEvent,
		EventInstigator,
		DamageCauser);
	if (AppliedDamage <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - AppliedDamage);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
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
		CurrentHealth,
		MaxHealth);

	if (IsDead())
	{
		Die();
	}
	else
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

void ARLPlayerCharacter::StartParry()
{
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
	const bool bOverdrive = ResultingCombo >= OverdriveComboThreshold;
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
		PlayParryImpactSound(ParrySoundLocation, ResultingCombo);
		TriggerParryHitStop(bPerfectParry, bOverdrive);
		EndParry(true);
		if (bOverdrive)
		{
			ConsumeParryCombo();
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
	const bool bOverdrive = ResultingCombo >= OverdriveComboThreshold;
	const bool bMaximumSpeed = bCloseRangeParry ||
		ResultingCombo >= ComboSpeedMilestone ||
		bOverdrive;

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
	if (ResultingCombo >= ComboExtraProjectileMilestone)
	{
		++SplitCount;
		if (SplitSpreadAngle <= 0.0f)
		{
			SplitSpreadAngle = ComboExtraProjectileSpreadAngle;
		}
	}
	if (bOverdrive)
	{
		SplitCount = OverdriveProjectileCount;
		SplitSpreadAngle = OverdriveSpreadAngleDegrees;
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
	int32 ResultingCombo) const
{
	USoundBase* SoundToPlay = ParryImpactSound;
	float VolumeMultiplier = ParryImpactSoundVolume;
	const int32 ComboSoundIndex = ResultingCombo - 1;
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
	OnParryChainChanged.Broadcast(ParryChainCount);
	OnParryComboChanged.Broadcast(
		ParryChainCount,
		FMath::Max(1, MultiParryCount),
		bPerfectParry,
		bCloseRangeParry);

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
		return;
	}

	ParryChainCount = 0;
	OnParryChainChanged.Broadcast(ParryChainCount);
	OnParryComboChanged.Broadcast(0, 0, false, false);
	UE_LOG(LogTemp, Display, TEXT("Parry chain reset."));
}

void ARLPlayerCharacter::ConsumeParryCombo()
{
	if (ParryChainCount == 0)
	{
		return;
	}

	ParryChainCount = 0;
	OnParryChainChanged.Broadcast(ParryChainCount);
	OnParryComboChanged.Broadcast(0, 0, false, false);
	UE_LOG(LogTemp, Display, TEXT("Parry combo consumed by overdrive."));
}
