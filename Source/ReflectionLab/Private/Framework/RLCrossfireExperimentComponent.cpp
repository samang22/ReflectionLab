#include "Framework/RLCrossfireExperimentComponent.h"
#include "Data/RLArenaExperimentDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/Components/RLEnemyAttackComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

URLCrossfireExperimentComponent::URLCrossfireExperimentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URLCrossfireExperimentComponent::Configure(URLArenaExperimentDataAsset* Data)
{
	ResetExperiment();
	if (!Data || Data->Type != ERLArenaExperimentType::Crossfire) { return; }
	if (!FMath::IsFinite(Data->Crossfire.PhaseDuration) || !FMath::IsFinite(Data->Crossfire.WarningDuration) ||
		!FMath::IsFinite(Data->Crossfire.ShutterMoveDuration) || Data->Crossfire.PhaseDuration < 3.0f ||
		Data->Crossfire.WarningDuration < 0.5f || Data->Crossfire.ShutterMoveDuration < 0.1f ||
		Data->Crossfire.WarningDuration + Data->Crossfire.ShutterMoveDuration >= Data->Crossfire.PhaseDuration ||
		!FMath::IsFinite(Data->Crossfire.TeamDistance) || !FMath::IsFinite(Data->Crossfire.TeamSpread) ||
		!FMath::IsFinite(Data->Crossfire.ShutterDistance) || Data->Crossfire.ShutterSize.ContainsNaN() ||
		Data->Crossfire.TeamDistance <= Data->Crossfire.ShutterDistance || Data->Crossfire.TeamSpread <= 0 || Data->Crossfire.ShutterSize.GetMin() <= 0 || !Data->ProjectileDefinition ||
		(Data->Crossfire.bAlternatingFiringLanes && (!FMath::IsFinite(Data->Crossfire.LaneCenterOffset) ||
		Data->Crossfire.LaneCenterOffset < 300 || Data->Crossfire.LaneCenterOffset > 1000)))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid arena experiment settings: %s"), *GetNameSafe(Data));
		return;
	}
	for (const auto& Wall : Data->Crossfire.MazeWalls)
	{
		if (Wall.Center.ContainsNaN() || Wall.Size.ContainsNaN() || Wall.Size.GetMin() <= 0 || !FMath::IsFinite(Wall.Yaw) ||
			Wall.PhaseGroup < 0 || Wall.PhaseGroup > 1 || FMath::Abs(Wall.Center.X) > 1700 || FMath::Abs(Wall.Center.Y) > 1700)
		{
			UE_LOG(LogTemp, Error, TEXT("Invalid crossfire maze wall in %s"), *GetNameSafe(Data));
			return;
		}
	}
	Settings = Data;
	Elapsed = 0;
	LastPhase = INDEX_NONE;
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube) { ResetExperiment(); return; }
	TArray<FRLCrossfireMazeWall> Layout = Data->Crossfire.MazeWalls;
	if (Layout.IsEmpty())
	{
		for (int32 Index = 0; Index < 2; ++Index)
		{
			auto& Wall = Layout.AddDefaulted_GetRef();
			Wall.Center = FVector2D(Index == 0 ? -Data->Crossfire.ShutterDistance : Data->Crossfire.ShutterDistance, 0);
			Wall.Size = Data->Crossfire.ShutterSize;
			Wall.PhaseGroup = Index == 0 ? 1 : 0;
		}
	}
	for (const auto& Wall : Layout)
	{
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Shutter = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(Wall.Center.X, Wall.Center.Y,
			(Wall.bAlwaysClosed ? 1 : -1) * Wall.Size.Z * 0.5f), FRotator(0, Wall.Yaw, 0), Spawn);
		AStaticMeshActor* Indicator = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(Wall.Center.X, Wall.Center.Y, 1), FRotator(0, Wall.Yaw, 0), Spawn);
		if (!Shutter || !Indicator) { if (Shutter) { Shutter->Destroy(); } if (Indicator) { Indicator->Destroy(); } ResetExperiment(); return; }
		for (AStaticMeshActor* Actor : {Shutter, Indicator})
		{
			UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
			Mesh->SetMobility(EComponentMobility::Movable);
			Mesh->SetStaticMesh(Cube);
			Mesh->SetCollisionProfileName(TEXT("NoCollision"));
			Mesh->SetCanEverAffectNavigation(false);
			if (Data->SurfaceMaterial) { Mesh->SetMaterial(0, Data->SurfaceMaterial); }
		}
		Shutter->SetActorScale3D(Wall.Size / 100.0f);
		if (Wall.bAlwaysClosed) { Shutter->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll")); }
		Indicator->SetActorScale3D(FVector(Wall.Size.X / 100.0f, Wall.Size.Y / 100.0f, 0.01f));
		UMaterialInstanceDynamic* Material = Shutter->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0);
		Indicator->GetStaticMeshComponent()->SetMaterial(0, Material);
		Shutters.Add(Shutter);
		Indicators.Add(Indicator);
		Materials.Add(Material);
		ShutterSizes.Add(Wall.Size);
		ShutterPhaseGroups.Add(Wall.PhaseGroup);
		PermanentWalls.Add(Wall.bAlwaysClosed);
		bSkipClosing.Add(false);
	}
}

void URLCrossfireExperimentComponent::ResetExperiment()
{
	for (const auto& Entry : Enemies)
	{
		if (ARLEnemyCharacter* Enemy = Entry.Enemy.Get(); Enemy && Enemy->IsPoolActive())
		{
			Enemy->SetTutorialMovementLocked(false);
			Enemy->FindComponentByClass<URLEnemyAttackComponent>()->SetExperimentAttackAllowed(true, false);
			Enemy->FindComponentByClass<URLEnemyAttackComponent>()->StopFiring();
		}
	}
	Enemies.Empty();
	for (AStaticMeshActor* Actor : Shutters) { if (IsValid(Actor)) { Actor->Destroy(); } }
	for (AStaticMeshActor* Actor : Indicators) { if (IsValid(Actor)) { Actor->Destroy(); } }
	Shutters.Empty(); Indicators.Empty(); Materials.Empty();
	ShutterSizes.Empty(); ShutterPhaseGroups.Empty(); bSkipClosing.Empty();
	PermanentWalls.Empty();
	Settings = nullptr;
	SetComponentTickEnabled(false);
}

void URLCrossfireExperimentComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	ResetExperiment();
	Super::EndPlay(Reason);
}

float URLCrossfireExperimentComponent::PhaseTime() const
{
	return Settings ? FMath::Fmod(Elapsed, Settings->Crossfire.PhaseDuration) : 0;
}

int32 URLCrossfireExperimentComponent::OccupiedTeamMask() const
{
	int32 Mask = 0;
	for (const auto& Entry : Enemies)
	{
		if (const auto* Enemy = Entry.Enemy.Get(); Enemy && Enemy->IsPoolActive()) { Mask |= 1 << Entry.Team; }
	}
	return Mask;
}

bool URLCrossfireExperimentComponent::ShouldCloseWall(int32 Index, int32 Phase, int32 OccupiedMask) const
{
	if (PermanentWalls[Index]) { return true; }
	if (Settings->Crossfire.bAlternatingFiringLanes && Settings->Crossfire.bKeepLastOccupiedLaneOpen &&
		(OccupiedMask == 1 || OccupiedMask == 2) && (OccupiedMask & (1 << ShutterPhaseGroups[Index])))
	{
		return false;
	}
	return Phase % 2 == ShutterPhaseGroups[Index];
}

bool URLCrossfireExperimentComponent::PlayerBlocksShutter(int32 Index) const
{
	const ACharacter* Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player || !Settings) { return false; }
	const FVector Position = Player->GetActorLocation();
	const float Padding = Player->GetCapsuleComponent()->GetScaledCapsuleRadius() + 20;
	const auto* Shutter = Shutters[Index].Get();
	const FVector Local = Shutter->GetActorRotation().UnrotateVector(Position - Shutter->GetActorLocation());
	return FMath::Abs(Local.X) < ShutterSizes[Index].X * 0.5f + Padding &&
		FMath::Abs(Local.Y) < ShutterSizes[Index].Y * 0.5f + Padding;
}

void URLCrossfireExperimentComponent::AdvanceExperiment(float DeltaTime)
{
	if (!Settings) { return; }
	Elapsed += FMath::Max(0.0f, DeltaTime);
	const int32 Phase = static_cast<int32>(Elapsed / Settings->Crossfire.PhaseDuration);
	if (Phase != LastPhase)
	{
		LastPhase = Phase;
		for (int32 Index = 0; Index < Shutters.Num(); ++Index)
		{
			bSkipClosing[Index] = false;
		}
	}
	const float Time = PhaseTime();
	const int32 OccupiedMask = OccupiedTeamMask();
	for (int32 Index = 0; Index < Shutters.Num(); ++Index)
	{
		if (PermanentWalls[Index])
		{
			if (Materials[Index]) { Materials[Index]->SetVectorParameterValue(TEXT("SurfaceColor"), FLinearColor(0.16f, 0.2f, 0.23f)); }
			continue;
		}
		const bool bClose = ShouldCloseWall(Index, Phase, OccupiedMask);
		if (bClose && PlayerBlocksShutter(Index)) { bSkipClosing[Index] = true; }
		UStaticMeshComponent* Mesh = Shutters[Index]->GetStaticMeshComponent();
		if (Time >= Settings->Crossfire.WarningDuration)
		{
			const float Target = bClose && !bSkipClosing[Index] ? ShutterSizes[Index].Z * 0.5f : -ShutterSizes[Index].Z * 0.5f;
			FVector Position = Shutters[Index]->GetActorLocation();
			// Never lift, drop or teleport a player standing over the panel footprint.
			// Resume travel smoothly once the player leaves, including during opening.
			const bool bOccupied = PlayerBlocksShutter(Index);
			const float TravelDelta = FMath::Min(FMath::Max(0.0f, DeltaTime), Time - Settings->Crossfire.WarningDuration);
			if (!bOccupied) { Position.Z = FMath::FInterpConstantTo(Position.Z, Target, TravelDelta, ShutterSizes[Index].Z / Settings->Crossfire.ShutterMoveDuration); }
			Shutters[Index]->SetActorLocation(Position);
			// Projectile spheres block WorldStatic but ignore WorldDynamic. Mobility
			// remains Movable for animation; the closed panel uses the wall profile.
			const bool bClosed = Position.Z >= ShutterSizes[Index].Z * 0.5f - 0.1f;
			const FName Profile = bClosed ? TEXT("BlockAll") : TEXT("NoCollision");
			if (Mesh->GetCollisionProfileName() != Profile) { Mesh->SetCollisionProfileName(Profile); }
		}
		if (Materials[Index] && Settings->Crossfire.bAlternatingFiringLanes)
		{
			const bool bPreview = Time >= Settings->Crossfire.PhaseDuration - Settings->Crossfire.WarningDuration;
			const bool bNextClosed = ShouldCloseWall(Index, Phase + 1, OccupiedMask);
			const FLinearColor Color = bPreview
				? (bNextClosed ? FLinearColor(0.9f,0.35f,0.03f) : FLinearColor(0.05f,0.85f,1.0f))
				: (bClose && !bSkipClosing[Index] ? FLinearColor(0.22f,0.32f,0.35f) : FLinearColor(0.05f,0.55f,0.7f));
			Materials[Index]->SetVectorParameterValue(TEXT("SurfaceColor"), Color);
		}
		else if (Materials[Index])
		{
			Materials[Index]->SetVectorParameterValue(TEXT("SurfaceColor"), Time < Settings->Crossfire.WarningDuration || bSkipClosing[Index]
				? FLinearColor(0.8f, 0.38f, 0.05f) : FLinearColor(0.22f, 0.42f, 0.45f));
		}
	}
}

bool URLCrossfireExperimentComponent::FindTeamSpawn(int32 Index, int32 Count, FTransform& OutTransform) const
{
	if (!Settings) { return false; }
	const int32 TeamCount = FMath::Max(1, (Count + 1) / 2);
	float Y = TeamCount > 1 ? FMath::Lerp(-Settings->Crossfire.TeamSpread, Settings->Crossfire.TeamSpread, static_cast<float>(Index / 2) / (TeamCount - 1)) : 0;
	if (Settings->Crossfire.bAlternatingFiringLanes) { Y += (Index % 2 == 0 ? -1 : 1) * Settings->Crossfire.LaneCenterOffset; }
	for (int32 Attempt = 0; Attempt < 20; ++Attempt)
	{
		const FVector Position(Index % 2 == 0 ? -Settings->Crossfire.TeamDistance : Settings->Crossfire.TeamDistance,
			Y + FMath::FRandRange(-50.0f, 50.0f), 96.0f);
		if (!GetWorld()->OverlapBlockingTestByChannel(Position, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(42, 96)))
		{
			OutTransform = FTransform(FRotator(0, Index % 2 == 0 ? 0 : 180, 0), Position);
			return true;
		}
	}
	return false;
}

void URLCrossfireExperimentComponent::RegisterEnemy(ARLEnemyCharacter* Enemy, int32 Index)
{
	if (!Settings || !Enemy) { return; }
	Enemies.Add({Enemy, Index % 2});
	Enemy->SetTutorialMovementLocked(true);
	// Both banks fire continuously; shutters, not attack permissions, provide cover.
	Enemy->FindComponentByClass<URLEnemyAttackComponent>()->SetExperimentAttackAllowed(true);
}

FText URLCrossfireExperimentComponent::GetStatusText() const
{
	if (!Settings) { return FText::GetEmpty(); }
	if (Settings->Crossfire.bAlternatingFiringLanes)
	{
		const int32 Mask = OccupiedTeamMask();
		const bool bLastBank = Settings->Crossfire.bKeepLastOccupiedLaneOpen && (Mask == 1 || Mask == 2);
		const bool bUpper = bLastBank ? Mask == 2 : static_cast<int32>(Elapsed / Settings->Crossfire.PhaseDuration) % 2 == 0;
		return FText::FromString(Settings->DisplayName.ToString() + TEXT("\n") +
			(bUpper ? TEXT("UPPER LANE OPEN") : TEXT("LOWER LANE OPEN")) +
			(bLastBank ? TEXT(" - LAST ENEMY LANE HELD OPEN") :
				(PhaseTime() >= Settings->Crossfire.PhaseDuration - Settings->Crossfire.WarningDuration ? TEXT(" - LANES SWITCHING SOON") : TEXT(" - MOVE TO RETURN FIRE"))));
	}
	if (!Settings->Crossfire.MazeWalls.IsEmpty())
	{
		return FText::FromString(Settings->DisplayName.ToString() + TEXT("\nSHIFTING WALLS - FIND AN OPEN FIRING LANE"));
	}
	const TCHAR* Cover = Settings->IsLeftPhase(Elapsed) ? TEXT("RIGHT COVER - LEFT LANE EXPOSED") : TEXT("LEFT COVER - RIGHT LANE EXPOSED");
	return FText::FromString(FString::Printf(TEXT("%s\n%s"), *Settings->DisplayName.ToString(),
		PhaseTime() < Settings->Crossfire.WarningDuration + Settings->Crossfire.ShutterMoveDuration ? TEXT("SHUTTERS CHANGING - KEEP CLEAR") :
		Cover));
}
