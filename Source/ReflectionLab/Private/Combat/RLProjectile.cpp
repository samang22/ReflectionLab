#include "Combat/RLProjectile.h"

#include "Combat/RLProjectilePoolSubsystem.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Enemies/RLEnemyCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ARLProjectile::ARLProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(16.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetCanEverAffectNavigation(false);
	CollisionComponent->OnComponentHit.AddDynamic(this, &ThisClass::HandleProjectileHit);
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(
		this,
		&ThisClass::HandleProjectileOverlap);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetCanEverAffectNavigation(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bAutoActivate = false;

}

void ARLProjectile::BeginPlay()
{
	Super::BeginPlay();

	DeactivateForPool();
}

void ARLProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void ARLProjectile::ActivateProjectile(
	const FTransform& SpawnTransform,
	AActor* NewOwner,
	APawn* NewInstigator)
{
	bIsReflected = false;
	SetOwner(NewOwner);
	SetInstigator(NewInstigator);
	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);

	CollisionComponent->ClearMoveIgnoreActors();
	if (NewOwner)
	{
		CollisionComponent->IgnoreActorWhenMoving(NewOwner, true);

		if (UPrimitiveComponent* OwnerRootComponent =
			Cast<UPrimitiveComponent>(NewOwner->GetRootComponent()))
		{
			OwnerRootComponent->IgnoreActorWhenMoving(this, true);
		}
	}

	SetActorHiddenInGame(false);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->Activate(true);
	ProjectileMovement->Velocity =
		GetActorForwardVector() * ProjectileSpeed;
	ProjectileMovement->UpdateComponentVelocity();

	bIsActive = true;
	GetWorldTimerManager().SetTimer(
		LifetimeTimerHandle,
		this,
		&ThisClass::ReturnToPool,
		FMath::Max(0.1f, LifeSeconds),
		false);
}

bool ARLProjectile::Reflect(
	AActor* NewOwner,
	APawn* NewInstigator,
	const FVector& NewDirection)
{
	if (!bIsActive || !NewOwner || !NewInstigator || NewDirection.IsNearlyZero())
	{
		return false;
	}

	if (AActor* PreviousOwner = GetOwner())
	{
		if (UPrimitiveComponent* PreviousOwnerRoot =
			Cast<UPrimitiveComponent>(PreviousOwner->GetRootComponent()))
		{
			PreviousOwnerRoot->IgnoreActorWhenMoving(this, false);
		}
	}

	SetOwner(NewOwner);
	SetInstigator(NewInstigator);

	CollisionComponent->ClearMoveIgnoreActors();
	CollisionComponent->IgnoreActorWhenMoving(NewOwner, true);
	if (UPrimitiveComponent* NewOwnerRoot =
		Cast<UPrimitiveComponent>(NewOwner->GetRootComponent()))
	{
		NewOwnerRoot->IgnoreActorWhenMoving(this, true);
	}

	const FVector ReflectedDirection = NewDirection.GetSafeNormal();
	SetActorRotation(ReflectedDirection.Rotation());
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->Velocity = ReflectedDirection * ProjectileSpeed;
	ProjectileMovement->Activate(true);
	ProjectileMovement->UpdateComponentVelocity();
	bIsReflected = true;

	return true;
}

void ARLProjectile::ReturnToPool()
{
	if (!bIsActive)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (URLProjectilePoolSubsystem* PoolSubsystem =
			World->GetSubsystem<URLProjectilePoolSubsystem>())
		{
			PoolSubsystem->ReleaseProjectile(this);
			return;
		}
	}

	Destroy();
}

void ARLProjectile::DeactivateForPool()
{
	bIsActive = false;
	bIsReflected = false;
	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);

	if (AActor* OwningActor = GetOwner())
	{
		if (UPrimitiveComponent* OwnerRootComponent =
			Cast<UPrimitiveComponent>(OwningActor->GetRootComponent()))
		{
			OwnerRootComponent->IgnoreActorWhenMoving(this, false);
		}
	}

	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Deactivate();
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionComponent->ClearMoveIgnoreActors();

	SetActorHiddenInGame(true);
	SetOwner(nullptr);
	SetInstigator(nullptr);
}

void ARLProjectile::HandleProjectileHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!bIsActive || ShouldIgnoreActor(OtherActor))
	{
		return;
	}

	ApplyDamageAndReturn(OtherActor);
}

void ARLProjectile::HandleProjectileOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bIsActive || ShouldIgnoreActor(OtherActor))
	{
		return;
	}

	ApplyDamageAndReturn(OtherActor);
}

bool ARLProjectile::ShouldIgnoreActor(const AActor* OtherActor) const
{
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner())
	{
		return true;
	}

	const APawn* ProjectileInstigator = GetInstigator();
	return ProjectileInstigator &&
		ProjectileInstigator->IsA<ARLEnemyCharacter>() &&
		OtherActor->IsA<ARLEnemyCharacter>();
}

void ARLProjectile::ApplyDamageAndReturn(AActor* OtherActor)
{
	UGameplayStatics::ApplyDamage(
		OtherActor,
		DamageAmount,
		GetInstigatorController(),
		this,
		UDamageType::StaticClass());

	ReturnToPool();
}

