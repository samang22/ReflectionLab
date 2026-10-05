#include "Framework/GameMode/RLGameModeBase.h"

#include "Combat/RLProjectile.h"
#include "Data/RLDifficultyScheduleDataAsset.h"
#include "Data/RLRunDefinitionDataAsset.h"
#include "Data/RLRunRewardDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/RLEnemySpawner.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/PlayerController/RLPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/RLPlayerCharacter.h"
#include "Player/Components/RLHealthComponent.h"
#include "Player/Components/RLRunRewardComponent.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Player/Components/RLParryProgressionComponent.h"
#endif
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLTutorialRewardTest, "ReflectionLab.Run.TutorialReward",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLTutorialRewardTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Settings = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARLGameModeBase* Mode = World->SpawnActor<ARLGameModeBase>();
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	ARLPlayerCharacter* Player = World->SpawnActor<ARLPlayerCharacter>(
		ARLPlayerCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	if (TestNotNull(TEXT("Game mode"), Mode) && TestNotNull(TEXT("Player controller"), Controller)
		&& TestNotNull(TEXT("Player"), Player))
	{
		Controller->Possess(Player);
		URLRunRewardComponent* TestRewards = Player->GetRunRewardComponent();
		TestRewards->Initialize(Player->GetHealthComponent());
		auto ApplyTestReward = [Mode, TestRewards](ERLRunRewardType Type, float Amount)
		{
			URLRunRewardDataAsset* Reward = NewObject<URLRunRewardDataAsset>(Mode);
			Reward->RewardType = Type;
			Reward->Amount = Amount;
			return TestRewards->TryApplyReward(Reward);
		};
		// This isolated world has not begun play, so register its player explicitly.
		World->AddController(Controller);
		TestTrue(TEXT("Player available to gameplay lookup"),
			UGameplayStatics::GetPlayerCharacter(Mode, 0) == Player);
		Mode->RunDefinition = NewObject<URLRunDefinitionDataAsset>(Mode);
		for (ERLRunRewardType Type : {ERLRunRewardType::WiderArc,
			ERLRunRewardType::ExtendedRange, ERLRunRewardType::PiercingReturn})
		{
			URLRunRewardDataAsset* Reward = NewObject<URLRunRewardDataAsset>(Mode->RunDefinition);
			Reward->RewardType = Type;
			Reward->Amount = 1.0f;
			Reward->Title = FText::FromString(TEXT("Test reward"));
			Mode->RunDefinition->RewardPool.Add(Reward);
		}
		Mode->RunDefinition->Rounds.AddDefaulted_GetRef().bIsTutorial = true;
		Mode->CurrentRoundIndex = 0;
		Mode->bTutorialOnlyMode = true;
		Mode->RunState = ERLRunState::PlayingRound;
		Mode->FinishRound();
		TestTrue(TEXT("Tutorial ends in reward selection"), Mode->IsChoosingReward());
		TestEqual(TEXT("Three practice cards"), Mode->GetRewardChoiceCount(), 3);
		TestFalse(TEXT("Invalid reward rejected"), Mode->SelectReward(-1));
		TestTrue(TEXT("Practice reward applied"), Mode->SelectReward(0));
		TestTrue(TEXT("Selection completes tutorial"), Mode->GetRunState() == ERLRunState::TutorialCompleted);
		TestFalse(TEXT("Duplicate selection rejected"), Mode->SelectReward(0));
		TestRewards->ResetRunRewards();
		const float BaseHealth = Player->GetMaxHealth();
		TestEqual(TEXT("Default max HP"), BaseHealth, 10.0f);
		Player->GetHealthComponent()->InitializeHealth(Player->GetMaxHealth());
		Player->GetHealthComponent()->ApplyDamage(Player->GetMaxHealth() - 5.0f);
		TestTrue(TEXT("Vitality applies through component"), ApplyTestReward(ERLRunRewardType::Vitality, 2.0f));
		TestEqual(TEXT("Vitality increases max HP"), Player->GetMaxHealth(), BaseHealth + 2.0f);
		TestEqual(TEXT("Vitality restores HP"), Player->GetCurrentHealth(), 7.0f);
		auto RegisterSuccessfulParry = [Player](int32 Count, bool bPerfect, bool bClose, bool bOverdrive)
		{
			const FRLParryResult Result{Count, bPerfect, bClose, bOverdrive};
			Player->GetRunRewardComponent()->ApplyPerfectRecovery(Result);
			Player->GetParryProgressionComponent()->RegisterSuccess(Result);
		};
		TestTrue(TEXT("Healing reward applies"), ApplyTestReward(ERLRunRewardType::PerfectRecovery, 1.0f));
		RegisterSuccessfulParry(3, true, false, false);
		TestEqual(TEXT("Multi perfect parry heals once"), Player->GetCurrentHealth(), 8.0f);
		RegisterSuccessfulParry(1, false, false, false);
		TestEqual(TEXT("Normal parry does not heal"), Player->GetCurrentHealth(), 8.0f);
		TestTrue(TEXT("Healing reward stacks"), ApplyTestReward(ERLRunRewardType::PerfectRecovery, 1.0f));
		RegisterSuccessfulParry(1, true, false, false);
		TestEqual(TEXT("Second recovery reward heals two HP"), Player->GetCurrentHealth(), 10.0f);
		TestTrue(TEXT("Healing reward stacks again"), ApplyTestReward(ERLRunRewardType::PerfectRecovery, 1.0f));
		Player->GetHealthComponent()->ApplyDamage(Player->GetCurrentHealth() - 5.0f);
		RegisterSuccessfulParry(1, true, false, false);
		TestEqual(TEXT("Third recovery reward heals three HP"), Player->GetCurrentHealth(), 8.0f);
		Player->RestoreHealth(Player->GetMaxHealth());
		RegisterSuccessfulParry(1, true, false, false);
		TestEqual(TEXT("Healing respects max HP"), Player->GetCurrentHealth(), Player->GetMaxHealth());
		TestRewards->ResetRunRewards();
		TestEqual(TEXT("Reset restores base max HP"), Player->GetMaxHealth(), BaseHealth);
		TestFalse(TEXT("Reset clears recovery reward"), Player->HasPerfectRecoveryReward());
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLShortRoundsTest, "ReflectionLab.Run.ShortRounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLShortRoundsTest::RunTest(const FString& Parameters)
{
	ARLGameModeBase* Mode = GetMutableDefault<ARLGameModeBase>();
	URLRunDefinitionDataAsset* Original = Mode->RunDefinition;
	const bool bWasPrepared = Mode->bShortRoundsPrepared;
	Mode->RunDefinition = DuplicateObject<URLRunDefinitionDataAsset>(Original, GetTransientPackage());
	Mode->bShortRoundsPrepared = false;
	const TArray<FRLRoundDefinition> Before = Mode->RunDefinition->Rounds;
	Mode->PrepareShortRounds();
	int32 OutputIndex = 0;
	int32 MainRounds = 0;
	for (const FRLRoundDefinition& Round : Before)
	{
		const int32 Parts = Round.bIsTutorial ? 1 : 2;
		int32 WaveIndex = 0;
		for (int32 Part = 0; Part < Parts; ++Part)
		{
			if (!TestTrue(TEXT("Split round exists"), Mode->RunDefinition->Rounds.IsValidIndex(OutputIndex)))
			{
				break;
			}
			const FRLRoundDefinition& Result = Mode->RunDefinition->Rounds[OutputIndex++];
			MainRounds += Result.bIsTutorial ? 0 : 1;
			for (const FRLWaveDefinition& Wave : Result.DifficultySchedule->Waves)
			{
				TestEqual(TEXT("Wave order preserved"), Wave.WaveName,
					Round.DifficultySchedule->Waves[WaveIndex++].WaveName);
			}
		}
		TestEqual(TEXT("All waves preserved"), WaveIndex, Round.DifficultySchedule->Waves.Num());
	}
	TestEqual(TEXT("Ten main rounds"), MainRounds, 10);
	const int32 Count = Mode->RunDefinition->Rounds.Num();
	Mode->PrepareShortRounds();
	TestEqual(TEXT("Restart does not split again"), Mode->RunDefinition->Rounds.Num(), Count);
	const bool bWasTutorialOnly = Mode->bTutorialOnlyMode;
	Mode->bTutorialOnlyMode = false;
	Mode->RunDefinition->StartingRoundNumber = 0;
	const int32 NormalStart = Mode->ResolveStartingRoundIndex();
	Mode->RunDefinition->StartingRoundNumber = 10;
	const int32 TenthRoundIndex = Mode->ResolveStartingRoundIndex();
	int32 MainRoundNumber = 0;
	for (int32 Index = 0; Index <= TenthRoundIndex; ++Index)
	{
		MainRoundNumber += Mode->RunDefinition->Rounds[Index].bIsTutorial ? 0 : 1;
	}
	TestEqual(TEXT("Start override selects displayed round ten after splitting"), MainRoundNumber, 10);
	Mode->bTutorialOnlyMode = true;
	const int32 TutorialIndex = Mode->RunDefinition->Rounds.IndexOfByPredicate(
		[](const FRLRoundDefinition& Round) { return Round.bIsTutorial; });
	TestEqual(TEXT("Tutorial mode ignores the main-round override"), Mode->ResolveStartingRoundIndex(), TutorialIndex);
	Mode->bTutorialOnlyMode = false;
	Mode->RunDefinition->StartingRoundNumber = 999;
	AddExpectedError(TEXT("Starting round 999 does not exist"), EAutomationExpectedErrorFlags::Contains, 1);
	TestEqual(TEXT("Invalid start override falls back to normal entry"), Mode->ResolveStartingRoundIndex(), NormalStart);
	Mode->bTutorialOnlyMode = bWasTutorialOnly;
	Mode->RunDefinition = Original;
	Mode->bShortRoundsPrepared = bWasPrepared;
	return true;
}
#endif

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
	PrepareShortRounds();
	if (!RunDefinition || RunDefinition->Rounds.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("RunDefinition is not assigned or contains no rounds."));
		return;
	}

	GetWorldTimerManager().ClearTimer(RoundUpdateTimerHandle);
	GetWorldTimerManager().ClearTimer(IntermissionTimerHandle);
	GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
	PendingRewardChoices.Reset();
	ResetRunRecord();
	RepeatedRoundCount = 0;
	bEndlessModeEntered = false;
	BindPlayerStats();
	if (ARLPlayerCharacter* PlayerCharacter = Cast<ARLPlayerCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		if (URLRunRewardComponent* Rewards = PlayerCharacter->GetRunRewardComponent())
		{
			Rewards->ResetRunRewards();
		}
	}
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

void ARLGameModeBase::PrepareShortRounds()
{
	if (!RunDefinition || bShortRoundsPrepared)
	{
		return;
	}

	// Keep shared editor assets untouched and do not split again on run restart.
	RunDefinition = DuplicateObject<URLRunDefinitionDataAsset>(RunDefinition, this);
	TArray<FRLRoundDefinition> ShortRounds;
	for (const FRLRoundDefinition& Round : RunDefinition->Rounds)
	{
		const URLDifficultyScheduleDataAsset* Schedule = Round.DifficultySchedule;
		if (Round.bIsTutorial || !Schedule || Schedule->Waves.Num() < 2)
		{
			ShortRounds.Add(Round);
			continue;
		}

		const int32 SplitIndex = (Schedule->Waves.Num() + 1) / 2;
		for (int32 Part = 0; Part < 2; ++Part)
		{
			FRLRoundDefinition& ShortRound = ShortRounds.Add_GetRef(Round);
			ShortRound.RoundName = FName(*FString::Printf(TEXT("%s %d"),
				*Round.RoundName.ToString(), Part + 1));
			URLDifficultyScheduleDataAsset* ShortSchedule =
				DuplicateObject<URLDifficultyScheduleDataAsset>(Schedule, RunDefinition,
					MakeUniqueObjectName(RunDefinition, URLDifficultyScheduleDataAsset::StaticClass()));
			ShortSchedule->Waves.Reset();
			const int32 Begin = Part == 0 ? 0 : SplitIndex;
			const int32 End = Part == 0 ? SplitIndex : Schedule->Waves.Num();
			for (int32 Index = Begin; Index < End; ++Index)
			{
				ShortSchedule->Waves.Add(Schedule->Waves[Index]);
			}
			ShortRound.DifficultySchedule = ShortSchedule;
		}
	}
	RunDefinition->Rounds = MoveTemp(ShortRounds);
	bShortRoundsPrepared = true;
	UE_LOG(LogTemp, Display, TEXT("Prepared short rounds: %d rounds including tutorial."),
		RunDefinition->Rounds.Num());
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
	PendingRewardChoices.Reset();
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

	// Restarting tutorial-only mode should not silently switch to the main run.
	UGameplayStatics::OpenLevel(this, CurrentLevelName, true,
		bTutorialOnlyMode ? TEXT("RunMode=Tutorial") : TEXT("RunMode=Main"));
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

bool ARLGameModeBase::IsEndlessRun() const
{
	return RunDefinition && RunDefinition->bRepeatFinalRound && bEndlessModeEntered && !bTutorialOnlyMode;
}

void ARLGameModeBase::ContinueInEndlessMode()
{
	if (RunState != ERLRunState::CampaignCleared || !RunDefinition ||
		!RunDefinition->bRepeatFinalRound || bTutorialOnlyMode ||
		CurrentRoundIndex != RunDefinition->Rounds.Num() - 1 || IsCurrentRoundTutorial())
	{
		return;
	}
	bEndlessModeEntered = true;
	// Preserve upgrades and award the final campaign round's card before round 11.
	BeginRewardSelection();
}

float ARLGameModeBase::GetRoundAttackFrequencyMultiplier() const
{
	if (!RunDefinition || IsCurrentRoundTutorial()) { return 1.0f; }
	const float Growth = FMath::IsFinite(RunDefinition->AttackFrequencyGrowthPerRound)
		? FMath::Clamp(RunDefinition->AttackFrequencyGrowthPerRound, 1.0f, 2.0f) : 1.0f;
	const float Maximum = FMath::IsFinite(RunDefinition->MaxAttackFrequencyMultiplier)
		? FMath::Clamp(RunDefinition->MaxAttackFrequencyMultiplier, 1.0f, 100.0f) : 1.0f;
	// Clamp in logarithmic space before exponentiation to avoid overflow in long runs.
	const double Steps = FMath::Max(0, GetCurrentRoundNumber() - 1);
	return static_cast<float>(FMath::Exp(FMath::Min(
		Steps * FMath::Loge(static_cast<double>(Growth)),
		FMath::Loge(static_cast<double>(Maximum)))));
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
	return RoundNumber + RepeatedRoundCount;
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
	ResetPlayerToArenaCenter();
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

void ARLGameModeBase::ResetPlayerToArenaCenter()
{
	ARLPlayerCharacter* PlayerCharacter = Cast<ARLPlayerCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerCharacter)
	{
		return;
	}

	FVector TargetLocation = ArenaCenterLocation;
	TargetLocation.Z = PlayerCharacter->GetActorLocation().Z;
	PlayerCharacter->TeleportTo(
		TargetLocation,
		PlayerCharacter->GetActorRotation(),
		false,
		true);

	if (UCharacterMovementComponent* MovementComponent = PlayerCharacter->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}
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

	// Balance a per-wave copy so authored schedules and retries never compound
	// the multipliers, including when the final round is repeated.
	FRLWaveDefinition WaveDefinition = Schedule->Waves[WaveIndex];
	if (!RoundDefinition->bIsTutorial)
	{
		WaveDefinition.EnemyCount = static_cast<int32>(FMath::Clamp<int64>(
			static_cast<int64>(WaveDefinition.EnemyCount) * 2, 1, MAX_int32));
		// Round one retains the existing half-frequency baseline. Later rounds
		// accelerate without mutating the schedule or compounding per wave.
		const float AttackTimingScale = 2.0f / GetRoundAttackFrequencyMultiplier();
		WaveDefinition.AttackIntervalMultiplier *= AttackTimingScale;
		WaveDefinition.TimeBetweenShotsMultiplier *= AttackTimingScale;
	}
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
	if (RoundDefinition->bIsTutorial)
	{
		StopEnemySpawners();
		TutorialEnemy.Reset();
		SetTutorialEnemyMovementLocked(false);
		CleanupRoundActors(*RoundDefinition);
		BeginRewardSelection();
		return;
	}

	StopEnemySpawners();
	TutorialEnemy.Reset();
	SetTutorialEnemyMovementLocked(false);
	CleanupRoundActors(*RoundDefinition);
	if (!RoundDefinition->bIsTutorial)
	{
		RoundsCleared = FMath::Max(RoundsCleared, GetCurrentRoundNumber());
		if (ARLPlayerCharacter* Player = Cast<ARLPlayerCharacter>(
			UGameplayStatics::GetPlayerCharacter(this, 0)))
		{
			Player->RestoreHealth(2.0f);
		}
	}

	if (!RunDefinition || (CurrentRoundIndex + 1 >= RunDefinition->Rounds.Num() && !IsEndlessRun()))
	{
		SetRunState(RunDefinition && RunDefinition->bRepeatFinalRound && !bTutorialOnlyMode
			? ERLRunState::CampaignCleared : ERLRunState::RunCompleted);
		return;
	}
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
		if (IsCurrentRoundTutorial())
		{
			SetRunState(ERLRunState::TutorialCompleted);
			return;
		}
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

void ARLGameModeBase::BuildRewardChoices()
{
	PendingRewardChoices.Reset();
	if (!RunDefinition) { return; }
	TArray<URLRunRewardDataAsset*> AvailableRewards;
	TSet<ERLRunRewardType> RegisteredTypes;
	for (URLRunRewardDataAsset* Reward : RunDefinition->RewardPool)
	{
		if (!IsValid(Reward) || !Reward->bEnabled || Reward->Weight <= 0 || !Reward->HasValidEffect() ||
			Reward->Title.IsEmpty() || RegisteredTypes.Contains(Reward->RewardType)) { continue; }
		RegisteredTypes.Add(Reward->RewardType);
		AvailableRewards.Add(Reward);
	}
	while (!AvailableRewards.IsEmpty() && PendingRewardChoices.Num() < 3)
	{
		int64 TotalWeight = 0;
		for (const URLRunRewardDataAsset* Reward : AvailableRewards) { TotalWeight += Reward->Weight; }
		// Use a cumulative draw instead of allocating Weight copies of each asset.
		int64 Ticket = FMath::RandRange(static_cast<int64>(0), TotalWeight - 1);
		for (int32 Index = 0; Index < AvailableRewards.Num(); ++Index)
		{
			Ticket -= AvailableRewards[Index]->Weight;
			if (Ticket < 0)
			{
				PendingRewardChoices.Add(AvailableRewards[Index]);
				// Remove the whole reward so later cards cannot repeat its effect.
				AvailableRewards.RemoveAtSwap(Index, 1, EAllowShrinking::No);
				break;
			}
		}
	}
}

const URLRunRewardDataAsset* ARLGameModeBase::GetRewardChoiceDefinition(int32 ChoiceIndex) const
{
	return PendingRewardChoices.IsValidIndex(ChoiceIndex) && IsValid(PendingRewardChoices[ChoiceIndex])
		? PendingRewardChoices[ChoiceIndex].Get() : nullptr;
}

FText ARLGameModeBase::GetRewardChoiceTitle(int32 ChoiceIndex) const
{
	const URLRunRewardDataAsset* Reward = GetRewardChoiceDefinition(ChoiceIndex);
	return Reward ? Reward->Title : FText::GetEmpty();
}

FText ARLGameModeBase::GetRewardChoiceDescription(int32 ChoiceIndex) const
{
	const URLRunRewardDataAsset* Reward = GetRewardChoiceDefinition(ChoiceIndex);
	return Reward ? Reward->Description : FText::GetEmpty();
}

TOptional<ERLRunRewardType> ARLGameModeBase::GetRewardChoiceType(int32 ChoiceIndex) const
{
	const URLRunRewardDataAsset* Reward = GetRewardChoiceDefinition(ChoiceIndex);
	return Reward ? TOptional<ERLRunRewardType>(Reward->RewardType) : TOptional<ERLRunRewardType>();
}

bool ARLGameModeBase::SelectReward(int32 ChoiceIndex)
{
	if (IsCurrentRoundTutorial())
	{
		const ARLPlayerController* Controller = Cast<ARLPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
		if (Controller && !Controller->IsTutorialRewardChoiceAllowed()) { return false; }
	}
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

	const URLRunRewardDataAsset* SelectedReward = GetRewardChoiceDefinition(ChoiceIndex);
	URLRunRewardComponent* Rewards = PlayerCharacter->GetRunRewardComponent();
	if (!IsValid(Rewards) || !Rewards->TryApplyReward(SelectedReward)) { return false; }
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Selected run reward %d after round %d."),
		static_cast<int32>(SelectedReward->RewardType),
		GetCurrentRoundNumber());
	if (IsCurrentRoundTutorial() && bTutorialOnlyMode)
	{
		PendingRewardChoices.Reset();
		SetRunState(ERLRunState::TutorialCompleted);
	}
	else
	{
		BeginIntermission();
	}
	return true;
}

void ARLGameModeBase::FinishIntermission()
{
	GetWorldTimerManager().ClearTimer(IntermissionTimerHandle);
	IntermissionEndTimeSeconds = 0.0f;
	if (IsEndlessRun() && RunDefinition->Rounds.IsValidIndex(CurrentRoundIndex) &&
		CurrentRoundIndex == RunDefinition->Rounds.Num() - 1 && !IsCurrentRoundTutorial())
	{
		if (RepeatedRoundCount >= MAX_int32 - GetRoundCount())
		{
			SetRunState(ERLRunState::RunCompleted);
			return;
		}
		++RepeatedRoundCount;
		BeginRoundCountdown(CurrentRoundIndex);
		return;
	}
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

ARLExpandingRingAttack* ARLGameModeBase::RequestTutorialRingAttack()
{
	if (!IsCurrentRoundTutorial() || RunState != ERLRunState::PlayingRound) { return nullptr; }
	ARLEnemyCharacter* Enemy = GetTutorialEnemy();
	if (Enemy) { ClearActiveProjectiles(); }
	return Enemy ? Enemy->FireTutorialRingAttack() : nullptr;
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
}

URLProjectileDefinitionDataAsset* ARLGameModeBase::FindTutorialProjectileDefinition(
	ERLProjectileBehavior ProjectileBehavior) const
{
	const FRLRoundDefinition* RoundDefinition = GetCurrentRoundDefinition();
	const URLDifficultyScheduleDataAsset* Schedule = RoundDefinition
		? RoundDefinition->DifficultySchedule
		: nullptr;
	auto FindInSchedule = [ProjectileBehavior](const URLDifficultyScheduleDataAsset* CandidateSchedule)
		-> URLProjectileDefinitionDataAsset*
	{
		if (!CandidateSchedule) { return nullptr; }
		for (const FRLWaveDefinition& WaveDefinition : CandidateSchedule->Waves)
		{
			if (WaveDefinition.DefaultProjectileDefinition &&
				WaveDefinition.DefaultProjectileDefinition->Behavior == ProjectileBehavior)
			{
				return WaveDefinition.DefaultProjectileDefinition;
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
	};
	if (URLProjectileDefinitionDataAsset* Definition = FindInSchedule(Schedule)) { return Definition; }
	// Practice newly introduced shots without changing the tutorial's regular waves.
	if (RunDefinition)
	{
		for (const FRLRoundDefinition& Round : RunDefinition->Rounds)
		{
			if (URLProjectileDefinitionDataAsset* Definition = FindInSchedule(Round.DifficultySchedule))
			{
				return Definition;
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

	if (!bTutorialOnlyMode && RunDefinition->StartingRoundNumber > 0)
	{
		int32 MainRoundNumber = 0;
		for (int32 RoundIndex = 0; RoundIndex < RunDefinition->Rounds.Num(); ++RoundIndex)
		{
			if (!RunDefinition->Rounds[RoundIndex].bIsTutorial &&
				++MainRoundNumber == RunDefinition->StartingRoundNumber)
			{
				return RoundIndex;
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("Starting round %d does not exist; using normal entry."),
			RunDefinition->StartingRoundNumber);
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
