#include "Framework/RLMirrorArenaComponent.h"
#include "Data/RLArenaExperimentDataAsset.h"
#include "Combat/RLProjectileMirrorWall.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/Components/RLEnemyAttackComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

bool URLMirrorArenaComponent::AddBarrier(const FVector& Location, const FVector& Size, bool bPawnOnly)
{
	auto* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
	if (!Actor) { return false; }
	Barriers.Add(Actor);
	auto* Mesh = Actor->GetStaticMeshComponent();
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetCollisionProfileName(TEXT("NoCollision"));
	// Separate low visual geometry from the full-height gameplay collision.
	auto* Collision = NewObject<UBoxComponent>(Actor,TEXT("LaneCollision"));
	Actor->AddInstanceComponent(Collision);
	Collision->SetMobility(EComponentMobility::Movable);
	Collision->SetBoxExtent(Size * 0.5f);
	Collision->SetCanEverAffectNavigation(false);
	Actor->SetRootComponent(Collision);
	Collision->RegisterComponent();
	Actor->SetActorLocation(Location);
	Mesh->AttachToComponent(Collision,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	Collision->SetCollisionProfileName(bPawnOnly ? TEXT("NoCollision") : TEXT("BlockAll"));
	if (bPawnOnly)
	{
		// Cyan rails constrain players, but do not consume incoming or returned shots.
		Collision->SetCollisionObjectType(ECC_WorldDynamic);
		Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Collision->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);
	}
	if (Settings->ShutterMaterial) { Mesh->SetMaterial(0,Settings->ShutterMaterial); }
	if (auto* Material = Mesh->CreateDynamicMaterialInstance(0))
	{ Material->SetVectorParameterValue(TEXT("SurfaceColor"),bPawnOnly ? FLinearColor(0.1f,0.55f,0.6f) : FLinearColor(0.3f,0.32f,0.35f)); }
	Mesh->SetRelativeLocation(FVector(0,0,(Settings->LaneBarrierVisualHeight-Size.Z)*0.5f));
	Mesh->SetRelativeScale3D(FVector(Size.X,Size.Y,Settings->LaneBarrierVisualHeight)/100);
	return true;
}

bool URLMirrorArenaComponent::Configure(URLArenaExperimentDataAsset* Data)
{
	ResetArena();
	if (!Data || !FMath::IsFinite(Data->LaneHalfWidth) || Data->LaneHalfWidth < 300 || Data->LaneHalfWidth > 600 ||
		!FMath::IsFinite(Data->LaneHalfLength) || Data->LaneHalfLength < 150 || Data->LaneHalfLength > 350 ||
		!FMath::IsFinite(Data->MirrorDistance) || Data->MirrorDistance < 1550 || Data->MirrorDistance > 1800 ||
		!FMath::IsFinite(Data->MirrorHalfWidth) || Data->MirrorHalfWidth < 1500 || Data->MirrorHalfWidth > 1900 ||
		!FMath::IsFinite(Data->LaneBarrierVisualHeight) || Data->LaneBarrierVisualHeight < 10 || Data->LaneBarrierVisualHeight > 100 ||
		Data->MaxMirrorBounces < 1 || Data->MaxMirrorBounces > 8) { return false; }
	Settings = Data;
	// Move an out-of-lane player before constructing colliders, never inside a wall.
	if (auto* Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this,0)))
	{
		FVector Safe(0,0,Player->GetActorLocation().Z);
		if (!GetWorld()->FindTeleportSpot(Player,Safe,Player->GetActorRotation())) { ResetArena(); return false; }
		Player->GetCharacterMovement()->StopMovementImmediately();
		Player->SetBase(static_cast<FMovementBaseInterfaceData*>(nullptr));
		Player->SetActorLocation(Safe,false,nullptr,ETeleportType::TeleportPhysics);
	}
	Mirror = GetWorld()->SpawnActor<ARLProjectileMirrorWall>(FVector(0,Data->MirrorDistance,100),FRotator::ZeroRotator);
	if (!Mirror) { ResetArena(); return false; }
	Mirror->Configure(Data->MirrorHalfWidth,Data->ShutterMaterial,Data->MaxMirrorBounces);
	const FVector Positions[] = {FVector(0,-Data->MirrorDistance,100), FVector(-Data->MirrorHalfWidth,0,100), FVector(Data->MirrorHalfWidth,0,100)};
	const float Yaws[] = {180,90,-90};
	for (int32 Index=0;Index<3;++Index)
	{
		auto* Extra = GetWorld()->SpawnActor<ARLProjectileMirrorWall>(Positions[Index],FRotator(0,Yaws[Index],0));
		if (!Extra) { ResetArena(); return false; }
		ExtraMirrors.Add(Extra);
		Extra->Configure(Index==0 ? Data->MirrorHalfWidth : Data->MirrorDistance,Data->ShutterMaterial,Data->MaxMirrorBounces);
	}
	for (float Side : {-1.0f,1.0f})
	{
		if (!AddBarrier(FVector(Side*(Data->LaneHalfWidth+100),0,100),FVector(80,1000,200),false) ||
			!AddBarrier(FVector(0,Side*(Data->LaneHalfLength+70),70),FVector((Data->LaneHalfWidth+100)*2,50,140),true))
		{ ResetArena(); return false; }
	}
	return true;
}

bool URLMirrorArenaComponent::FindSpawn(int32 Index, FTransform& Transform) const
{
	if (!Settings) { return false; }
	const int32 TeamIndex = Index/2;
	const FVector Location((Index%2 ? 1 : -1)*(1100+180*(TeamIndex/4)), -300+200*(TeamIndex%4),96);
	if (FMath::Abs(Location.X)>1800 || GetWorld()->OverlapBlockingTestByChannel(Location,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,96))) { return false; }
	Transform = FTransform(FRotator::ZeroRotator,Location);
	return true;
}

void URLMirrorArenaComponent::RegisterEnemy(ARLEnemyCharacter* Enemy)
{
	if (!Settings || !Enemy) { return; }
	Enemies.Add(Enemy);
	Enemy->SetTutorialMovementLocked(true);
	Enemy->FindComponentByClass<URLEnemyAttackComponent>()->SetAimMirror(Mirror);
}

void URLMirrorArenaComponent::ResetArena()
{
	for (const auto& Entry : Enemies)
	{
		if (auto* Enemy = Entry.Get())
		{
			Enemy->FindComponentByClass<URLEnemyAttackComponent>()->SetAimMirror(nullptr);
			Enemy->SetTutorialMovementLocked(false);
		}
	}
	Enemies.Empty();
	for (AStaticMeshActor* Actor : Barriers) { if (IsValid(Actor)) { Actor->Destroy(); } }
	Barriers.Empty();
	for (ARLProjectileMirrorWall* Extra : ExtraMirrors) { if (IsValid(Extra)) { Extra->Destroy(); } }
	ExtraMirrors.Empty();
	if (IsValid(Mirror)) { Mirror->Destroy(); } Mirror = nullptr; Settings = nullptr;
}

FText URLMirrorArenaComponent::GetStatusText() const
{
	return Settings ? FText::FromString(Settings->DisplayName.ToString()+TEXT("\nBLUE WALL REFLECTS SHOTS - AIM RETURNS VIA THE MIRROR")) : FText::GetEmpty();
}
