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

void ARLGameModeBase::InitGame(
	const FString& MapName,
	const FString& Options,
	FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	const FString RunMode = UGameplayStatics::ParseOption(Options, TEXT("RunMode"));
	bTutorialOnlyMode = RunMode.Equals(TEXT("Tutorial"), ESearchCase::IgnoreCase);
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
	PendingRewardChoices.Reset();
	const int32 StartingRoundIndex = ResolveStartingRoundIndex();
	if (!RunDefinition->Rounds.IsValidIndex(StartingRoundIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("RunDefinition has no valid starting round."));
		return;
	}
	if (const FRLRoundDefinition* FirstRound = &RunDefinition->Rounds[StartingRoundIndex])
	{
		CleanupRoundActors(*FirstRound);
	}
	BeginRoundCountdown(StartingRoundIndex);
}

void ARLGameModeBase::StopRun()
{
	GetWorldTimerManager().ClearTimer(RoundUpdateTimerHandle);
	GetWorldTimerManager().ClearTimer(IntermissionTimerHandle);
	GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
	StopEnemySpawners();
	SetTutorialEnemyMovementLocked(false);
	CurrentRoundIndex = INDEX_NONE;
	CurrentPhaseIndex = INDEX_NONE;
	CurrentWaveIndex = INDEX_NONE;
	RoundElapsedSeconds = 0.0f;
	IntermissionEndTimeSeconds = 0.0f;
	CountdownEndTimeSeconds = 0.0f;
	CurrentPhaseName = NAME_None;
	CurrentWaveName = NAME_None;
	bWaveTransitionPending = false;
	NextWaveStartTimeSeconds = 0.0f;
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

void ARLGameModeBase::StartMainGame()
{
	if (!GetWorld())
	{
		return;
	}

	const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(this, true));
	if (!CurrentLevelName.IsNone())
	{
		UGameplayStatics::OpenLevel(this, CurrentLevelName, true, TEXT("RunMode=Main"));
	}
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
	if (!RunDefinition)
	{
		return 0;
	}

	int32 RoundCount = 0;
	for (const FRLRoundDefinition& RoundDefinition : RunDefinition->Rounds)
	{
		if (!RoundDefinition.bIsTutorial)
		{
			++RoundCount;
		}
	}
	return RoundCount;
}

int32 ARLGameModeBase::GetCurrentRoundNumber() const
{
	if (!RunDefinition || !RunDefinition->Rounds.IsValidIndex(CurrentRoundIndex))
	{
		return 0;
	}

	int32 RoundNumber = 0;
	for (int32 Index = 0; Index <= CurrentRoundIndex; ++Index)
	{
		if (!RunDefinition->Rounds[Index].bIsTutorial)
		{
			++RoundNumber;
		}
	}
	return RoundNumber;
}

bool ARLGameModeBase::IsCurrentRoundTutorial() const
{
	const FRLRoundDefinition* RoundDefinition = GetCurrentRoundDefinition();
	return RoundDefinition && RoundDefinition->bIsTutorial;
}

void ARLGameModeBase::SetTutorialEnemyMovementLocked(bool bLocked)
{
	if (bTutorialEnemyMovementLocked == bLocked)
	{
		return;
	}

	bTutorialEnemyMovementLocked = bLocked;
	if (!GetWorld())
	{
		return;
	}

	for (TActorIterator<ARLEnemyCharacter> Iterator(GetWorld()); Iterator; ++Iterator)
	{
		if (Iterator->IsPoolActive())
		{
			Iterator->SetTutorialMovementLocked(bTutorialEnemyMovementLocked);
		}
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Tutorial enemy movement lock: %s."),
		bTutorialEnemyMovementLocked ? TEXT("enabled") : TEXT("disabled"));
}

int32 ARLGameModeBase::GetCurrentPhaseCount() const
{
	const FRLRoundDefinition* RoundDefinition = GetCurrentRoundDefinition();
	return RoundDefinition && RoundDefinition->DifficultySchedule
		? RoundDefinition->DifficultySchedule->Phases.Num()
		: 0;
}

int32 ARLGameModeBase::GetCurrentWaveCount() const
{
	if (IsCurrentRoundTutorial())
	{
		return 1;
	}

	const FRLRoundDefinition* RoundDefinition = GetCurrentRoundDefinition();
	return RoundDefinition && RoundDefinition->DifficultySchedule
		? RoundDefinition->DifficultySchedule->Waves.Num()
		: 0;
}

int32 ARLGameModeBase::GetRemainingEnemyCount() const
{
	if (!GetWorld())
	{
		return 0;
	}

	int32 ActiveEnemyCount = 0;
	for (TActorIterator<ARLEnemyCharacter> Iterator(GetWorld()); Iterator; ++Iterator)
	{
		if (Iterator->IsPoolActive())
		{
			++ActiveEnemyCount;
		}
	}
	return ActiveEnemyCount;
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
	SetTutorialEnemyMovementLocked(RunDefinition->Rounds[RoundIndex].bIsTutorial);
	CurrentPhaseIndex = INDEX_NONE;
	CurrentPhaseName = NAME_None;
	CurrentWaveIndex = INDEX_NONE;
	CurrentWaveName = NAME_None;
	bWaveTransitionPending = false;
	NextWaveStartTimeSeconds = 0.0f;
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
	SetTutorialEnemyMovementLocked(RunDefinition->Rounds[RoundIndex].bIsTutorial);
	CurrentPhaseIndex = INDEX_NONE;
	CurrentPhaseName = NAME_None;
	CurrentWaveIndex = INDEX_NONE;
	CurrentWaveName = NAME_None;
	TutorialEnemy.Reset();
	bWaveTransitionPending = false;
	NextWaveStartTimeSeconds = 0.0f;
	RoundElapsedSeconds = 0.0f;
	IntermissionEndTimeSeconds = 0.0f;
	CountdownEndTimeSeconds = 0.0f;
	RoundStartTimeSeconds = GetWorld()->GetTimeSeconds();
	BindPlayerStats();
	SetRunState(ERLRunState::PlayingRound);
	BeginWave(0);

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

	RoundElapsedSeconds = FMath::Max(
		0.0f,
		GetWorld()->GetTimeSeconds() - RoundStartTimeSeconds);

	const URLDifficultyScheduleDataAsset* Schedule = RoundDefinition->DifficultySchedule;
	if (!Schedule || Schedule->Waves.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Current round has no wave definitions."));
		FinishRound();
		return;
	}

	if (bWaveTransitionPending)
	{
		if (GetWorld()->GetTimeSeconds() >= NextWaveStartTimeSeconds)
		{
			bWaveTransitionPending = false;
			NextWaveStartTimeSeconds = 0.0f;
			BeginWave(CurrentWaveIndex + 1);
		}
		return;
	}

	if (GetRemainingEnemyCount() > 0)
	{
		return;
	}

	ClearActiveProjectiles();
	if (RoundDefinition->bIsTutorial)
	{
		FinishRound();
		return;
	}

	if (CurrentWaveIndex + 1 >= Schedule->Waves.Num())
	{
		FinishRound();
		return;
	}

	const FRLWaveDefinition& CompletedWave = Schedule->Waves[CurrentWaveIndex];
	bWaveTransitionPending = true;
	NextWaveStartTimeSeconds = GetWorld()->GetTimeSeconds() +
		FMath::Max(0.0f, CompletedWave.NextWaveDelaySeconds);
}

void ARLGameModeBase::BeginWave(int32 WaveIndex)
{
	const FRLRoundDefinition* RoundDefinition = GetCurrentRoundDefinition();
	const URLDifficultyScheduleDataAsset* Schedule = RoundDefinition
		? RoundDefinition->DifficultySchedule
		: nullptr;
	if (!Schedule || !Schedule->Waves.IsValidIndex(WaveIndex))
	{
		FinishRound();
		return;
	}

	const FRLWaveDefinition& WaveDefinition = Schedule->Waves[WaveIndex];
	ApplyWaveDefinition(WaveDefinition, WaveIndex);

	TArray<ARLEnemySpawner*> ValidSpawners;
	for (const TWeakObjectPtr<ARLEnemySpawner>& SpawnerPtr : EnemySpawners)
	{
		if (ARLEnemySpawner* Spawner = SpawnerPtr.Get())
		{
			Spawner->ConfigureWave(WaveDefinition);
			ValidSpawners.Add(Spawner);
		}
	}

	if (ValidSpawners.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot begin wave: no enemy spawners are available."));
		return;
	}

	const bool bTutorialRound = RoundDefinition->bIsTutorial;
	const int32 RequestedEnemyCount = bTutorialRound
		? 1
		: FMath::Max(1, WaveDefinition.EnemyCount);
	int32 SpawnedEnemyCount = 0;
	for (int32 EnemyIndex = 0; EnemyIndex < RequestedEnemyCount; ++EnemyIndex)
	{
		bool bSpawned = false;
		for (int32 Attempt = 0; Attempt < ValidSpawners.Num(); ++Attempt)
		{
			const int32 SpawnerIndex = (EnemyIndex + Attempt) % ValidSpawners.Num();
			if (ARLEnemyCharacter* SpawnedEnemy = ValidSpawners[SpawnerIndex]->SpawnEnemy())
			{
				++SpawnedEnemyCount;
				if (bTutorialRound)
				{
					TutorialEnemy = SpawnedEnemy;
					SpawnedEnemy->SetTutorialCombatControlled(true);
					SpawnedEnemy->SetTutorialInvulnerable(true);
				}
				bSpawned = true;
				break;
			}
		}
		if (!bSpawned)
		{
			break;
		}
	}

	if (SpawnedEnemyCount != RequestedEnemyCount)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Wave %d requested %d enemies but spawned %d."),
			WaveIndex + 1,
			RequestedEnemyCount,
			SpawnedEnemyCount);
	}
}

void ARLGameModeBase::FinishRound()
{
	GetWorldTimerManager().ClearTimer(RoundUpdateTimerHandle);
	bWaveTransitionPending = false;
	NextWaveStartTimeSeconds = 0.0f;
	const FRLRoundDefinition* RoundDefinition = GetCurrentRoundDefinition();
	if (!RoundDefinition)
	{
		SetRunState(ERLRunState::RunCompleted);
		return;
	}
	if (RoundDefinition->bIsTutorial && bTutorialOnlyMode)
	{
		StopEnemySpawners();
		TutorialEnemy.Reset();
		SetTutorialEnemyMovementLocked(false);
		CleanupRoundActors(*RoundDefinition);
		SetRunState(ERLRunState::TutorialCompleted);
		return;
	}

	StopEnemySpawners();
	TutorialEnemy.Reset();
	SetTutorialEnemyMovementLocked(false);
	CleanupRoundActors(*RoundDefinition);
	if (!RoundDefinition->bIsTutorial)
	{
		RoundsCleared = FMath::Max(RoundsCleared, GetCurrentRoundNumber());
	}

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

void ARLGameModeBase::ApplyWaveDefinition(
	const FRLWaveDefinition& WaveDefinition,
	int32 WaveIndex)
{
	CurrentWaveIndex = WaveIndex;
	CurrentWaveName = WaveDefinition.WaveName;
	OnWaveChanged.Broadcast(CurrentRoundIndex, CurrentWaveIndex, CurrentWaveName);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Round %d began wave %d (%s) with %d enemies."),
		CurrentRoundIndex + 1,
		CurrentWaveIndex + 1,
		*CurrentWaveName.ToString(),
		IsCurrentRoundTutorial() ? 1 : FMath::Max(1, WaveDefinition.EnemyCount));
}

bool ARLGameModeBase::RequestTutorialProjectile(
	ERLProjectileBehavior ProjectileBehavior)
{
	if (!IsCurrentRoundTutorial() || RunState != ERLRunState::PlayingRound)
	{
		return false;
	}

	ARLEnemyCharacter* Enemy = GetTutorialEnemy();
	URLProjectileDefinitionDataAsset* ProjectileDefinition =
		FindTutorialProjectileDefinition(ProjectileBehavior);
	if (!Enemy || !ProjectileDefinition)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Tutorial projectile request failed. Enemy=%s Definition=%s Behavior=%d."),
			Enemy ? TEXT("valid") : TEXT("missing"),
			ProjectileDefinition ? TEXT("valid") : TEXT("missing"),
			static_cast<int32>(ProjectileBehavior));
		return false;
	}

	const bool bFired = Enemy->FireTutorialProjectile(ProjectileDefinition);
	if (bFired)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("Tutorial enemy fired projectile behavior %d."),
			static_cast<int32>(ProjectileBehavior));
	}
	return bFired;
}

void ARLGameModeBase::CompleteTutorialCombatIntroduction()
{
	if (!IsCurrentRoundTutorial())
	{
		return;
	}

	if (ARLEnemyCharacter* Enemy = GetTutorialEnemy())
	{
		ClearActiveProjectiles();
		Enemy->SetTutorialInvulnerable(false);
		Enemy->SetTutorialCombatControlled(false);
		UE_LOG(LogTemp, Display, TEXT("Tutorial enemy is now vulnerable."));
	}
}

ARLEnemyCharacter* ARLGameModeBase::GetTutorialEnemy() const
{
	if (ARLEnemyCharacter* Enemy = TutorialEnemy.Get();
		Enemy && Enemy->IsPoolActive())
	{
		return Enemy;
	}

	if (!GetWorld())
	{
		return nullptr;
	}

	for (TActorIterator<ARLEnemyCharacter> Iterator(GetWorld()); Iterator; ++Iterator)
	{
		if (Iterator->IsPoolActive())
		{
			return *Iterator;
		}
	}
	return nullptr;
	if (!RoundDefinition->bIsTutorial)
	{
		BeginRewardSelection();
		return;
	}
	BeginIntermission();
}

void ARLGameModeBase::BeginRewardSelection()
{
	BuildRewardChoices();
	if (PendingRewardChoices.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Reward selection skipped: insufficient reward choices."));
		BeginIntermission();
		return;
	}

	SetRunState(ERLRunState::RewardSelection);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Round %d cleared. Awaiting reward selection."),
		GetCurrentRoundNumber());
}

void ARLGameModeBase::BeginIntermission()
{
	PendingRewardChoices.Reset();
	const FRLRoundDefinition* RoundDefinition = GetCurrentRoundDefinition();
	if (!RoundDefinition)
	{
		SetRunState(ERLRunState::RunCompleted);
		return;
	}
}

URLProjectileDefinitionDataAsset* ARLGameModeBase::FindTutorialProjectileDefinition(
	ERLProjectileBehavior ProjectileBehavior) const
{
	const FRLRoundDefinition* RoundDefinition = GetCurrentRoundDefinition();
	const URLDifficultyScheduleDataAsset* Schedule = RoundDefinition
		? RoundDefinition->DifficultySchedule
		: nullptr;
	if (!Schedule)
	{
		return nullptr;
	}

	for (const FRLWaveDefinition& WaveDefinition : Schedule->Waves)
	{
		if (WaveDefinition.DefaultProjectileDefinition &&
			WaveDefinition.DefaultProjectileDefinition->Behavior == ProjectileBehavior)
		{
			return WaveDefinition.DefaultProjectileDefinition;
		}
void ARLGameModeBase::BuildRewardChoices()
{
	PendingRewardChoices.Reset();
	static constexpr ERLRunRewardType RewardRotation[] = {
		ERLRunRewardType::WiderArc,
		ERLRunRewardType::ExtendedRange,
		ERLRunRewardType::PiercingReturn,
		ERLRunRewardType::PerfectFocus,
		ERLRunRewardType::VelocityDrive,
		ERLRunRewardType::CloseCall,
	};

	const int32 RewardCount = UE_ARRAY_COUNT(RewardRotation);
	const int32 StartIndex = FMath::Abs(RoundsCleared - 1) % RewardCount;
	for (int32 Offset = 0; Offset < 3; ++Offset)
	{
		PendingRewardChoices.Add(RewardRotation[(StartIndex + Offset) % RewardCount]);
	}
}

FText ARLGameModeBase::GetRewardChoiceTitle(int32 ChoiceIndex) const
{
	if (!PendingRewardChoices.IsValidIndex(ChoiceIndex))
	{
		return FText::GetEmpty();
	}

	switch (PendingRewardChoices[ChoiceIndex])
	{
	case ERLRunRewardType::WiderArc:
		return FText::FromString(TEXT("WIDE SWING"));
	case ERLRunRewardType::ExtendedRange:
		return FText::FromString(TEXT("LONG REACH"));
	case ERLRunRewardType::PiercingReturn:
		return FText::FromString(TEXT("PIERCING RETURN"));
	case ERLRunRewardType::PerfectFocus:
		return FText::FromString(TEXT("PERFECT VOLLEY"));
	case ERLRunRewardType::VelocityDrive:
		return FText::FromString(TEXT("VELOCITY DRIVE"));
	case ERLRunRewardType::CloseCall:
		return FText::FromString(TEXT("CLOSE CALL"));
	default:
		return FText::GetEmpty();
	}
}

FText ARLGameModeBase::GetRewardChoiceDescription(int32 ChoiceIndex) const
{
	if (!PendingRewardChoices.IsValidIndex(ChoiceIndex))
	{
		return FText::GetEmpty();
	}

	switch (PendingRewardChoices[ChoiceIndex])
	{
	case ERLRunRewardType::WiderArc:
		return FText::FromString(TEXT("Parry angle +12 degrees"));
	case ERLRunRewardType::ExtendedRange:
		return FText::FromString(TEXT("Parry range +18%"));
	case ERLRunRewardType::PiercingReturn:
		return FText::FromString(TEXT("Reflected projectile pierce +1"));
	case ERLRunRewardType::PerfectFocus:
		return FText::FromString(TEXT("Perfect parry split projectile +1"));
	case ERLRunRewardType::VelocityDrive:
		return FText::FromString(TEXT("Maximum reflected speed +0.2x"));
	case ERLRunRewardType::CloseCall:
		return FText::FromString(TEXT("Close-range parry zone +12 cm"));
	default:
		return FText::GetEmpty();
	}
}

TOptional<ERLRunRewardType> ARLGameModeBase::GetRewardChoiceType(int32 ChoiceIndex) const
{
	if (!PendingRewardChoices.IsValidIndex(ChoiceIndex))
	{
		return TOptional<ERLRunRewardType>();
	}

	return PendingRewardChoices[ChoiceIndex];
}

bool ARLGameModeBase::SelectReward(int32 ChoiceIndex)
{
	if (RunState != ERLRunState::RewardSelection ||
		!PendingRewardChoices.IsValidIndex(ChoiceIndex))
	{
		return false;
	}

	ARLPlayerCharacter* PlayerCharacter = Cast<ARLPlayerCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerCharacter)
	{
		return false;
	}

	const ERLRunRewardType SelectedReward = PendingRewardChoices[ChoiceIndex];
	PlayerCharacter->ApplyRunReward(SelectedReward);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Selected run reward %d after round %d."),
		static_cast<int32>(SelectedReward),
		GetCurrentRoundNumber());
	BeginIntermission();
	return true;
}

		for (const FRLProjectileSpawnRule& Rule : WaveDefinition.ProjectileRules)
		{
			if (Rule.ProjectileDefinition &&
				Rule.ProjectileDefinition->Behavior == ProjectileBehavior)
			{
				return Rule.ProjectileDefinition;
			}
		}
	}
	return nullptr;
}

void ARLGameModeBase::ClearActiveProjectiles()
{
	if (!GetWorld())
	{
		return;
	}

	for (TActorIterator<ARLProjectile> Iterator(GetWorld()); Iterator; ++Iterator)
	{
		if (Iterator->IsPoolActive())
		{
			Iterator->ReturnToPool();
		}
	}
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
	if ((RunState == ERLRunState::TutorialCompleted ||
		RunState == ERLRunState::RunCompleted || RunState == ERLRunState::GameOver) &&
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

int32 ARLGameModeBase::ResolveStartingRoundIndex() const
{
	if (!RunDefinition)
	{
		return INDEX_NONE;
	}

	for (int32 RoundIndex = 0; RoundIndex < RunDefinition->Rounds.Num(); ++RoundIndex)
	{
		if (RunDefinition->Rounds[RoundIndex].bIsTutorial == bTutorialOnlyMode)
		{
			return RoundIndex;
		}
	}
	return INDEX_NONE;
}
