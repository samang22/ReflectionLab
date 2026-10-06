#include "Enemies/Components/RLEnemyMovementComponent.h"

#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/RLEnemyAIController.h"
#include "Data/RLEnemyMovementDataAsset.h"
#include "EngineUtils.h"
#include "Framework/GameMode/RLGameModeBase.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

URLEnemyMovementComponent::URLEnemyMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URLEnemyMovementComponent::InitializeMovement()
{
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy) { return; }
	// Enforce facing ownership even when a blueprint saved other defaults.
	Enemy->bUseControllerRotationPitch = false;
	Enemy->bUseControllerRotationYaw = false;
	Enemy->bUseControllerRotationRoll = false;
	if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = false;
	}
	SetComponentTickEnabled(Enemy->IsPoolActive());
	if (!Enemy->GetController()) { Enemy->SpawnDefaultController(); }
	SelectMovingEnemy();
	UpdateFacingPlayer();
}

void URLEnemyMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const ARLEnemyCharacter* SpawningEnemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (SpawningEnemy && SpawningEnemy->IsSpawning()) { return; }
	UpdateFacingPlayer();
	if (!IsRepositioning()) { return; }
	RepositionTimeRemaining -= DeltaTime;
	if (bWaitingToMove && RepositionTimeRemaining <= 0.0f)
	{
		bWaitingToMove = false;
		RequestReposition();
	}
	else if (bMoving)
	{
		const APawn* EnemyPawn = Cast<APawn>(GetOwner());
		const AAIController* AI = EnemyPawn ? Cast<AAIController>(EnemyPawn->GetController()) : nullptr;
		if (!AI || AI->GetMoveStatus() != EPathFollowingStatus::Moving || RepositionTimeRemaining <= 0.0f)
		{
			CancelReposition();
		}
	}
}

void URLEnemyMovementComponent::UpdateFacingPlayer()
{
	const ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy || !Enemy->IsPoolActive()) { return; }
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!IsValid(PlayerPawn)) { return; }
	const FVector ToPlayer = (PlayerPawn->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
	if (!ToPlayer.IsNearlyZero())
	{
		GetOwner()->SetActorRotation(FRotator(0.0f, ToPlayer.Rotation().Yaw, 0.0f));
	}
}

void URLEnemyMovementComponent::SetTutorialMovementLocked(bool bLocked)
{
	bTutorialMovementLocked = bLocked;
	CancelReposition();
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy || !Enemy->IsPoolActive()) { return; }
	// Lock translation, not facing: tutorial enemies still face the player.
	SetComponentTickEnabled(true);
	if (AController* Controller = Enemy->GetController()) { Controller->StopMovement(); }
	if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		if (bTutorialMovementLocked) { Movement->DisableMovement(); }
		else if (!Enemy->IsSpawning()) { Movement->SetDefaultMovementMode(); }
	}
}

void URLEnemyMovementComponent::ActivateForPool()
{
	bTutorialMovementLocked = false;
	InitializeMovement();
	SetTutorialMovementLocked(false);
}

void URLEnemyMovementComponent::ResumeAfterSpawn()
{
	SetTutorialMovementLocked(bTutorialMovementLocked);
}

void URLEnemyMovementComponent::DeactivateForPool()
{
	CancelReposition();
	bTutorialMovementLocked = false;
	SetComponentTickEnabled(false);
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	if (!Enemy) { return; }
	if (AController* Controller = Enemy->GetController()) { Controller->StopMovement(); }
	if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
}

void URLEnemyMovementComponent::SelectMovingEnemy()
{
	const URLEnemyMovementDataAsset* Settings = MovementSettings
		? MovementSettings.Get() : GetDefault<URLEnemyMovementDataAsset>();
	bSelectedToMove = FMath::FRand() < FMath::Clamp(Settings->MovingEnemyRatio, 0.0f, 1.0f);
}

void URLEnemyMovementComponent::BeginReposition()
{
	const ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	const ARLGameModeBase* Mode = Cast<ARLGameModeBase>(UGameplayStatics::GetGameMode(this));
	if (!Enemy || !Enemy->IsPoolActive() || !bSelectedToMove || bTutorialMovementLocked ||
		(Mode && Mode->IsCurrentRoundTutorial())) { return; }
	const URLEnemyMovementDataAsset* Settings = MovementSettings
		? MovementSettings.Get() : GetDefault<URLEnemyMovementDataAsset>();
	bWaitingToMove = true;
	RepositionTimeRemaining = FMath::Max(0.0f, Settings->PostAttackDelay);
}

void URLEnemyMovementComponent::CancelReposition()
{
	bWaitingToMove = false;
	bMoving = false;
	RepositionTimeRemaining = 0.0f;
	APawn* EnemyPawn = Cast<APawn>(GetOwner());
	if (EnemyPawn && EnemyPawn->GetController()) { EnemyPawn->GetController()->StopMovement(); }
}

void URLEnemyMovementComponent::RequestReposition()
{
	ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	AAIController* AI = Enemy ? Cast<AAIController>(Enemy->GetController()) : nullptr;
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Enemy || !PlayerPawn || !AI || !Nav || bTutorialMovementLocked || !Enemy->IsPoolActive()) { return; }
	const URLEnemyMovementDataAsset* Settings = MovementSettings
		? MovementSettings.Get() : GetDefault<URLEnemyMovementDataAsset>();
	for (int32 Attempt = 0; Attempt < 16; ++Attempt)
	{
		const float Angle = FMath::FRandRange(-PI, PI);
		const float Radius = FMath::FRandRange(80.0f, FMath::Max(80.0f, Settings->MaxRepositionDistance));
		const FVector Candidate = Enemy->GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Radius;
		FNavLocation Destination;
		if (!Nav->ProjectPointToNavigation(Candidate, Destination, FVector(100.0f, 100.0f, 250.0f))) { continue; }
		const float PlayerDistance = FVector::Dist2D(Destination.Location, PlayerPawn->GetActorLocation());
		if (PlayerDistance < Settings->MinPlayerDistance || PlayerDistance > FMath::Max(Settings->MinPlayerDistance, Settings->MaxPlayerDistance)) { continue; }
		bool bOccupied = false;
		for (TActorIterator<ARLEnemyCharacter> It(GetWorld()); It; ++It)
		{
			if (*It != Enemy && It->IsPoolActive() && FVector::Dist2D(It->GetActorLocation(), Destination.Location) < Settings->EnemySeparation)
			{
				bOccupied = true;
				break;
			}
		}
		if (bOccupied) { continue; }
		Enemy->GetCharacterMovement()->MaxWalkSpeed = FMath::Max(1.0f, Settings->MoveSpeed);
		const EPathFollowingRequestResult::Type Result = AI->MoveToLocation(Destination.Location, 35.0f, true, true, false, true, nullptr, false);
		if (Result == EPathFollowingRequestResult::RequestSuccessful)
		{
			bMoving = true;
			RepositionTimeRemaining = FMath::Max(0.1f, Settings->MoveTimeout);
			return;
		}
	}
}

