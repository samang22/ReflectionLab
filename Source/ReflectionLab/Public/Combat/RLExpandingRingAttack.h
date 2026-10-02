#pragma once

#include "CoreMinimal.h"
#include "Data/RLRingAttackDataAsset.h"
#include "GameFramework/Actor.h"
#include "RLExpandingRingAttack.generated.h"

class UInstancedStaticMeshComponent;
class ARLPlayerCharacter;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRLRingPlayerDodgedSignature, ARLPlayerCharacter*, Player);

// A world-space hazard, deliberately not an RLProjectile: it cannot be parried.
UCLASS()
class REFLECTIONLAB_API ARLExpandingRingAttack : public AActor
{
	GENERATED_BODY()

public:
	ARLExpandingRingAttack();
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Ring Attack")
	bool StartAttack();
	UFUNCTION(BlueprintCallable, Category = "Ring Attack")
	void BeginFadeOut();

	UPROPERTY(BlueprintAssignable, Category = "Ring Attack")
	FRLRingPlayerDodgedSignature OnPlayerDodged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring Attack", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<URLRingAttackDataAsset> AttackData;
	// Placed test actors wait for PlayingRound before starting, then fire once.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring Attack", meta = (ExposeOnSpawn = "true"))
	bool bAutoStart = true;

protected:
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ring Attack")
	TObjectPtr<UInstancedStaticMeshComponent> RingVisual;

private:
	static bool IntersectsSweptRing(const FVector2D& Start, const FVector2D& End,
		float PreviousRadius, float NextRadius, float Padding);
	bool IsCombatActive() const;
	void UpdateVisual();
	void CheckPlayerHit(float PreviousRadius);
	FRLRingAttackSettings ActiveSettings;
	float CurrentRadius = 0.0f;
	float ActiveElapsedTime = 0.0f;
	float FadeOutElapsedTime = 0.0f;
	bool bFadingOut = false;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RingMaterial;
	bool bActive = false;
	bool bPlayerHit = false;
	bool bPlayerDodged = false;
	FVector PreviousPlayerLocation = FVector::ZeroVector;
	TWeakObjectPtr<ARLPlayerCharacter> TrackedPlayer;
	static constexpr int32 SegmentCount = 96;
};
