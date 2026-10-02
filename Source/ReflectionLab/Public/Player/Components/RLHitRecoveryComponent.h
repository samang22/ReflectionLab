#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "RLHitRecoveryComponent.generated.h"

class URLPlayerStatsDataAsset;
class UMaterialInterface;
class USoundBase;

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLHitRecoverySettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Recovery")
	float Duration = 0.6f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Recovery")
	float MovementSpeedMultiplier = 0.45f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Recovery")
	TObjectPtr<UMaterialInterface> FlashMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Recovery")
	TObjectPtr<USoundBase> HitSound;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Recovery")
	float SoundVolume = 0.8f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Recovery")
	float SoundPitchMin = 0.97f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Recovery")
	float SoundPitchMax = 1.03f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Recovery")
	float FlashInterval = 0.08f;
};

DECLARE_MULTICAST_DELEGATE(FRLHitRecoverySignature);

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLHitRecoveryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLHitRecoveryComponent();
	void SetPlayerStats(const URLPlayerStatsDataAsset* Stats);
	void Configure(const FRLHitRecoverySettings& NewSettings);
	void PlayHitSound() const;
	bool BeginRecovery();
	void EndRecovery();
	bool IsRecovering() const { return bRecovering; }

	// The owner coordinates other actions; this component does not depend on parry or health.
	FRLHitRecoverySignature OnRecoveryStarted;
	FRLHitRecoverySignature OnRecoveryEnded;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void StartFlash();
	void ToggleFlash();
	void StopFlash();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Recovery", meta = (AllowPrivateAccess = "true"))
	FRLHitRecoverySettings Settings;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PreviousOverlayMaterial;
	FTimerHandle RecoveryTimerHandle;
	FTimerHandle FlashTimerHandle;
	float PreviousMaxWalkSpeed = 0.0f;
	bool bRecovering = false;
	bool bRestoreWalkSpeed = false;
	bool bFlashActive = false;
	bool bFlashVisible = false;
};
