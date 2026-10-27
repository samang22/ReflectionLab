#include "Framework/RLBallisticsExperimentComponent.h"
#include "Data/RLArenaExperimentDataAsset.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLProjectileFieldResponseComponent.h"
#include "Combat/RLProjectileVisualComponent.h"
#include "Combat/RLAccelerationBarrier.h"
#include "Enemies/RLEnemyCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

bool URLBallisticsExperimentComponent::AddPlate(const FVector& Position, const FVector& Scale, float Yaw, const FLinearColor& Color)
{
	auto* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Position, FRotator(0,Yaw,0));
	if (!Actor) { return false; }
	Visuals.Add(Actor);
	auto* Mesh = Actor->GetStaticMeshComponent();
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Mesh->SetCollisionProfileName(TEXT("NoCollision"));
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetCastShadow(false);
	if (Settings->SurfaceMaterial) { Mesh->SetMaterial(0, Settings->SurfaceMaterial); }
	if (auto* Material = Mesh->CreateDynamicMaterialInstance(0)) { Material->SetVectorParameterValue(TEXT("SurfaceColor"), Color); }
	Actor->SetActorScale3D(Scale);
	return true;
}

bool URLBallisticsExperimentComponent::Configure(URLArenaExperimentDataAsset* Data)
{
	ResetExperiment();
	if (!Data || Data->FieldLayout.FieldCenters.IsEmpty() || Data->FieldLayout.FieldHalfExtent.ContainsNaN() || Data->FieldLayout.FieldHalfExtent.GetMin() <= 0) { return false; }
	if (Data->Type == ERLArenaExperimentType::AccelerationField)
	{
		if (!FMath::IsFinite(Data->Acceleration.FieldSpeedMultiplier) || Data->Acceleration.FieldSpeedMultiplier < 1 || Data->Acceleration.FieldSpeedMultiplier > 3 ||
			!FMath::IsFinite(Data->Acceleration.BaselineHostileSpeedMultiplier) || Data->Acceleration.BaselineHostileSpeedMultiplier < 0.1f || Data->Acceleration.BaselineHostileSpeedMultiplier > 1) { return false; }
		if (Data->Acceleration.bEnableReturnBarriers)
		{
			const auto& BarrierSize = Data->Acceleration.ReturnBarrierSize;
			if (Data->Acceleration.ReturnBarrierCenters.Num() != 2 || BarrierSize.ContainsNaN() || BarrierSize.GetMin() <= 0 ||
				Data->Acceleration.ReturnBarrierCenters[0].ContainsNaN() || Data->Acceleration.ReturnBarrierCenters[1].ContainsNaN() ||
				Data->Acceleration.ReturnBarrierCenters[0].X >= -BarrierSize.X || Data->Acceleration.ReturnBarrierCenters[1].X <= BarrierSize.X) { return false; }
		}
	}
	else if (Data->Type == ERLArenaExperimentType::RefractionGate)
	{
		if (!FMath::IsFinite(Data->Refraction.RefractionAngleDegrees) || FMath::Abs(Data->Refraction.RefractionAngleDegrees) > 80) { return false; }
	}
	else { return false; }
	for (const auto& Center : Data->FieldLayout.FieldCenters) { if (Center.ContainsNaN()) { return false; } }
	Settings = Data;
	const bool bAcceleration = Data->Type == ERLArenaExperimentType::AccelerationField;
	if (bAcceleration && Data->Acceleration.bEnableReturnBarriers)
	{
		if (auto* Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			FVector Safe(0, 0, Player->GetActorLocation().Z);
			if (!GetWorld()->FindTeleportSpot(Player, Safe, Player->GetActorRotation())) { ResetExperiment(); return false; }
			Player->GetCharacterMovement()->StopMovementImmediately();
			Player->SetBase(static_cast<FMovementBaseInterfaceData*>(nullptr));
			Player->SetActorLocation(Safe, false, nullptr, ETeleportType::TeleportPhysics);
		}
		for (const auto& Center : Data->Acceleration.ReturnBarrierCenters)
		{
			auto* Barrier = GetWorld()->SpawnActor<ARLAccelerationBarrier>(FVector(Center.X, Center.Y, Data->Acceleration.ReturnBarrierSize.Z * 0.5f), FRotator::ZeroRotator);
			if (!Barrier) { ResetExperiment(); return false; }
			Barriers.Add(Barrier);
			Barrier->Configure(Data->Acceleration.ReturnBarrierSize, Data->SurfaceMaterial, this);
		}
	}
	const FLinearColor Color = bAcceleration ? FLinearColor(0.65f,0.35f,0.03f) : FLinearColor(0.12f,0.35f,0.7f);
	for (const auto& Center : Data->FieldLayout.FieldCenters)
	{
		const FVector Position(Center.X, Center.Y, 1);
		const FVector2D Extent = Data->FieldLayout.FieldHalfExtent;
		// Outline only: preserve floor readability and avoid a bright full-area overlay.
		for (float Side : {-1.0f, 1.0f})
		{
			if (!AddPlate(Position + FVector(Side*Extent.X,0,0), FVector(0.06f,Extent.Y/50,0.01f),0,Color) ||
				!AddPlate(Position + FVector(0,Side*Extent.Y,0), FVector(Extent.X/50,0.06f,0.01f),0,Color)) { ResetExperiment(); return false; }
		}
		// Chevron-shaped floor marks indicate acceleration or the authored turn direction.
		for (float Side : {-1.0f, 1.0f})
		{
			if (!AddPlate(Position + FVector(0,Side*35,1), FVector(0.9f,0.06f,0.01f),
				bAcceleration ? Side*40 : Data->Refraction.RefractionAngleDegrees + Side*40, Color)) { ResetExperiment(); return false; }
		}
	}
	// Capture existing active shots so a fast crossing on the first frame is not lost.
	Advance();
	return true;
}

void URLBallisticsExperimentComponent::Advance()
{
	if (!Settings) { return; }
	for (auto It = Transits.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || !It.Key()->IsPoolActive() || It.Key()->IsFadingOut()) { It.RemoveCurrent(); }
	}
	for (TActorIterator<ARLProjectile> It(GetWorld()); It; ++It)
	{
		ProcessProjectile(*It, It->GetActorLocation());
	}
}

void URLBallisticsExperimentComponent::ProcessProjectile(ARLProjectile* Projectile, const FVector& Position)
{
	if (!Settings || !Projectile || !Projectile->IsPoolActive() || Projectile->IsFadingOut()) { return; }
	auto* Response = Projectile->GetFieldResponse();
	FTransit* Existing = Transits.Find(Projectile);
	if (!Existing || Existing->Serial != Response->GetActivationSerial() || Existing->bReflected != Projectile->IsReflected())
	{
		// Reflection resets speed in the core path; treat the returning flight as a new transit.
		FTransit Fresh;
		Fresh.Serial = Response->GetActivationSerial(); Fresh.Previous = Response->GetFlightStart(); Fresh.bReflected = Projectile->IsReflected();
		Transits.Add(Projectile, Fresh);
		if (Settings->Type == ERLArenaExperimentType::AccelerationField && !Projectile->IsReflected())
		{ Response->SetSpeedMultiplier(Settings->Acceleration.BaselineHostileSpeedMultiplier); }
	}
	FTransit& Transit = Transits.FindChecked(Projectile);
	if (!Transit.bApplied)
	{
		for (const auto& Center : Settings->FieldLayout.FieldCenters)
		{
			const FVector Min(Center.X-Settings->FieldLayout.FieldHalfExtent.X, Center.Y-Settings->FieldLayout.FieldHalfExtent.Y, -1);
			const FVector Max(Center.X+Settings->FieldLayout.FieldHalfExtent.X, Center.Y+Settings->FieldLayout.FieldHalfExtent.Y, 1);
			const FVector Start(Transit.Previous.X, Transit.Previous.Y, 0), End(Position.X,Position.Y,0);
			if (!FMath::LineBoxIntersection(FBox(Min,Max), Start, End, End-Start)) { continue; }
			if (Settings->Type == ERLArenaExperimentType::AccelerationField)
			{
				Response->SetSpeedMultiplier(Settings->Acceleration.FieldSpeedMultiplier);
				if (Projectile->IsReflected()) { Response->MarkAccelerationCharged(); }
				if (Settings->Acceleration.AccelerationSound && FMath::IsFinite(Settings->Acceleration.AccelerationSoundVolume))
				{ UGameplayStatics::PlaySoundAtLocation(this, Settings->Acceleration.AccelerationSound, Position, FMath::Clamp(Settings->Acceleration.AccelerationSoundVolume,0.0f,1.0f)); }
			}
			else { Response->Refract(Settings->Refraction.RefractionAngleDegrees); }
			Transit.bApplied = true;
			break;
		}
	}
	Transit.Previous = Position;
}

bool URLBallisticsExperimentComponent::FindBarrierSpawn(int32 Index, int32 Count, FTransform& Transform) const
{
	if (!Settings || Settings->Type != ERLArenaExperimentType::AccelerationField || !Settings->Acceleration.bEnableReturnBarriers) { return false; }
	const int32 SideIndex = Index % 2;
	const float Side = SideIndex == 0 ? -1.0f : 1.0f;
	const auto& Center = Settings->Acceleration.ReturnBarrierCenters[SideIndex];
	const int32 TeamCount = FMath::Max(1, (Count + 1) / 2);
	const float Spread = FMath::Min(1200.0f, Settings->Acceleration.ReturnBarrierSize.Y * 0.35f);
	const float Y = TeamCount > 1 ? FMath::Lerp(-Spread, Spread, static_cast<float>(Index / 2) / (TeamCount - 1)) : 0;
	const FVector Position(Center.X + Side * (Settings->Acceleration.ReturnBarrierSize.X * 0.5f + 350), Center.Y + Y, 96);
	if (FMath::Abs(Position.X) > 1800 || FMath::Abs(Position.Y) > 1800 || GetWorld()->OverlapBlockingTestByChannel(Position, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(42, 96))) { return false; }
	Transform = FTransform(FRotator(0, SideIndex == 0 ? 0 : 180, 0), Position);
	return true;
}

void URLBallisticsExperimentComponent::RegisterEnemy(ARLEnemyCharacter* Enemy)
{
	if (Enemy && Settings && Settings->Type == ERLArenaExperimentType::AccelerationField && Settings->Acceleration.bEnableReturnBarriers)
	{
		Enemies.AddUnique(Enemy);
		Enemy->SetTutorialMovementLocked(true);
	}
}

void URLBallisticsExperimentComponent::ResetExperiment()
{
	for (const auto& Entry : Transits)
	{
		if (auto* Projectile = Entry.Key.Get()) { Projectile->GetFieldResponse()->ClearAccelerationCharge(); Projectile->GetFieldResponse()->SetSpeedMultiplier(1); }
	}
	for (const auto& Entry : Enemies) { if (auto* Enemy = Entry.Get()) { Enemy->SetTutorialMovementLocked(false); } }
	Enemies.Empty();
	for (ARLAccelerationBarrier* Barrier : Barriers) { if (IsValid(Barrier)) { Barrier->Destroy(); } }
	Barriers.Empty();
	for (AStaticMeshActor* Actor : Visuals) { if (IsValid(Actor)) { Actor->Destroy(); } }
	Visuals.Empty(); Transits.Empty(); Settings = nullptr;
}

FText URLBallisticsExperimentComponent::GetStatusText() const
{
	if (!Settings) { return FText::GetEmpty(); }
	return FText::FromString(Settings->DisplayName.ToString() + (Settings->Type == ERLArenaExperimentType::AccelerationField
		? (Settings->Acceleration.bEnableReturnBarriers ? TEXT("\nPARRY THROUGH AMBER FIELDS TO PIERCE CYAN FILTERS") : TEXT("\nAMBER FIELD: SHOTS ACCELERATE - RETURNS TOO")) : TEXT("\nBLUE GATE: SHOTS TURN - RETURNS TOO")));
}
