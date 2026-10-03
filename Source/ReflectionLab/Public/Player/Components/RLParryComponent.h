#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "Player/RLPlayerCombatTypes.h"
#include "RLParryComponent.generated.h"

class ACharacter;
class ARLProjectile;
class UAnimMontage;
class URLPlayerStatsDataAsset;
class URLParryProgressionComponent;
class URLRunRewardComponent;
class URLParryFeedbackComponent;
struct FRLProjectileReflectionParams;

DECLARE_MULTICAST_DELEGATE(FRLParryStateChangedSignature);
DECLARE_DELEGATE_RetVal(bool, FRLCanPerformParrySignature);

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLParryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLParryComponent();
	void Initialize(URLParryProgressionComponent* Progression, URLRunRewardComponent* Rewards,
		URLParryFeedbackComponent* Feedback);
	void Configure(const URLPlayerStatsDataAsset* PlayerStatsData);
	bool TryStartParry();
	void BeginParryWindow();
	void EndParryWindow();
	void CancelParry(bool bDowngradeEnhancement);
	bool IsAttemptInProgress() const { return bParryAttemptInProgress; }
	bool IsActive() const { return bParryActive; }
	bool IsOnCooldown() const { return bParryOnCooldown; }
	FRLParryViewState GetViewState() const;
	FRLParryStateChangedSignature OnStateChanged;
	FRLCanPerformParrySignature CanPerformParry;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Animation", meta = (AllowPrivateAccess = "true", DisplayName = "Parry Montage"))
	TObjectPtr<UAnimMontage> ParryMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Animation", meta = (AllowPrivateAccess = "true", DisplayName = "Mirrored Parry Montage"))
	TObjectPtr<UAnimMontage> MirroredParryMontage;

private:
	bool CanParry() const;
	void UpdateParry();
	bool IsProjectileWithinParryArc(const ARLProjectile* Projectile) const;
	bool TryParryProjectile(ARLProjectile* Projectile, int32 ResultingCombo,
		bool& bOutPerfectParry, bool& bOutCloseRangeParry);
	void SpawnAdditionalReflectedProjectiles(ARLProjectile* SourceProjectile,
		const TArray<FVector>& SplitDirections, const FRLProjectileReflectionParams& ReflectionParams);
	void EndParry(bool bSucceeded);
	void ResetParryCooldown();
	void ResetParryChain();
	UPROPERTY(Transient)
	TObjectPtr<URLParryProgressionComponent> ProgressionComponent;
	UPROPERTY(Transient)
	TObjectPtr<URLRunRewardComponent> RewardComponent;
	UPROPERTY(Transient)
	TObjectPtr<URLParryFeedbackComponent> FeedbackComponent;
	FRLParryStats Stats;
	FTimerHandle ParryAttemptTimerHandle;
	FTimerHandle ParryCooldownTimerHandle;
	bool bParryAttemptInProgress = false;
	bool bParryActive = false;
	bool bParryOnCooldown = false;
	bool bPlayMirroredParryNext = false;
};

