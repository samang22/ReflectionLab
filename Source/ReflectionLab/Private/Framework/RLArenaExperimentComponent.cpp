#include "Framework/RLArenaExperimentComponent.h"
#include "Framework/RLArenaVariationComponent.h"
#include "Framework/RLBallisticsExperimentComponent.h"
#include "Framework/RLMirrorArenaComponent.h"
#include "Framework/GameMode/RLGameModeBase.h"
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

URLArenaExperimentComponent::URLArenaExperimentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void URLArenaExperimentComponent::Configure(URLArenaExperimentDataAsset* Data)
{
	ResetExperiment();
	if (!Data) { return; }
	if (Data->Type == ERLArenaExperimentType::MirrorArena)
	{
		if (!MirrorArena) { MirrorArena = NewObject<URLMirrorArenaComponent>(GetOwner()); MirrorArena->RegisterComponent(); }
		if (MirrorArena->Configure(Data)) { Settings = Data; SetComponentTickEnabled(true); }
		return;
	}
	if (Data->Type == ERLArenaExperimentType::AccelerationField || Data->Type == ERLArenaExperimentType::RefractionGate)
	{
		if (!Ballistics) { Ballistics = NewObject<URLBallisticsExperimentComponent>(GetOwner()); Ballistics->RegisterComponent(); }
		if (Ballistics->Configure(Data)) { Settings = Data; SetComponentTickEnabled(true); }
		else { UE_LOG(LogTemp, Error, TEXT("Invalid ballistics experiment: %s"), *GetNameSafe(Data)); }
		return;
	}
	if (Data->Type != ERLArenaExperimentType::Crossfire)
	{
		if (!Variation) { Variation = NewObject<URLArenaVariationComponent>(GetOwner()); Variation->RegisterComponent(); }
		if (Variation->Configure(Data)) { Settings = Data; SetComponentTickEnabled(true); }
		else { UE_LOG(LogTemp, Error, TEXT("Invalid arena variation: %s"), *GetNameSafe(Data)); }
		return;
	}
	if (!FMath::IsFinite(Data->PhaseDuration) || !FMath::IsFinite(Data->WarningDuration) ||
		!FMath::IsFinite(Data->ShutterMoveDuration) || Data->PhaseDuration < 3.0f ||
		Data->WarningDuration < 0.5f || Data->ShutterMoveDuration < 0.1f ||
		Data->WarningDuration + Data->ShutterMoveDuration >= Data->PhaseDuration ||
		!FMath::IsFinite(Data->TeamDistance) || !FMath::IsFinite(Data->TeamSpread) ||
		!FMath::IsFinite(Data->ShutterDistance) || Data->ShutterSize.ContainsNaN() ||
		Data->TeamDistance <= Data->ShutterDistance || Data->TeamSpread <= 0 || Data->ShutterSize.GetMin() <= 0 || !Data->ProjectileDefinition)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid arena experiment settings: %s"), *GetNameSafe(Data));
		return;
	}
	Settings = Data;
	Elapsed = 0;
	LastPhase = INDEX_NONE;
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube) { ResetExperiment(); return; }
	for (int32 Index = 0; Index < 2; ++Index)
	{
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const float X = Index == 0 ? -Data->ShutterDistance : Data->ShutterDistance;
		AStaticMeshActor* Shutter = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(X, 0, -Data->ShutterSize.Z * 0.5f), FRotator::ZeroRotator, Spawn);
		AStaticMeshActor* Indicator = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(X, 0, 1), FRotator::ZeroRotator, Spawn);
		if (!Shutter || !Indicator) { if (Shutter) { Shutter->Destroy(); } if (Indicator) { Indicator->Destroy(); } ResetExperiment(); return; }
		for (AStaticMeshActor* Actor : {Shutter, Indicator})
		{
			UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
			Mesh->SetMobility(EComponentMobility::Movable);
			Mesh->SetStaticMesh(Cube);
			Mesh->SetCollisionProfileName(TEXT("NoCollision"));
			Mesh->SetCanEverAffectNavigation(false);
			if (Data->ShutterMaterial) { Mesh->SetMaterial(0, Data->ShutterMaterial); }
		}
		Shutter->SetActorScale3D(Data->ShutterSize / 100.0f);
		Indicator->SetActorScale3D(FVector(Data->ShutterSize.X / 100.0f, Data->ShutterSize.Y / 100.0f, 0.01f));
		UMaterialInstanceDynamic* Material = Shutter->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0);
		Indicator->GetStaticMeshComponent()->SetMaterial(0, Material);
		Shutters.Add(Shutter);
		Indicators.Add(Indicator);
		Materials.Add(Material);
	}
	SetComponentTickEnabled(true);
}

void URLArenaExperimentComponent::ResetExperiment()
{
	if (MirrorArena) { MirrorArena->ResetArena(); }
	if (Ballistics) { Ballistics->ResetExperiment(); }
	if (Variation) { Variation->ResetVariation(); }
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
	Settings = nullptr;
	SetComponentTickEnabled(false);
}

void URLArenaExperimentComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	ResetExperiment();
	Super::EndPlay(Reason);
}

float URLArenaExperimentComponent::PhaseTime() const
{
	return Settings ? FMath::Fmod(Elapsed, Settings->PhaseDuration) : 0;
}

bool URLArenaExperimentComponent::PlayerBlocksShutter(int32 Index) const
{
	const ACharacter* Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player || !Settings) { return false; }
	const FVector Position = Player->GetActorLocation();
	const float Padding = Player->GetCapsuleComponent()->GetScaledCapsuleRadius() + 20;
	const float X = Index == 0 ? -Settings->ShutterDistance : Settings->ShutterDistance;
	return FMath::Abs(Position.X - X) < Settings->ShutterSize.X * 0.5f + Padding &&
		FMath::Abs(Position.Y) < Settings->ShutterSize.Y * 0.5f + Padding;
}

void URLArenaExperimentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function)
{
	Super::TickComponent(DeltaTime, TickType, Function);
	const ARLGameModeBase* Mode = Cast<ARLGameModeBase>(GetOwner());
	if (!Settings || !Mode || Mode->GetRunState() != ERLRunState::PlayingRound) { ResetExperiment(); return; }
	AdvanceExperiment(DeltaTime);
}

void URLArenaExperimentComponent::AdvanceExperiment(float DeltaTime)
{
	if (!Settings) { return; }
	if (Settings->Type == ERLArenaExperimentType::MirrorArena) { return; }
	if (Settings->Type == ERLArenaExperimentType::AccelerationField || Settings->Type == ERLArenaExperimentType::RefractionGate) { Ballistics->Advance(); return; }
	if (Settings->Type != ERLArenaExperimentType::Crossfire) { Variation->Advance(DeltaTime); return; }
	Elapsed += FMath::Max(0.0f, DeltaTime);
	const int32 Phase = static_cast<int32>(Elapsed / Settings->PhaseDuration);
	if (Phase != LastPhase)
	{
		LastPhase = Phase;
		for (int32 Index = 0; Index < Shutters.Num(); ++Index)
		{
			bSkipClosing[Index] = false;
			PhaseStartHeight[Index] = Shutters[Index]->GetActorLocation().Z;
		}
	}
	const float Time = PhaseTime();
	for (int32 Index = 0; Index < Shutters.Num(); ++Index)
	{
		const bool bClose = (Index == 0) != Settings->IsLeftPhase(Elapsed);
		if (bClose && PlayerBlocksShutter(Index)) { bSkipClosing[Index] = true; }
		UStaticMeshComponent* Mesh = Shutters[Index]->GetStaticMeshComponent();
		if (Time >= Settings->WarningDuration)
		{
			const float Target = bClose && !bSkipClosing[Index] ? Settings->ShutterSize.Z * 0.5f : -Settings->ShutterSize.Z * 0.5f;
			FVector Position = Shutters[Index]->GetActorLocation();
			// Never lift, drop or teleport a player standing over the panel footprint.
			// Resume travel smoothly once the player leaves, including during opening.
			const bool bOccupied = PlayerBlocksShutter(Index);
			const float TravelDelta = FMath::Min(FMath::Max(0.0f, DeltaTime), Time - Settings->WarningDuration);
			if (!bOccupied) { Position.Z = FMath::FInterpConstantTo(Position.Z, Target, TravelDelta, Settings->ShutterSize.Z / Settings->ShutterMoveDuration); }
			Shutters[Index]->SetActorLocation(Position);
			// Projectile spheres block WorldStatic but ignore WorldDynamic. Mobility
			// remains Movable for animation; the closed panel uses the wall profile.
			const bool bClosed = Position.Z >= Settings->ShutterSize.Z * 0.5f - 0.1f;
			const FName Profile = bClosed ? TEXT("BlockAll") : TEXT("NoCollision");
			if (Mesh->GetCollisionProfileName() != Profile) { Mesh->SetCollisionProfileName(Profile); }
		}
		if (Materials[Index])
		{
			Materials[Index]->SetVectorParameterValue(TEXT("SurfaceColor"), Time < Settings->WarningDuration || bSkipClosing[Index]
				? FLinearColor(0.8f, 0.38f, 0.05f) : FLinearColor(0.22f, 0.42f, 0.45f));
		}
	}
}

bool URLArenaExperimentComponent::FindTeamSpawn(int32 Index, int32 Count, FTransform& OutTransform) const
{
	if (!Settings) { return false; }
	if (Settings->Type == ERLArenaExperimentType::MirrorArena) { return MirrorArena->FindSpawn(Index,OutTransform); }
	const int32 TeamCount = FMath::Max(1, (Count + 1) / 2);
	const float Y = TeamCount > 1 ? FMath::Lerp(-Settings->TeamSpread, Settings->TeamSpread, static_cast<float>(Index / 2) / (TeamCount - 1)) : 0;
	for (int32 Attempt = 0; Attempt < 20; ++Attempt)
	{
		const FVector Position(Index % 2 == 0 ? -Settings->TeamDistance : Settings->TeamDistance,
			Y + FMath::FRandRange(-50.0f, 50.0f), 96.0f);
		if (!GetWorld()->OverlapBlockingTestByChannel(Position, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(42, 96)))
		{
			OutTransform = FTransform(FRotator(0, Index % 2 == 0 ? 0 : 180, 0), Position);
			return true;
		}
	}
	return false;
}

void URLArenaExperimentComponent::RegisterEnemy(ARLEnemyCharacter* Enemy, int32 Index)
{
	if (!Settings || !Enemy) { return; }
	if (Settings->Type == ERLArenaExperimentType::MirrorArena) { MirrorArena->RegisterEnemy(Enemy); return; }
	if (Settings->Type == ERLArenaExperimentType::AccelerationField || Settings->Type == ERLArenaExperimentType::RefractionGate) { return; }
	if (Settings->Type != ERLArenaExperimentType::Crossfire) { Variation->RegisterEnemy(Enemy, Index); return; }
	Enemies.Add({Enemy});
	Enemy->SetTutorialMovementLocked(true);
	// Both banks fire continuously; shutters, not attack permissions, provide cover.
	Enemy->FindComponentByClass<URLEnemyAttackComponent>()->SetExperimentAttackAllowed(true);
}

FText URLArenaExperimentComponent::GetStatusText() const
{
	if (!Settings) { return FText::GetEmpty(); }
	if (Settings->Type == ERLArenaExperimentType::MirrorArena) { return MirrorArena->GetStatusText(); }
	if (Settings->Type == ERLArenaExperimentType::AccelerationField || Settings->Type == ERLArenaExperimentType::RefractionGate) { return Ballistics->GetStatusText(); }
	if (Settings->Type != ERLArenaExperimentType::Crossfire) { return Variation->GetStatusText(); }
	const TCHAR* Cover = Settings->IsLeftPhase(Elapsed) ? TEXT("RIGHT COVER - LEFT LANE EXPOSED") : TEXT("LEFT COVER - RIGHT LANE EXPOSED");
	return FText::FromString(FString::Printf(TEXT("%s\n%s"), *Settings->DisplayName.ToString(),
		PhaseTime() < Settings->WarningDuration + Settings->ShutterMoveDuration ? TEXT("SHUTTERS CHANGING - KEEP CLEAR") :
		Cover));
}
