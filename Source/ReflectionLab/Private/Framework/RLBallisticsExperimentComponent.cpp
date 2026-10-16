#include "Framework/RLBallisticsExperimentComponent.h"
#include "Data/RLArenaExperimentDataAsset.h"
#include "Combat/RLProjectile.h"
#include "Combat/RLProjectileFieldResponseComponent.h"
#include "Combat/RLProjectileVisualComponent.h"
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
	if (Settings->ShutterMaterial) { Mesh->SetMaterial(0, Settings->ShutterMaterial); }
	if (auto* Material = Mesh->CreateDynamicMaterialInstance(0)) { Material->SetVectorParameterValue(TEXT("SurfaceColor"), Color); }
	Actor->SetActorScale3D(Scale);
	return true;
}

bool URLBallisticsExperimentComponent::Configure(URLArenaExperimentDataAsset* Data)
{
	ResetExperiment();
	if (!Data || Data->FieldCenters.IsEmpty() || Data->FieldHalfExtent.ContainsNaN() || Data->FieldHalfExtent.GetMin() <= 0 ||
		!FMath::IsFinite(Data->FieldSpeedMultiplier) || Data->FieldSpeedMultiplier < 1 || Data->FieldSpeedMultiplier > 3 ||
		!FMath::IsFinite(Data->BaselineHostileSpeedMultiplier) || Data->BaselineHostileSpeedMultiplier < 0.1f || Data->BaselineHostileSpeedMultiplier > 1 ||
		!FMath::IsFinite(Data->RefractionAngleDegrees) || FMath::Abs(Data->RefractionAngleDegrees) > 80) { return false; }
	for (const auto& Center : Data->FieldCenters) { if (Center.ContainsNaN()) { return false; } }
	Settings = Data;
	const bool bAcceleration = Data->Type == ERLArenaExperimentType::AccelerationField;
	const FLinearColor Color = bAcceleration ? FLinearColor(0.65f,0.35f,0.03f) : FLinearColor(0.12f,0.35f,0.7f);
	for (const auto& Center : Data->FieldCenters)
	{
		const FVector Position(Center.X, Center.Y, 1);
		const FVector2D Extent = Data->FieldHalfExtent;
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
				bAcceleration ? Side*40 : Data->RefractionAngleDegrees + Side*40, Color)) { ResetExperiment(); return false; }
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
		ARLProjectile* Projectile = *It;
		if (!Projectile->IsPoolActive() || Projectile->IsFadingOut()) { continue; }
		auto* Response = Projectile->GetFieldResponse();
		const FVector Position = Projectile->GetActorLocation();
		FTransit* Existing = Transits.Find(Projectile);
		if (!Existing || Existing->Serial != Response->GetActivationSerial() || Existing->bReflected != Projectile->IsReflected())
		{
			// Reflection resets speed in the core path; treat the returning flight as a new transit.
			FTransit Fresh;
			Fresh.Serial = Response->GetActivationSerial(); Fresh.Previous = Response->GetFlightStart(); Fresh.bReflected = Projectile->IsReflected();
			Transits.Add(Projectile, Fresh);
			if (Settings->Type == ERLArenaExperimentType::AccelerationField && !Projectile->IsReflected())
			{ Response->SetSpeedMultiplier(Settings->BaselineHostileSpeedMultiplier); }
		}
		FTransit& Transit = Transits.FindChecked(Projectile);
		if (!Transit.bApplied)
		{
			for (const auto& Center : Settings->FieldCenters)
			{
				const FVector Min(Center.X-Settings->FieldHalfExtent.X, Center.Y-Settings->FieldHalfExtent.Y, -1);
				const FVector Max(Center.X+Settings->FieldHalfExtent.X, Center.Y+Settings->FieldHalfExtent.Y, 1);
				const FVector Start(Transit.Previous.X, Transit.Previous.Y, 0), End(Position.X,Position.Y,0);
				if (!FMath::LineBoxIntersection(FBox(Min,Max), Start, End, End-Start)) { continue; }
				if (Settings->Type == ERLArenaExperimentType::AccelerationField)
				{
					Response->SetSpeedMultiplier(Settings->FieldSpeedMultiplier);
					if (Settings->AccelerationSound && FMath::IsFinite(Settings->AccelerationSoundVolume))
					{ UGameplayStatics::PlaySoundAtLocation(this, Settings->AccelerationSound, Position, FMath::Clamp(Settings->AccelerationSoundVolume,0.0f,1.0f)); }
				}
				else { Response->Refract(Settings->RefractionAngleDegrees); }
				Transit.bApplied = true;
				break;
			}
		}
		Transit.Previous = Position;
	}
}

void URLBallisticsExperimentComponent::ResetExperiment()
{
	for (const auto& Entry : Transits)
	{
		if (auto* Projectile = Entry.Key.Get()) { Projectile->GetFieldResponse()->SetSpeedMultiplier(1); }
	}
	for (AStaticMeshActor* Actor : Visuals) { if (IsValid(Actor)) { Actor->Destroy(); } }
	Visuals.Empty(); Transits.Empty(); Settings = nullptr;
}

FText URLBallisticsExperimentComponent::GetStatusText() const
{
	if (!Settings) { return FText::GetEmpty(); }
	return FText::FromString(Settings->DisplayName.ToString() + (Settings->Type == ERLArenaExperimentType::AccelerationField
		? TEXT("\nAMBER FIELD: SHOTS ACCELERATE - RETURNS TOO") : TEXT("\nBLUE GATE: SHOTS TURN - RETURNS TOO")));
}
