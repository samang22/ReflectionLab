#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/TimerHandle.h"
#include "Player/RLPlayerCombatTypes.h"
#include "RLParryFeedbackComponent.generated.h"

class UDecalComponent;
class UNiagaraComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USoundBase;
class UAudioComponent;
class URLPlayerStatsDataAsset;

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLParryFeedbackComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	URLParryFeedbackComponent();
	void Configure(const URLPlayerStatsDataAsset* Stats);
	void UpdateView(const FRLParryViewState& State);
	void ShowParrySuccessIndicator();
	void ClearParrySuccessIndicator();
	void PlayParrySwingSound() const;
	void PlayParryImpactSound(const FVector& SoundLocation, int32 EnhancementLevel, bool bPerfectParry);
	void TriggerParryHitStop(bool bPerfectParry, bool bOverdrive);
	void RestoreTimeDilation();

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDecalComponent> ParryRangeIndicator;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parry|Overdrive|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> OverdriveAuraComponent;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> ParryRangeIndicatorMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	bool bShowParryRangeIndicator = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	FLinearColor ParryAvailableIndicatorColor = FLinearColor(0.05f, 1.0f, 0.15f, 1.0f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	FLinearColor PerfectParryAvailableIndicatorColor = FLinearColor(0.05f, 1.0f, 0.45f, 1.0f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	FLinearColor ParryCooldownIndicatorColor = FLinearColor(1.0f, 0.05f, 0.03f, 1.0f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	FLinearColor PerfectParryCooldownIndicatorColor = FLinearColor(1.0f, 0.25f, 0.03f, 1.0f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	FLinearColor ParrySuccessIndicatorColor = FLinearColor(0.02f, 0.45f, 1.0f, 1.0f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (AllowPrivateAccess = "true"))
	FLinearColor PerfectParrySuccessIndicatorColor = FLinearColor(0.02f, 0.75f, 1.0f, 1.0f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Indicator", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float ParrySuccessIndicatorDuration = 0.18f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parry|Feedback", meta = (AllowPrivateAccess = "true", DisplayName = "Parry Impact Sound"))
	TObjectPtr<USoundBase> ParryImpactSound;

private:
	void UpdateParryRangeIndicator();
	void UpdateParryIndicatorColor();
	void UpdateOverdriveAura();
	FRLParryViewState View;
	float PerfectHitStopDurationMultiplier = 1.75f;
	float OverdriveHitStopDurationMultiplier = 2.5f;
	float ParryIndicatorIdleOpacity = 0.18f;
	float ParryIndicatorActiveOpacity = 0.38f;
	float ParryIndicatorSuccessOpacity = 0.55f;
	float ParryIndicatorUnavailableOpacity = 0.22f;
	float ParryImpactSoundVolume = 0.65f;
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> PerfectParrySound;
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> PerfectParryAudioComponent;
	float PerfectParrySoundVolume = 0.8f;
	float PerfectImpactVolumeMultiplier = 0.4f;
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> PowerLevelUpSound;
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> PowerLevelUpAudioComponent;
	float PowerLevelUpSoundVolume = 0.65f;
	bool bHasViewState = false;
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> ParrySwingSound;
	float ParrySwingSoundVolume = 0.45f;
	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundBase>> ParryComboImpactSounds;
	TArray<float> ParryComboImpactSoundVolumes;
	float ParryHitStopDuration = 0.04f;
	float ParryHitStopTimeDilation = 0.1f;
	float OverdriveAuraBaseScale = 1.0f;
	float EnhancementAuraStage2ScaleMultiplier = 0.1f;
	float EnhancementAuraStage3ScaleMultiplier = 0.3f;
	float EnhancementAuraStage4ScaleMultiplier = 0.7f;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ParryRangeIndicatorMaterialInstance;
	FTimerHandle ParrySuccessIndicatorTimerHandle;
	FTimerHandle ParryHitStopTimerHandle;
	bool bShowingParrySuccessIndicator = false;
	bool bParryHitStopActive = false;
	float PreviousTimeDilation = 1.0f;
};

