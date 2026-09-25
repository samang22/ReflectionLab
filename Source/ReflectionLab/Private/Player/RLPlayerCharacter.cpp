// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/RLPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Combat/RLProjectile.h"
#include "Components/SphereComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ARLPlayerCharacter::ARLPlayerCharacter()
{
	static ConstructorHelpers::FObjectFinder<USoundBase> ParryImpactSoundFinder(
		TEXT("/Game/ReflectionLab/Audio/SFX/Combat/Parry/SC_ParryImpact.SC_ParryImpact"));
	ParryImpactSound = ParryImpactSoundFinder.Object;

	// Set this character to call Tick() every frame. You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 1000.0f;
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
}

// Called when the game starts or when spawned
void ARLPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	ReflectionZone->SetSphereRadius(ReflectionRange);
}

float ARLPlayerCharacter::TakeDamage(
	float DamageAmount,
	const FDamageEvent& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	return 0.f;

	// const float AppliedDamage = Super::TakeDamage(
	// 	DamageAmount,
	// 	DamageEvent,
	// 	EventInstigator,
	// 	DamageCauser);
	//
	//
	// if (AppliedDamage <= 0.0f || IsDead())
	// {
	// 	return 0.0f;
	// }
	//
	// CurrentHealth = FMath::Max(0.0f, CurrentHealth - AppliedDamage);
	// OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	//
	// UE_LOG(
	// 	LogTemp,
	// 	Display,
	// 	TEXT("Player took %.1f damage. Health: %.1f / %.1f"),
	// 	AppliedDamage,
	// 	CurrentHealth,
	// 	MaxHealth);
	//
	// if (IsDead())
	// {
	// 	Die();
	// }
	//
	// return AppliedDamage;
}

void ARLPlayerCharacter::Die_Implementation()
{
	if (bParryAttemptInProgress || bParryActive)
	{
		EndParry(false);
	}

	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);

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
	if (IsDead() || bParryAttemptInProgress || bParryOnCooldown || !GetWorld())
	{
		return;
	}

	if (!ParryMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot start parry: ParryMontage is not assigned."));
		return;
	}

	const float MontageDuration = PlayAnimMontage(ParryMontage);
	if (MontageDuration <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to play the parry montage."));
		return;
	}

	bParryAttemptInProgress = true;

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
	if (IsDead() || !bParryAttemptInProgress || bParryActive)
	{
		return;
	}

	bParryActive = true;
	ParriesThisActivation = 0;
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

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (ARLProjectile* Projectile = Cast<ARLProjectile>(Overlap.GetActor()))
		{
			if (TryParryProjectile(Projectile) &&
				ParriesThisActivation >= MaxReflectionsPerActivation)
			{
				EndParry(true);
				break;
			}
		}
	}
}

bool ARLPlayerCharacter::TryParryProjectile(ARLProjectile* Projectile)
{
	if (!IsValid(Projectile) || Projectile->IsReflected())
	{
		return false;
	}

	const FVector DirectionToProjectile =
		(Projectile->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const FVector ForwardDirection = GetActorForwardVector().GetSafeNormal2D();
	const float MinimumForwardDot =
		FMath::Cos(FMath::DegreesToRadians(ReflectionHalfAngleDegrees));

	if (FVector::DotProduct(ForwardDirection, DirectionToProjectile) < MinimumForwardDot)
	{
		return false;
	}

	const FVector ParryDirection = GetActorForwardVector().GetSafeNormal();
	if (!Projectile->Reflect(this, this, ParryDirection))
	{
		return false;
	}

	if (ParryImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ParryImpactSound,
			Projectile->GetActorLocation());
	}

	++ParriesThisActivation;
	++ParryChainCount;
	OnParryChainChanged.Broadcast(ParryChainCount);

	GetWorldTimerManager().ClearTimer(ParryChainResetTimerHandle);
	if (ParryChainGracePeriod > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			ParryChainResetTimerHandle,
			this,
			&ThisClass::ResetParryChain,
			ParryChainGracePeriod,
			false);
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Projectile parried. Chain: %d"),
		ParryChainCount);
	return true;
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

	const float CooldownDuration = bSucceeded
		? SuccessfulParryCooldown
		: ReflectionCooldown;

	GetWorldTimerManager().ClearTimer(ParryCooldownTimerHandle);
	if (CooldownDuration <= 0.0f)
	{
		bParryOnCooldown = false;
		return;
	}

	bParryOnCooldown = true;
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
}

void ARLPlayerCharacter::ResetParryChain()
{
	GetWorldTimerManager().ClearTimer(ParryChainResetTimerHandle);
	if (ParryChainCount == 0)
	{
		return;
	}

	ParryChainCount = 0;
	OnParryChainChanged.Broadcast(ParryChainCount);
	UE_LOG(LogTemp, Display, TEXT("Parry chain reset."));
}
