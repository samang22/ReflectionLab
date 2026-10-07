#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/RLDifficultyScheduleDataAsset.h"
#include "Engine/TimerHandle.h"
#include "RLEnemyAttackComponent.generated.h"

class ARLProjectile;
class ARLExpandingRingAttack;
class USceneComponent;
struct FRLEnemyCombatRow;

// Owns attack timing, pattern selection and spawning. The character keeps the
// virtual Fire entrypoint so derived enemies can still customize each shot.
UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLEnemyAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLEnemyAttackComponent();
	void Configure(const FRLEnemyCombatRow& Config, TSubclassOf<ARLProjectile> NewProjectileClass,
		USceneComponent* NewMuzzlePoint, bool bAutoStart);
	void ActivateForPool();
	void DeactivateForPool();
	void ApplyDifficultyPhase(const FRLDifficultyPhase& DifficultyPhase);
	void ApplyWaveDefinition(const FRLWaveDefinition& WaveDefinition);
	void StartFiring();
	void StopFiring();
	void Fire();
	void SetTutorialCombatControlled(bool bControlled);
	bool FireTutorialProjectile(URLProjectileDefinitionDataAsset* ProjectileDefinition);
	ARLExpandingRingAttack* FireTutorialRingAttack();

	FSimpleDelegate OnFireShot;
	FSimpleDelegate OnShotSpawned;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void CacheBaseCombatValues();
	void BeginBurst();
	void FireNextShot();
	void RandomizePatternStart();
	bool UsesAttackVariation() const;
	void FireWeightedPattern();
	ARLExpandingRingAttack* SpawnRingAttack(bool bAutoStart = true);
	bool SpawnProjectile(URLProjectileDefinitionDataAsset* ProjectileDefinition,
		ERLShotPattern ShotPattern = ERLShotPattern::Single,
		float CrossLateralOffset = 90.0f, float CrossTargetOffset = 110.0f);

	UPROPERTY(Transient)
	TSubclassOf<ARLProjectile> ProjectileClass;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> MuzzlePoint;
	UPROPERTY(Transient)
	TObjectPtr<URLProjectileDefinitionDataAsset> DefaultProjectileDefinition;
	UPROPERTY(Transient)
	TArray<FRLProjectileSpawnRule> ProjectileRules;
	UPROPERTY(Transient)
	FRLRingAttackSpawnRule RingAttackRule;

	FTimerHandle AttackTimerHandle;
	FTimerHandle BurstTimerHandle;
	FRLEnemyAttackVariation AttackVariation;
	float AttackInterval = 3.0f;
	int32 ShotsPerBurst = 1;
	float TimeBetweenShots = 0.5f;
	float InitialFireDelay = 1.0f;
	float BaseAttackInterval = 3.0f;
	int32 BaseShotsPerBurst = 1;
	float BaseTimeBetweenShots = 0.5f;
	float CurrentBurstInterval = 3.0f;
	double BurstStartTime = 0.0;
	double NextRingAttackTime = 0.0;
	int32 DefaultProjectileWeight = 12;
	bool bUseWeightedPatterns = false;
	int32 RemainingShotsInBurst = 0;
	int32 ShotsFiredSinceActivation = 0;
	int32 PatternShotOffset = 0;
	bool bFiring = false;
	bool bExecutingAlignedShot = false;
	bool bAutoStartFiring = true;
	bool bTutorialCombatControlled = false;
};

