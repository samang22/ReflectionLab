#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLDodgeRollComponent.generated.h"

class ACharacter;
class UAnimMontage;
class URLPlayerStatsDataAsset;

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLDodgeRollComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLDodgeRollComponent();
	void SetPlayerStats(URLPlayerStatsDataAsset* Stats) { PlayerStatsData = Stats; }
	bool TryStartRoll(const FVector& Direction);
	void StopRoll();
	bool IsRolling() const { return bRolling; }
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Roll|Animation")
	TObjectPtr<UAnimMontage> RollMontage;

private:
	friend class FRLPlayerRollTest;
	UPROPERTY(Transient)
	TObjectPtr<URLPlayerStatsDataAsset> PlayerStatsData;
	// Keep the running roll independent of edits to the shared tuning asset.
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveRollMontage;
	float ActiveRollCooldown = 0.0f;
	void HandleMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	TWeakObjectPtr<ACharacter> RollingCharacter;
	double NextRollTime = 0.0;
	bool bRolling = false;
	bool bPreviousUseControllerDesiredRotation = false;
};
