#pragma once

#include "CoreMinimal.h"
#include "Data/RLEnemyAttackVariation.h"
#include "Data/RLProjectileDefinitionDataAsset.h"
#include "Data/RLRingAttackDataAsset.h"
#include "Engine/DataAsset.h"
#include "RLDifficultyScheduleDataAsset.generated.h"

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLProjectileSpawnRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Rule")
	TObjectPtr<URLProjectileDefinitionDataAsset> ProjectileDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Rule", meta = (ClampMin = "0"))
	int32 Weight = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Rule", meta = (ClampMin = "0", DisplayName = "Legacy Shot Interval"))
	int32 ShotInterval = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Rule")
	ERLShotPattern ShotPattern = ERLShotPattern::Single;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Rule|Cross", meta = (ClampMin = "0.0", EditCondition = "ShotPattern == ERLShotPattern::Cross", EditConditionHides))
	float CrossLateralOffset = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Rule|Cross", meta = (ClampMin = "0.0", EditCondition = "ShotPattern == ERLShotPattern::Cross", EditConditionHides))
	float CrossTargetOffset = 110.0f;
};

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLDifficultyPhase
{
	GENERATED_BODY()

	// Opt in after migrating old interval rules to weights.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Patterns")
	bool bUseWeightedPatterns = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Patterns", meta = (ClampMin = "0"))
	int32 DefaultProjectileWeight = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	FName PhaseName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float StartTimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Spawning", meta = (ClampMin = "0"))
	int32 MaxAliveEnemies = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Spawning", meta = (ClampMin = "1"))
	int32 SpawnBatchSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Spawning", meta = (ClampMin = "0.1"))
	float SpawnInterval = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Spawning", meta = (ClampMin = "0.0"))
	float MinimumSpawnDistance = 750.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Spawning", meta = (ClampMin = "0.0"))
	float MaximumSpawnDistance = 1150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Combat", meta = (ClampMin = "0.1"))
	float AttackIntervalMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Combat", meta = (ClampMin = "0"))
	int32 ShotsPerBurstOverride = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Combat", meta = (ClampMin = "0.1"))
	float TimeBetweenShotsMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Projectiles")
	TObjectPtr<URLProjectileDefinitionDataAsset> DefaultProjectileDefinition;

	// Interval ordering is used only by legacy/tutorial selection.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Projectiles")
	TArray<FRLProjectileSpawnRule> ProjectileRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Ring Attack")
	FRLRingAttackSpawnRule RingAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Combat")
	FRLEnemyAttackVariation AttackVariation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty|Enemy Mechanics", meta = (ClampMin = "0"))
	int32 ShieldEnemyInterval = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	bool bBreatherPhase = false;
};

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLWaveDefinition
{
	GENERATED_BODY()

	// Opt in after migrating old interval rules to weights.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Patterns")
	bool bUseWeightedPatterns = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Patterns", meta = (ClampMin = "0"))
	int32 DefaultProjectileWeight = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FName WaveName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Spawning", meta = (ClampMin = "1"))
	int32 EnemyCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "0.0"))
	float NextWaveDelaySeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Spawning", meta = (ClampMin = "0.0"))
	float MinimumSpawnDistance = 750.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Spawning", meta = (ClampMin = "0.0"))
	float MaximumSpawnDistance = 1150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Combat", meta = (ClampMin = "0.1"))
	float AttackIntervalMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Combat", meta = (ClampMin = "0"))
	int32 ShotsPerBurstOverride = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Combat", meta = (ClampMin = "0.1"))
	float TimeBetweenShotsMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Projectiles")
	TObjectPtr<URLProjectileDefinitionDataAsset> DefaultProjectileDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Projectiles")
	TArray<FRLProjectileSpawnRule> ProjectileRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Ring Attack")
	FRLRingAttackSpawnRule RingAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Combat")
	FRLEnemyAttackVariation AttackVariation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Enemy Mechanics", meta = (ClampMin = "0"))
	int32 ShieldEnemyInterval = 0;
};

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLDifficultyScheduleDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wave")
	TArray<FRLWaveDefinition> Waves;

	// Retained only so existing assets can be migrated. Runtime gameplay ignores it.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deprecated", meta = (DeprecatedProperty, DeprecationMessage = "Rounds now progress by Waves."))
	float DurationSeconds = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deprecated", meta = (DeprecatedProperty, DeprecationMessage = "Use Waves instead."))
	TArray<FRLDifficultyPhase> Phases;

	const FRLDifficultyPhase* FindPhaseAtTime(float ElapsedSeconds, int32& OutPhaseIndex) const;
};
