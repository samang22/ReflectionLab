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
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

URLMirrorArenaComponent::URLMirrorArenaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void URLMirrorArenaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Settings && CameraBoom.IsValid())
	{
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, Settings->Mirror.MirrorCameraArmLength, DeltaTime, 3);
		CameraBoom->SetWorldLocation(FMath::VInterpTo(CameraBoom->GetComponentLocation(), CameraAnchor, DeltaTime, 3));
	}
}

void URLMirrorArenaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetArena();
	Super::EndPlay(EndPlayReason);
}

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
	if (Settings->SurfaceMaterial) { Mesh->SetMaterial(0,Settings->SurfaceMaterial); }
	if (auto* Material = Mesh->CreateDynamicMaterialInstance(0))
	{ Material->SetVectorParameterValue(TEXT("SurfaceColor"),bPawnOnly ? FLinearColor(0.1f,0.55f,0.6f) : FLinearColor(0.3f,0.32f,0.35f)); }
	// Keep the overlapping gray cover above cyan rails to avoid coplanar tops.
	const float VisualHeight = bPawnOnly ? Settings->Mirror.LaneBarrierVisualHeight
		: FMath::Max(Settings->Mirror.ShotBarrierVisualHeight,Settings->Mirror.LaneBarrierVisualHeight+5.0f);
	Mesh->SetRelativeLocation(FVector(0,0,(VisualHeight-Size.Z)*0.5f));
	Mesh->SetRelativeScale3D(FVector(Size.X,Size.Y,VisualHeight)/100);
	return true;
}

bool URLMirrorArenaComponent::Configure(URLArenaExperimentDataAsset* Data)
{
	ResetArena();
	if (!Data || !FMath::IsFinite(Data->Mirror.LaneHalfWidth) || Data->Mirror.LaneHalfWidth < 300 || Data->Mirror.LaneHalfWidth > 600 ||
		!FMath::IsFinite(Data->Mirror.LaneHalfLength) || Data->Mirror.LaneHalfLength < 150 || Data->Mirror.LaneHalfLength > 350 ||
		!FMath::IsFinite(Data->Mirror.DiagonalMirrorHalfLength) || Data->Mirror.DiagonalMirrorHalfLength < 400 || Data->Mirror.DiagonalMirrorHalfLength > 1200 ||
		Data->Mirror.DiagonalMirrorCenter.ContainsNaN() || Data->Mirror.DiagonalMirrorCenter.X < 800 || Data->Mirror.DiagonalMirrorCenter.Y < 650 ||
		Data->Mirror.DiagonalMirrorCenter.X > 1200 || Data->Mirror.DiagonalMirrorCenter.Y > 1200 ||
		!FMath::IsFinite(Data->Mirror.MirrorCameraArmLength) || Data->Mirror.MirrorCameraArmLength < 2000 || Data->Mirror.MirrorCameraArmLength > 6000 ||
		!FMath::IsFinite(Data->Mirror.LaneBarrierVisualHeight) || Data->Mirror.LaneBarrierVisualHeight < 10 || Data->Mirror.LaneBarrierVisualHeight > 100 ||
		!FMath::IsFinite(Data->Mirror.ShotBarrierVisualHeight) || Data->Mirror.ShotBarrierVisualHeight < 20 || Data->Mirror.ShotBarrierVisualHeight > 200 ||
		!FMath::IsFinite(Data->Mirror.ShotBarrierLength) || Data->Mirror.ShotBarrierLength < 100 || Data->Mirror.ShotBarrierLength > 1200 ||
		Data->Mirror.MaxMirrorBounces < 1 || Data->Mirror.MaxMirrorBounces > 8) { return false; }
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
	const float X = Data->Mirror.DiagonalMirrorCenter.X;
	const float Y = Data->Mirror.DiagonalMirrorCenter.Y;
	Mirror = GetWorld()->SpawnActor<ARLProjectileMirrorWall>(FVector(-X,Y,100),FRotator(0,45,0));
	if (!Mirror) { ResetArena(); return false; }
	Mirror->Configure(Data->Mirror.DiagonalMirrorHalfLength,Data->SurfaceMaterial,Data->Mirror.MaxMirrorBounces);
	const FVector Positions[] = {FVector(X,Y,100), FVector(-X,-Y,100), FVector(X,-Y,100)};
	const float Yaws[] = {-45,135,-135};
	for (int32 Index=0;Index<3;++Index)
	{
		auto* Extra = GetWorld()->SpawnActor<ARLProjectileMirrorWall>(Positions[Index],FRotator(0,Yaws[Index],0));
		if (!Extra) { ResetArena(); return false; }
		ExtraMirrors.Add(Extra);
		Extra->Configure(Data->Mirror.DiagonalMirrorHalfLength,Data->SurfaceMaterial,Data->Mirror.MaxMirrorBounces);
	}
	for (float Side : {-1.0f,1.0f})
	{
		if (!AddBarrier(FVector(Side*(Data->Mirror.LaneHalfWidth+100),0,100),FVector(80,Data->Mirror.ShotBarrierLength,200),false) ||
			!AddBarrier(FVector(Side*(Data->Mirror.LaneHalfWidth+100),0,70),FVector(50,(Data->Mirror.LaneHalfLength+70)*2,140),true) ||
			!AddBarrier(FVector(0,Side*(Data->Mirror.LaneHalfLength+70),70),FVector((Data->Mirror.LaneHalfWidth+100)*2,50,140),true))
		{ ResetArena(); return false; }
	}
	if (auto* Player = UGameplayStatics::GetPlayerPawn(this,0))
	{
		if (auto* Boom = Player->FindComponentByClass<USpringArmComponent>())
		{
			CameraBoom = Boom;
			OriginalArmLength = Boom->TargetArmLength;
			OriginalBoomLocation = Boom->GetRelativeLocation();
			bOriginalAbsoluteLocation = Boom->IsUsingAbsoluteLocation();
			CameraAnchor = FVector(0,0,Boom->GetComponentLocation().Z);
			Boom->SetUsingAbsoluteLocation(true);
			SetComponentTickEnabled(true);
		}
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
	if (auto* Attack = Enemy->FindComponentByClass<URLEnemyAttackComponent>())
	{
		const FVector Location = Enemy->GetActorLocation();
		ARLProjectileMirrorWall* AimWall = Location.X < 0 ? (Location.Y >= 0 ? Mirror.Get() : ExtraMirrors[1].Get())
			: (Location.Y >= 0 ? ExtraMirrors[0].Get() : ExtraMirrors[2].Get());
		Attack->SetAimMirror(AimWall);
	}
}

void URLMirrorArenaComponent::ResetArena()
{
	SetComponentTickEnabled(false);
	if (CameraBoom.IsValid())
	{
		CameraBoom->TargetArmLength = OriginalArmLength;
		CameraBoom->SetUsingAbsoluteLocation(bOriginalAbsoluteLocation);
		CameraBoom->SetRelativeLocation(OriginalBoomLocation);
	}
	CameraBoom.Reset();
	for (const auto& Entry : Enemies)
	{
		if (auto* Enemy = Entry.Get())
		{
			if (auto* Attack = Enemy->FindComponentByClass<URLEnemyAttackComponent>()) { Attack->SetAimMirror(nullptr); }
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
