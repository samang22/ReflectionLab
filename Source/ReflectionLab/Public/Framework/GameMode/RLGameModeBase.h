#pragma once

#include "CoreMinimal.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Data/RLRunRewardTypes.h"
#include "GameFramework/GameModeBase.h"
#include "RLGameModeBase.generated.h"

class ARLEnemySpawner;
class ARLEnemyCharacter;
class ARLExpandingRingAttack;
class URLRunDefinitionDataAsset;
struct FRLDifficultyPhase;
struct FRLWaveDefinition;
struct FRLRoundDefinition;

UENUM(BlueprintType)
enum class ERLRunState : uint8
{
	Waiting,
	Countdown,
	PlayingRound,
	RewardSelection,
	Intermission,
	TutorialCompleted,
	RunCompleted,
	GameOver,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FRLRunStateChangedSignature,
	ERLRunState,
	NewState,
	int32,
	RoundIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FRLDifficultyPhaseChangedSignature,
	int32,
	RoundIndex,
	int32,
	PhaseIndex,
	FName,
	PhaseName);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FRLWaveChangedSignature,
	int32,
	RoundIndex,
	int32,
	WaveIndex,
	FName,
	WaveName);

UCLASS()
class REFLECTIONLAB_API ARLGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARLGameModeBase();
	virtual void InitGame(
		const FString& MapName,
		const FString& Options,
		FString& ErrorMessage) override;

	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartRun();

	UFUNCTION(BlueprintCallable, Category = "Run")
	void StopRun();

	UFUNCTION(BlueprintCallable, Category = "Run")
	void RestartRun();

	UFUNCTION(BlueprintCallable, Category = "Run")
	void ReturnToMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartMainGame();

	UFUNCTION(BlueprintCallable, Category = "Run")
	void NotifyPlayerDied();

	UFUNCTION(BlueprintPure, Category = "Run")
	ERLRunState GetRunState() const { return RunState; }

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetCurrentRoundIndex() const { return CurrentRoundIndex; }

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetRoundCount() const;

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetCurrentRoundNumber() const;

	UFUNCTION(BlueprintPure, Category = "Run")
	bool IsCurrentRoundTutorial() const;

	UFUNCTION(BlueprintCallable, Category = "Run|Tutorial")
	void SetTutorialEnemyMovementLocked(bool bLocked);

	UFUNCTION(BlueprintPure, Category = "Run|Tutorial")
	bool IsTutorialEnemyMovementLocked() const
	{
		return bTutorialEnemyMovementLocked;
	}

	UFUNCTION(BlueprintCallable, Category = "Run|Tutorial")
	bool RequestTutorialProjectile(ERLProjectileBehavior ProjectileBehavior);

	ARLExpandingRingAttack* RequestTutorialRingAttack();

	UFUNCTION(BlueprintCallable, Category = "Run|Tutorial")
	void CompleteTutorialCombatIntroduction();

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetCurrentPhaseIndex() const { return CurrentPhaseIndex; }

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetCurrentPhaseCount() const;

	UFUNCTION(BlueprintPure, Category = "Run")
	float GetRoundElapsedSeconds() const { return RoundElapsedSeconds; }

	UFUNCTION(BlueprintPure, Category = "Run")
	float GetRoundRemainingSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Run")
	float GetNextPhaseRemainingSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Run")
	float GetIntermissionRemainingSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Run")
	float GetCountdownRemainingSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Run")
	FName GetCurrentPhaseName() const { return CurrentPhaseName; }

	UFUNCTION(BlueprintPure, Category = "Run|Wave")
	int32 GetCurrentWaveIndex() const { return CurrentWaveIndex; }

	UFUNCTION(BlueprintPure, Category = "Run|Wave")
	int32 GetCurrentWaveCount() const;

	UFUNCTION(BlueprintPure, Category = "Run|Wave")
	FName GetCurrentWaveName() const { return CurrentWaveName; }

	UFUNCTION(BlueprintPure, Category = "Run|Wave")
	int32 GetRemainingEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "Run|Results")
	float GetTotalRunElapsedSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Run|Results")
	int32 GetRoundsCleared() const { return RoundsCleared; }

	UFUNCTION(BlueprintPure, Category = "Run|Results")
	int32 GetBestParryCombo() const { return BestParryCombo; }

	UFUNCTION(BlueprintPure, Category = "Run|Navigation")
	bool CanReturnToMainMenu() const { return !MainMenuLevelName.IsNone(); }

	UFUNCTION(BlueprintPure, Category = "Run|Rewards")
	bool IsChoosingReward() const { return RunState == ERLRunState::RewardSelection; }

	UFUNCTION(BlueprintPure, Category = "Run|Rewards")
	int32 GetRewardChoiceCount() const { return PendingRewardChoices.Num(); }

	UFUNCTION(BlueprintPure, Category = "Run|Rewards")
	FText GetRewardChoiceTitle(int32 ChoiceIndex) const;

	UFUNCTION(BlueprintPure, Category = "Run|Rewards")
	FText GetRewardChoiceDescription(int32 ChoiceIndex) const;

	TOptional<ERLRunRewardType> GetRewardChoiceType(int32 ChoiceIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Run|Rewards")
	bool SelectReward(int32 ChoiceIndex);

	UPROPERTY(BlueprintAssignable, Category = "Run")
	FRLRunStateChangedSignature OnRunStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Run")
	FRLDifficultyPhaseChangedSignature OnDifficultyPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Run|Wave")
	FRLWaveChangedSignature OnWaveChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run")
	TObjectPtr<URLRunDefinitionDataAsset> RunDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run")
	bool bAutoStartRun = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run", meta = (ClampMin = "0.05"))
	float RunUpdateInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run", meta = (ClampMin = "0.0"))
	float RoundCountdownDuration = 3.0f;

	// Arena center in world XY coordinates. The player's current height is retained
	// when a new round begins so the destination remains valid on uneven floors.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Arena")
	FVector ArenaCenterLocation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Navigation")
	FName MainMenuLevelName = TEXT("MainMenu");

private:
	UFUNCTION()
	void HandleParryChainChanged(int32 ParryChainCount);

	void BeginRoundCountdown(int32 RoundIndex);
	void ResetPlayerToArenaCenter();
	void FinishRoundCountdown();
	void StartRound(int32 RoundIndex);
	void UpdateRound();
	void BeginWave(int32 WaveIndex);
	void FinishRound();
	void BeginRewardSelection();
	void BeginIntermission();
	void FinishIntermission();
	void BuildRewardChoices();
	void ApplyDifficultyPhase(const FRLDifficultyPhase& DifficultyPhase, int32 PhaseIndex);
	void ApplyWaveDefinition(const FRLWaveDefinition& WaveDefinition, int32 WaveIndex);
	void ClearActiveProjectiles();
	void CacheEnemySpawners();
	void StopEnemySpawners();
	void CleanupRoundActors(const FRLRoundDefinition& RoundDefinition);
	void BindPlayerStats();
	void ResetRunRecord();
	void EnsureExtendedRounds();
	void PrepareShortRounds();
	friend class FRLShortRoundsTest;
	friend class FRLTutorialRewardTest;
	bool bShortRoundsPrepared = false;
	void SetRunState(ERLRunState NewState);
	const FRLRoundDefinition* GetCurrentRoundDefinition() const;
	int32 ResolveStartingRoundIndex() const;
	ARLEnemyCharacter* GetTutorialEnemy() const;
	URLProjectileDefinitionDataAsset* FindTutorialProjectileDefinition(
		ERLProjectileBehavior ProjectileBehavior) const;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ARLEnemySpawner>> EnemySpawners;

	FTimerHandle RoundUpdateTimerHandle;
	FTimerHandle IntermissionTimerHandle;
	FTimerHandle CountdownTimerHandle;
	ERLRunState RunState = ERLRunState::Waiting;
	int32 CurrentRoundIndex = INDEX_NONE;
	int32 CurrentPhaseIndex = INDEX_NONE;
	int32 CurrentWaveIndex = INDEX_NONE;
	float RoundStartTimeSeconds = 0.0f;
	float RoundElapsedSeconds = 0.0f;
	float IntermissionEndTimeSeconds = 0.0f;
	float CountdownEndTimeSeconds = 0.0f;
	float RunStartTimeSeconds = -1.0f;
	float RunEndTimeSeconds = 0.0f;
	int32 RoundsCleared = 0;
	int32 BestParryCombo = 0;
	FName CurrentPhaseName = NAME_None;
	FName CurrentWaveName = NAME_None;
	float NextWaveStartTimeSeconds = 0.0f;
	bool bWaveTransitionPending = false;
	bool bTutorialEnemyMovementLocked = false;
	bool bTutorialOnlyMode = false;
	TArray<ERLRunRewardType> PendingRewardChoices;
	TWeakObjectPtr<ARLEnemyCharacter> TutorialEnemy;
};
