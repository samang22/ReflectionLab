#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/RLProjectileRallySettings.h"
#include "RLProjectileRallyComponent.generated.h"

class ARLEnemyCharacter;
class URLProjectileDefinitionDataAsset;

// Owns rally settings and targets. ARLProjectile drives updates.
UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLProjectileRallyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLProjectileRallyComponent();
	void ApplyDefinitionSettings(const URLProjectileDefinitionDataAsset& Definition);
	void ResetRuntimeState(float InitialSpeed = 0.0f);
	bool IsRallyProjectile() const { return bIsRallyProjectile; }
	bool IsRelayTarget(const AActor* Actor) const { return RallyTarget.Get() == Actor; }
	AActor* GetTargetActor() const { return RallyTarget.Get(); }
	bool HasDamagedEnemy(const ARLEnemyCharacter* Enemy) const;
	void RecordEnemyHit(ARLEnemyCharacter* Enemy);
	void DisableRally(bool bClearTargets = false);
	void ResetTargetsForReflection();
	void BeginReflectedReturn(float Speed);
	void AdvanceAfterEnemyHit();
	void ConfigureAsRally(
		AActor* FinalTarget,
		int32 InMaxRallies,
		float InSpeedMultiplierPerRally);
	void UpdateRally(float DeltaTime);
	void AdvanceRally();

private:
	void SetRallyTarget(AActor* NewTarget);
	AActor* FindNextRallyTarget(AActor* RelaySource) const;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Rally", meta = (AllowPrivateAccess = "true"))
	FRLProjectileRallySettings Settings;

	float RallyCurrentSpeed = 0.0f;
	float RallySpeedMultiplierPerRally = 1.15f;
	int32 RallyCount = 0;
	int32 MaxRallies = 0;
	TWeakObjectPtr<AActor> RallyTarget;
	TArray<TWeakObjectPtr<ARLEnemyCharacter>> RallyDamagedEnemies;
	TWeakObjectPtr<AActor> RallyFinalTarget;
	bool bIsRallyProjectile = false;
	bool bRallyFinalShot = false;
};
