#include "Framework/RLArenaVariationComponent.h"
#include "Data/RLArenaExperimentDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/RLRobotMinionCharacter.h"
#include "Data/RLRobotMinionDataAsset.h"
#include "Enemies/Components/RLEnemyAttackComponent.h"
#include "Enemies/Components/RLBossLaserComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

AStaticMeshActor* URLArenaVariationComponent::MakeVisual(const FVector& Position, const FVector& Scale)
{
	AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Position, FRotator::ZeroRotator);
	if (!Actor) { return nullptr; }
	Visuals.Add(Actor);
	auto* Mesh = Actor->GetStaticMeshComponent();
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Mesh->SetCollisionProfileName(TEXT("NoCollision"));
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetCastShadow(false);
	if (Settings->ShutterMaterial) { Mesh->SetMaterial(0, Settings->ShutterMaterial); }
	Mesh->CreateDynamicMaterialInstance(0);
	Actor->SetActorScale3D(Scale);
	return Actor;
}

bool URLArenaVariationComponent::Configure(URLArenaExperimentDataAsset* Data)
{
	ResetVariation();
	if (!Data || !FMath::IsFinite(Data->PhaseDuration) || !FMath::IsFinite(Data->WarningDuration) ||
		Data->WarningDuration < 1 || Data->PhaseDuration <= Data->WarningDuration + 1 ||
		!FMath::IsFinite(Data->SupportRadius) || Data->SupportRadius <= 0 ||
		!FMath::IsFinite(Data->SupportAttackRate) || Data->SupportAttackRate < 1 || Data->SupportAttackRate > 3 ||
		!FMath::IsFinite(Data->ArenaHalfExtent) || Data->ArenaHalfExtent <= 100 ||
		!FMath::IsFinite(Data->ZoneDamageInterval) || Data->ZoneDamageInterval < 0.1f ||
		!FMath::IsFinite(Data->ZoneDamage) || Data->ZoneDamage < 0 ||
		(Data->Type == ERLArenaExperimentType::SupportNetwork && !Data->SupportEnemyClass)) { return false; }
	Settings = Data;
	if (Data->Type == ERLArenaExperimentType::AlternatingZones)
	{
		// Thin floor plates, not blocking volumes. Leave a safe central crossing lane.
		const float Half = Data->ArenaHalfExtent;
		for (int32 Index = 0; Index < 2; ++Index)
		{
			if (!MakeVisual(FVector((Index ? 1 : -1) * (Half + 100) * 0.5f, 0, 1),
				FVector((Half - 100) / 100, Half / 50, 0.01f))) { ResetVariation(); return false; }
		}
	}
	Advance(0);
	return true;
}

void URLArenaVariationComponent::RegisterEnemy(ARLEnemyCharacter* Enemy, int32 Index)
{
	if (!Settings || !Enemy) { return; }
	Enemies.Add(Enemy);
	if (Settings->Type == ERLArenaExperimentType::SupportNetwork && Index == 0)
	{
		Support = Enemy;
		if (const auto* Robot = Cast<ARLRobotMinionCharacter>(Enemy); Robot && Robot->Settings)
		{
			FRLWaveDefinition BasicWave;
			BasicWave.DefaultProjectileDefinition = Robot->Settings->ProjectileDefinition;
			BasicWave.AttackIntervalMultiplier = 2;
			Enemy->FindComponentByClass<URLEnemyAttackComponent>()->ApplyWaveDefinition(BasicWave);
		}
		if (auto* Laser = Enemy->FindComponentByClass<URLBossLaserComponent>()) { bSupportLaserTickEnabled = Laser->IsComponentTickEnabled(); Laser->Cancel(true); Laser->SetComponentTickEnabled(false); }
		// A tall cyan marker makes the priority target readable above crowds.
		MakeVisual(Enemy->GetActorLocation() + FVector(0, 0, 150), FVector(0.25f, 0.25f, 0.5f));
		if (Visuals.Num())
		{
			if (auto* Material = Cast<UMaterialInstanceDynamic>(Visuals[0]->GetStaticMeshComponent()->GetMaterial(0)))
			{ Material->SetVectorParameterValue(TEXT("SurfaceColor"), FLinearColor(0.05f,0.65f,0.7f)); }
		}
	}
	else if (Settings->Type == ERLArenaExperimentType::SupportNetwork)
	{
		MakeVisual(Enemy->GetActorLocation(), FVector(1, 0.025f, 0.025f));
	}
}

bool URLArenaVariationComponent::IsZoneDangerous(const FVector& Position) const
{
	return Settings && Settings->Type == ERLArenaExperimentType::AlternatingZones &&
		FMath::Fmod(Elapsed, Settings->PhaseDuration) >= Settings->WarningDuration &&
		FMath::Abs(Position.X) > 100 && FMath::Abs(Position.X) <= Settings->ArenaHalfExtent &&
		FMath::Abs(Position.Y) <= Settings->ArenaHalfExtent &&
		((Position.X < 0) == Settings->IsLeftPhase(Elapsed));
}

void URLArenaVariationComponent::Advance(float DeltaTime)
{
	if (!Settings || !FMath::IsFinite(DeltaTime) || DeltaTime < 0) { return; }
	Elapsed += DeltaTime;
	if (Settings->Type == ERLArenaExperimentType::SupportNetwork)
	{
		const bool bSupportAlive = Support.IsValid() && Support->IsPoolActive();
		for (int32 Index = 0; Index < Enemies.Num(); ++Index)
		{
			ARLEnemyCharacter* Enemy = Enemies[Index].Get();
			if (Index > 0 && Visuals.IsValidIndex(Index)) { Visuals[Index]->SetActorHiddenInGame(true); }
			if (!Enemy || !Enemy->IsPoolActive()) { continue; }
			const bool bLinked = bSupportAlive && Enemy != Support.Get() &&
				FVector::DistSquared2D(Enemy->GetActorLocation(), Support->GetActorLocation()) <= FMath::Square(Settings->SupportRadius);
			Enemy->FindComponentByClass<URLEnemyAttackComponent>()->SetSupportAttackRate(bLinked ? Settings->SupportAttackRate : 1);
			if (bLinked && Visuals.IsValidIndex(Index))
			{
				FVector From = Support->GetActorLocation(); From.Z = 60;
				FVector To = Enemy->GetActorLocation(); To.Z = 60;
				Visuals[Index]->SetActorHiddenInGame(false);
				Visuals[Index]->SetActorLocation((From + To) * 0.5f);
				Visuals[Index]->SetActorRotation((To - From).Rotation());
				Visuals[Index]->SetActorScale3D(FVector((To - From).Size() / 100, 0.025f, 0.025f));
			}
		}
		if (Visuals.Num())
		{
			Visuals[0]->SetActorHiddenInGame(!bSupportAlive);
			if (bSupportAlive) { Visuals[0]->SetActorLocation(Support->GetActorLocation() + FVector(0,0,150)); }
		}
	}
	else
	{
		const bool bWarning = FMath::Fmod(Elapsed, Settings->PhaseDuration) < Settings->WarningDuration;
		for (int32 Index = 0; Index < Visuals.Num(); ++Index)
		{
			const bool bDangerSide = (Index == 0) == Settings->IsLeftPhase(Elapsed);
			Visuals[Index]->SetActorHiddenInGame(!bDangerSide);
			if (auto* Material = Cast<UMaterialInstanceDynamic>(Visuals[Index]->GetStaticMeshComponent()->GetMaterial(0)))
			{
				Material->SetVectorParameterValue(TEXT("SurfaceColor"), bWarning ? FLinearColor(0.65f,0.4f,0.08f) : FLinearColor(0.65f,0.08f,0.04f));
			}
		}
		DamageCooldown = FMath::Max(0.0f, DamageCooldown - DeltaTime);
		if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0); Pawn && IsZoneDangerous(Pawn->GetActorLocation()) && DamageCooldown <= 0)
		{
			UGameplayStatics::ApplyDamage(Pawn, Settings->ZoneDamage, nullptr, GetOwner(), nullptr);
			DamageCooldown = Settings->ZoneDamageInterval;
		}
	}
}

void URLArenaVariationComponent::ResetVariation()
{
	if (Support.IsValid())
	{
		if (auto* Laser = Support->FindComponentByClass<URLBossLaserComponent>()) { Laser->SetComponentTickEnabled(bSupportLaserTickEnabled); }
	}
	for (const auto& Entry : Enemies)
	{
		if (auto* Enemy = Entry.Get()) { Enemy->FindComponentByClass<URLEnemyAttackComponent>()->SetSupportAttackRate(1); }
	}
	for (AStaticMeshActor* Actor : Visuals) { if (IsValid(Actor)) { Actor->Destroy(); } }
	Visuals.Empty(); Enemies.Empty(); Support.Reset(); Settings = nullptr;
	Elapsed = 0; DamageCooldown = 0;
}

FText URLArenaVariationComponent::GetStatusText() const
{
	if (!Settings) { return FText::GetEmpty(); }
	const TCHAR* Hint = TEXT("SUPPORT OFFLINE");
	if (Settings->Type == ERLArenaExperimentType::SupportNetwork)
	{
		if (Support.IsValid() && Support->IsPoolActive()) { Hint = TEXT("CYAN MARKER: SUPPORT LINK ACTIVE"); }
	}
	else { Hint = FMath::Fmod(Elapsed, Settings->PhaseDuration) < Settings->WarningDuration ? TEXT("AMBER ZONE: RELOCATE BEFORE ACTIVATION") : TEXT("RED ZONE ACTIVE - CENTER LANE SAFE"); }
	return FText::FromString(Settings->DisplayName.ToString() + TEXT("\n") + Hint);
}
