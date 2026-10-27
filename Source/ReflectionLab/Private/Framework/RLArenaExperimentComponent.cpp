#include "Framework/RLArenaExperimentComponent.h"
#include "Framework/RLCrossfireExperimentComponent.h"
#include "Framework/RLArenaVariationComponent.h"
#include "Framework/RLBallisticsExperimentComponent.h"
#include "Framework/RLMirrorArenaComponent.h"
#include "Framework/GameMode/RLGameModeBase.h"
#include "Data/RLArenaExperimentDataAsset.h"

URLArenaExperimentComponent::URLArenaExperimentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void URLArenaExperimentComponent::Configure(URLArenaExperimentDataAsset* Data)
{
	// Persistent arenas retain camera/player position across waves of the same round.
	if (Data && Settings == Data && (Data->Type == ERLArenaExperimentType::MirrorArena ||
		(Data->Type == ERLArenaExperimentType::AccelerationField && Data->Acceleration.bEnableReturnBarriers)))
	{
		return;
	}
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
	if (!Crossfire) { Crossfire = NewObject<URLCrossfireExperimentComponent>(GetOwner()); Crossfire->RegisterComponent(); }
	Crossfire->Configure(Data);
	if (Crossfire->HasActiveExperiment()) { Settings = Data; SetComponentTickEnabled(true); }
}

void URLArenaExperimentComponent::ResetExperiment()
{
	if (Crossfire) { Crossfire->ResetExperiment(); }
	if (MirrorArena) { MirrorArena->ResetArena(); }
	if (Ballistics) { Ballistics->ResetExperiment(); }
	if (Variation) { Variation->ResetVariation(); }
	Settings = nullptr;
	SetComponentTickEnabled(false);
}

void URLArenaExperimentComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	ResetExperiment();
	Super::EndPlay(Reason);
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
	Crossfire->AdvanceExperiment(DeltaTime);
}

bool URLArenaExperimentComponent::FindTeamSpawn(int32 Index, int32 Count, FTransform& OutTransform) const
{
	if (!Settings) { return false; }
	if (Settings->Type == ERLArenaExperimentType::MirrorArena) { return MirrorArena->FindSpawn(Index,OutTransform); }
	if (Settings->Type == ERLArenaExperimentType::AccelerationField) { return Ballistics->FindBarrierSpawn(Index, Count, OutTransform); }
	return Settings->Type == ERLArenaExperimentType::Crossfire && Crossfire
		&& Crossfire->FindTeamSpawn(Index, Count, OutTransform);
}

void URLArenaExperimentComponent::RegisterEnemy(ARLEnemyCharacter* Enemy, int32 Index)
{
	if (!Settings || !Enemy) { return; }
	if (Settings->Type == ERLArenaExperimentType::MirrorArena) { MirrorArena->RegisterEnemy(Enemy); return; }
	if (Settings->Type == ERLArenaExperimentType::AccelerationField || Settings->Type == ERLArenaExperimentType::RefractionGate) { Ballistics->RegisterEnemy(Enemy); return; }
	if (Settings->Type != ERLArenaExperimentType::Crossfire) { Variation->RegisterEnemy(Enemy, Index); return; }
	Crossfire->RegisterEnemy(Enemy, Index);
}

FText URLArenaExperimentComponent::GetStatusText() const
{
	if (!Settings) { return FText::GetEmpty(); }
	if (Settings->Type == ERLArenaExperimentType::MirrorArena) { return MirrorArena->GetStatusText(); }
	if (Settings->Type == ERLArenaExperimentType::AccelerationField || Settings->Type == ERLArenaExperimentType::RefractionGate) { return Ballistics->GetStatusText(); }
	if (Settings->Type != ERLArenaExperimentType::Crossfire) { return Variation->GetStatusText(); }
	return Crossfire->GetStatusText();
}
