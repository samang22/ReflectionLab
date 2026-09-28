#include "Framework/GameMode/RLGameModeBase.h"

#include "Combat/RLProjectile.h"
#include "Data/RLDifficultyScheduleDataAsset.h"
#include "Data/RLRunDefinitionDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/RLEnemySpawner.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Player/RLPlayerCharacter.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ARLGameModeBase::ARLGameModeBase()
{
	static ConstructorHelpers::FObjectFinder<URLRunDefinitionDataAsset> RunDefinitionFinder(
		TEXT("/Game/ReflectionLab/Data/Difficulty/DA_DefaultRun.DA_DefaultRun"));
	if (RunDefinitionFinder.Succeeded())
	{
		RunDefinition = RunDefinitionFinder.Object;
	}
}

void ARLGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	CacheEnemySpawners();

	if (bAutoStartRun)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::StartRun);
	}
}

void ARLGameModeBase::StartRun()
{
	if (!RunDefinition || RunDefinition->Rounds.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("RunDefinition is not assigned or contains no rounds."));
		return;
	}

	GetWorldTimerManager().ClearTimer(RoundUpdateTimerHandle);
	GetWorldTimerManager().ClearTimer(IntermissionTimerHandle);
	GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
	ResetRunRecord();
	BindPlayerStats();
	BeginRoundCountdown(0);
}

void ARLGameModeBase::StopRun()
{
	GetWorldTimerManager().ClearTimer(RoundUpdateTimerHandle);
	GetWorldTimerManager().ClearTimer(IntermissionTimerHandle);
	GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
	StopEnemySpawners();
	CurrentRoundIndex = INDEX_NONE;
	CurrentPhaseIndex = INDEX_NONE;
	RoundElapsedSeconds = 0.0f;
	IntermissionEndTimeSeconds = 0.0f;
	CountdownEndTimeSeconds = 0.0f;
	CurrentPhaseName = NAME_None;
	SetRunState(ERLRunState::Waiting);
}

void ARLGameModeBase::RestartRun()
{
	if (!GetWorld())
	{
		return;
	}

	const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(this, true));
	if (CurrentLevelName.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot restart run: current level name is invalid."));
		return;
	}

	UGameplayStatics::OpenLevel(this, CurrentLevelName);
}

void ARLGameModeBase::ReturnToMainMenu()
{
	if (MainMenuLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot return to main menu: MainMenuLevelName is not configured."));
		return;
	}

	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}

void ARLGameModeBase::NotifyPlayerDied()
{
	if (RunState != ERLRunState::PlayingRound)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(RoundUpdateTimerHandle);
	GetWorldTimerManager().ClearTimer(IntermissionTimerHandle);
	GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
	StopEnemySpawners();
	if (const FRLRoundDefinition* RoundDefinition = GetCurrentRoundDefinition())
	{
		CleanupRoundActors(*RoundDefinition);
	}
	SetRunState(ERLRunState::GameOver);
}

float ARLGameModeBase::GetRoundRemainingSeconds() const
{
	const FRLRoundDefinition* RoundDefinition = GetCurrentRoundDefinition();
	return RoundDefinition
		? FMath::Max(0.0f, RoundDefinition->DurationSeconds - RoundElapsedSeconds)
		: 0.0f;
}

float ARLGameModeBase::GetTotalRunElapsedSeconds() const
{
	if (!GetWorld() || RunStartTimeSeconds < 0.0f)
	{
		return 0.0f;
	}

	const float EndTime = RunEndTimeSeconds > 0.0f
		? RunEndTimeSeconds
		: GetWorld()->GetTimeSeconds();
	return FMath::Max(0.0f, EndTime - RunStartTimeSeconds);
}

int32 ARLGameModeBase::GetRoundCount() const
{
	return RunDefinition ? RunDefinition->Rounds.Num() : 0;
}

int32 ARLGameModeBase::GetCurrentPhaseCount() const
{
	const FRLRoundDefinition* RoundDefinition = GetCurrentRoundDefinition();
	return RoundDefinition && RoundDefinition->DifficultySchedule
		? RoundDefinition->DifficultySchedule->Phases.Num()
		: 0;
}

float ARLGameModeBase::GetNextPhaseRemainingSeconds() const
{
	if (RunState != ERLRunState::PlayingRound)
	{
		return -1.0f;
	}

	const FRLRoundDefinition* RoundDefinition = GetCurrentRoundDefinition();
	const URLDifficultyScheduleDataAsset* Schedule = RoundDefinition
		? RoundDefinition->DifficultySchedule
		: nullptr;
	if (!Schedule)
	{
		return -1.0f;
	}

	const int32 NextPhaseIndex = CurrentPhaseIndex == INDEX_NONE
		? 0
		: CurrentPhaseIndex + 1;
	if (!Schedule->Phases.IsValidIndex(NextPhaseIndex))
	{
		return -1.0f;
	}

	return FMath::Max(
		0.0f,
		Schedule->Phases[NextPhaseIndex].StartTimeSeconds - RoundElapsedSeconds);
}

float ARLGameModeBase::GetIntermissionRemainingSeconds() const
{
	return RunState == ERLRunState::Intermission && GetWorld()
		? FMath::Max(0.0f, IntermissionEndTimeSeconds - GetWorld()->GetTimeSeconds())
		: 0.0f;
}

float ARLGameModeBase::GetCountdownRemainingSeconds() const
{
	return RunState == ERLRunState::Countdown && GetWorld()
		? FMath::Max(0.0f, CountdownEndTimeSeconds - GetWorld()->GetTimeSeconds())
		: 0.0f;
}

void ARLGameModeBase::BeginRoundCountdown(int32 RoundIndex)
{
	if (!RunDefinition || !RunDefinition->Rounds.IsValidIndex(RoundIndex) || !GetWorld())
	{
		SetRunState(ERLRunState::RunCompleted);
		return;
	}

	CacheEnemySpawners();
	StopEnemySpawners();
	CurrentRoundIndex = RoundIndex;
	CurrentPhaseIndex = INDEX_NONE;
	CurrentPhaseName = NAME_None;
	RoundElapsedSeconds = 0.0f;
	IntermissionEndTimeSeconds = 0.0f;
	const float CountdownDuration = FMath::Max(0.0f, RoundCountdownDuration);
	CountdownEndTimeSeconds = GetWorld()->GetTimeSeconds() + CountdownDuration;
	SetRunState(ERLRunState::Countdown);

	if (CountdownDuration <= KINDA_SMALL_NUMBER)
	{
		FinishRoundCountdown();
		return;
	}

	GetWorldTimerManager().SetTimer(
		CountdownTimerHandle,
		this,
		&ThisClass::FinishRoundCountdown,
		CountdownDuration,
		false);
}

void ARLGameModeBase::FinishRoundCountdown()
{
	GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
	CountdownEndTimeSeconds = 0.0f;
	StartRound(CurrentRoundIndex);
}

void ARLGameModeBase::StartRound(int32 RoundIndex)
{
	if (!RunDefinition || !RunDefinition->Rounds.IsValidIndex(RoundIndex) || !GetWorld())
	{
		SetRunState(ERLRunState::RunCompleted);
		return;
	}

	CacheEnemySpawners();
	CurrentRoundIndex = RoundIndex;
	CurrentPhaseIndex = INDEX_NONE;
	CurrentPhaseName = NAME_None;
	RoundElapsedSeconds = 0.0f;
	IntermissionEndTimeSeconds = 0.0f;
	CountdownEndTimeSeconds = 0.0f;
	RoundStartTimeSeconds = GetWorld()->GetTimeSeconds();
	BindPlayerStats();
	SetRunState(ERLRunState::PlayingRound);
	UpdateRound();

	GetWorldTimerManager().SetTimer(
		RoundUpdateTimerHandle,
		this,
		&ThisClass::UpdateRound,
		FMath::Max(0.05f, RunUpdateInterval),
		true);
}

void ARLGameModeBase::UpdateRound()
{
	if (RunState != ERLRunState::PlayingRound || !GetWorld())
	{
		return;
	}

	const FRLRoundDefinition* RoundDefinition = GetCurrentRoundDefinition();
	if (!RoundDefinition)
	{
		FinishRound();
		return;
	}

	const float PreviousElapsedSeconds = RoundElapsedSeconds;
	RoundElapsedSeconds = FMath::Max(
		0.0f,
		GetWorld()->GetTimeSeconds() - RoundStartTimeSeconds);
	const float RoundDuration = FMath::Max(1.0f, RoundDefinition->DurationSeconds);
	if (RoundElapsedSeconds >= RoundDuration)
	{
		if (PreviousElapsedSeconds < RoundDuration)
		{
			StopEnemySpawners();
			UE_LOG(
				LogTemp,
				Display,
				TEXT("Round %d time limit reached. Waiting for remaining enemies to be cleared."),
				CurrentRoundIndex + 1);
		}

		bool bHasActiveEnemies = false;
		for (TActorIterator<ARLEnemyCharacter> Iterator(GetWorld()); Iterator; ++Iterator)
		{
			if (Iterator->IsPoolActive())
			{
				bHasActiveEnemies = true;
				break;
			}
		}

		if (!bHasActiveEnemies)
		{
			FinishRound();
		}
		return;
	}

	if (const URLDifficultyScheduleDataAsset* Schedule = RoundDefinition->DifficultySchedule)
	{
		int32 PhaseIndex = INDEX_NONE;
		if (const FRLDifficultyPhase* Phase =
			Schedule->FindPhaseAtTime(RoundElapsedSeconds, PhaseIndex))
		{
			if (PhaseIndex != CurrentPhaseIndex)
			{
				ApplyDifficultyPhase(*Phase, PhaseIndex);
			}
		}
	}
}

void ARLGameModeBase::FinishRound()
{
	GetWorldTimerManager().ClearTimer(RoundUpdateTimerHandle);
	const FRLRoundDefinition* RoundDefinition = GetCurrentRoundDefinition();
	if (!RoundDefinition)
	{
		SetRunState(ERLRunState::RunCompleted);
		return;
	}

	StopEnemySpawners();
	CleanupRoundActors(*RoundDefinition);
	RoundsCleared = FMath::Max(RoundsCleared, CurrentRoundIndex + 1);

	if (!RunDefinition || CurrentRoundIndex + 1 >= RunDefinition->Rounds.Num())
	{
		SetRunState(ERLRunState::RunCompleted);
		return;
	}

	SetRunState(ERLRunState::Intermission);
	const float IntermissionDuration =
		FMath::Max(0.0f, RoundDefinition->IntermissionDurationSeconds);
	IntermissionEndTimeSeconds = GetWorld()
		? GetWorld()->GetTimeSeconds() + IntermissionDuration
		: 0.0f;
	if (IntermissionDuration <= KINDA_SMALL_NUMBER)
	{
		FinishIntermission();
		return;
	}

	GetWorldTimerManager().SetTimer(
		IntermissionTimerHandle,
		this,
		&ThisClass::FinishIntermission,
		IntermissionDuration,
		false);
}

void ARLGameModeBase::FinishIntermission()
{
	GetWorldTimerManager().ClearTimer(IntermissionTimerHandle);
	IntermissionEndTimeSeconds = 0.0f;
	BeginRoundCountdown(CurrentRoundIndex + 1);
}

void ARLGameModeBase::ApplyDifficultyPhase(
	const FRLDifficultyPhase& DifficultyPhase,
	int32 PhaseIndex)
{
	CurrentPhaseIndex = PhaseIndex;
	CurrentPhaseName = DifficultyPhase.PhaseName;
	for (const TWeakObjectPtr<ARLEnemySpawner>& SpawnerPtr : EnemySpawners)
	{
		if (ARLEnemySpawner* Spawner = SpawnerPtr.Get())
		{
			Spawner->ApplyDifficultyPhase(DifficultyPhase);
		}
	}

	OnDifficultyPhaseChanged.Broadcast(
		CurrentRoundIndex,
		CurrentPhaseIndex,
		DifficultyPhase.PhaseName);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Round %d entered difficulty phase %d (%s)."),
		CurrentRoundIndex + 1,
		CurrentPhaseIndex + 1,
		*DifficultyPhase.PhaseName.ToString());
}

void ARLGameModeBase::CacheEnemySpawners()
{
	EnemySpawners.Reset();
	for (TActorIterator<ARLEnemySpawner> Iterator(GetWorld()); Iterator; ++Iterator)
	{
		EnemySpawners.Add(*Iterator);
	}
}

void ARLGameModeBase::StopEnemySpawners()
{
	for (const TWeakObjectPtr<ARLEnemySpawner>& SpawnerPtr : EnemySpawners)
	{
		if (ARLEnemySpawner* Spawner = SpawnerPtr.Get())
		{
			Spawner->StopSpawning();
		}
	}
}

void ARLGameModeBase::CleanupRoundActors(const FRLRoundDefinition& RoundDefinition)
{
	if (RoundDefinition.bClearProjectilesOnComplete)
	{
		for (TActorIterator<ARLProjectile> Iterator(GetWorld()); Iterator; ++Iterator)
		{
			Iterator->ReturnToPool();
		}
	}

	if (RoundDefinition.bClearEnemiesOnComplete)
	{
		for (TActorIterator<ARLEnemyCharacter> Iterator(GetWorld()); Iterator; ++Iterator)
		{
			if (Iterator->IsPoolActive())
			{
				Iterator->ReturnToPool();
			}
		}
	}
}

void ARLGameModeBase::BindPlayerStats()
{
	ARLPlayerCharacter* PlayerCharacter = Cast<ARLPlayerCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->OnParryChainChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleParryChainChanged);
	BestParryCombo = FMath::Max(
		BestParryCombo,
		PlayerCharacter->GetParryComboCount());
}

void ARLGameModeBase::HandleParryChainChanged(int32 ParryChainCount)
{
	BestParryCombo = FMath::Max(BestParryCombo, ParryChainCount);
}

void ARLGameModeBase::ResetRunRecord()
{
	RunStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	RunEndTimeSeconds = 0.0f;
	RoundsCleared = 0;
	BestParryCombo = 0;
}

void ARLGameModeBase::SetRunState(ERLRunState NewState)
{
	if (RunState == NewState)
	{
		return;
	}

	RunState = NewState;
	if ((RunState == ERLRunState::RunCompleted || RunState == ERLRunState::GameOver) &&
		RunEndTimeSeconds <= 0.0f && GetWorld())
	{
		RunEndTimeSeconds = GetWorld()->GetTimeSeconds();
	}
	OnRunStateChanged.Broadcast(RunState, CurrentRoundIndex);
}

const FRLRoundDefinition* ARLGameModeBase::GetCurrentRoundDefinition() const
{
	return RunDefinition && RunDefinition->Rounds.IsValidIndex(CurrentRoundIndex)
		? &RunDefinition->Rounds[CurrentRoundIndex]
		: nullptr;
}
