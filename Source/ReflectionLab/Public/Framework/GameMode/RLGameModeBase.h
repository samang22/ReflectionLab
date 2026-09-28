#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RLGameModeBase.generated.h"

class ARLEnemySpawner;
class URLRunDefinitionDataAsset;
struct FRLDifficultyPhase;
struct FRLRoundDefinition;

UENUM(BlueprintType)
enum class ERLRunState : uint8
{
	Waiting,
	Countdown,
	PlayingRound,
	Intermission,
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

UCLASS()
class REFLECTIONLAB_API ARLGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARLGameModeBase();

	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartRun();

	UFUNCTION(BlueprintCallable, Category = "Run")
	void StopRun();

	UFUNCTION(BlueprintCallable, Category = "Run")
	void RestartRun();

	UFUNCTION(BlueprintCallable, Category = "Run")
	void ReturnToMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Run")
	void NotifyPlayerDied();

	UFUNCTION(BlueprintPure, Category = "Run")
	ERLRunState GetRunState() const { return RunState; }

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetCurrentRoundIndex() const { return CurrentRoundIndex; }

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetRoundCount() const;

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

	UFUNCTION(BlueprintPure, Category = "Run|Results")
	float GetTotalRunElapsedSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Run|Results")
	int32 GetRoundsCleared() const { return RoundsCleared; }

	UFUNCTION(BlueprintPure, Category = "Run|Results")
	int32 GetBestParryCombo() const { return BestParryCombo; }

	UFUNCTION(BlueprintPure, Category = "Run|Navigation")
	bool CanReturnToMainMenu() const { return !MainMenuLevelName.IsNone(); }

	UPROPERTY(BlueprintAssignable, Category = "Run")
	FRLRunStateChangedSignature OnRunStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Run")
	FRLDifficultyPhaseChangedSignature OnDifficultyPhaseChanged;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Navigation")
	FName MainMenuLevelName = NAME_None;

private:
	UFUNCTION()
	void HandleParryChainChanged(int32 ParryChainCount);

	void BeginRoundCountdown(int32 RoundIndex);
	void FinishRoundCountdown();
	void StartRound(int32 RoundIndex);
	void UpdateRound();
	void FinishRound();
	void FinishIntermission();
	void ApplyDifficultyPhase(const FRLDifficultyPhase& DifficultyPhase, int32 PhaseIndex);
	void CacheEnemySpawners();
	void StopEnemySpawners();
	void CleanupRoundActors(const FRLRoundDefinition& RoundDefinition);
	void BindPlayerStats();
	void ResetRunRecord();
	void SetRunState(ERLRunState NewState);
	const FRLRoundDefinition* GetCurrentRoundDefinition() const;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ARLEnemySpawner>> EnemySpawners;

	FTimerHandle RoundUpdateTimerHandle;
	FTimerHandle IntermissionTimerHandle;
	FTimerHandle CountdownTimerHandle;
	ERLRunState RunState = ERLRunState::Waiting;
	int32 CurrentRoundIndex = INDEX_NONE;
	int32 CurrentPhaseIndex = INDEX_NONE;
	float RoundStartTimeSeconds = 0.0f;
	float RoundElapsedSeconds = 0.0f;
	float IntermissionEndTimeSeconds = 0.0f;
	float CountdownEndTimeSeconds = 0.0f;
	float RunStartTimeSeconds = -1.0f;
	float RunEndTimeSeconds = 0.0f;
	int32 RoundsCleared = 0;
	int32 BestParryCombo = 0;
	FName CurrentPhaseName = NAME_None;
};
