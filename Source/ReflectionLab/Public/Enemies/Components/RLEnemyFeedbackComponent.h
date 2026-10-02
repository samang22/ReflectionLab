#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLEnemyFeedbackComponent.generated.h"

class USoundBase;
class UStaticMesh;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLEnemyFeedbackSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback")
	TObjectPtr<USoundBase> HitSound;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback")
	float HitSoundVolume = 0.65f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback")
	float HitSoundPitchMin = 0.94f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback")
	float HitSoundPitchMax = 1.06f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback|Death")
	TObjectPtr<UStaticMesh> DeathEffectShardMesh;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback|Death")
	TObjectPtr<UMaterialInterface> DeathEffectShardMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback|Death")
	FVector DeathEffectShardScale = FVector(0.22f, 0.07f, 0.07f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback|Death")
	float DeathEffectRadius = 75.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Feedback|Death")
	FLinearColor DeathEffectColor = FLinearColor(1.0f, 0.015f, 0.005f, 1.0f);
};

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLEnemyFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLEnemyFeedbackComponent();
	void Configure(const FRLEnemyFeedbackSettings& NewSettings);
	void PlayHitSound() const;
	void SpawnDeathEffect() const;
	void PlayShootAnimation() const;
	void ResetCombatAnimation() const;

private:
	UPROPERTY(Transient)
	FRLEnemyFeedbackSettings Settings;
};

