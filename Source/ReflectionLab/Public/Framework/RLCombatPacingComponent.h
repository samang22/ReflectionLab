#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/RLCombatPacingSettings.h"
#include "RLCombatPacingComponent.generated.h"
class URLEnemyAttackComponent;

// One world scan per update, rather than one projectile scan per enemy.
UCLASS(ClassGroup=(ReflectionLab))
class REFLECTIONLAB_API URLCombatPacingComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URLCombatPacingComponent();
	void Configure(const FRLCombatPacingSettings& NewSettings, bool bOrdinaryCombatRound);
	void UpdatePacing();
	void ResetPacing();
protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UPROPERTY(Transient) FRLCombatPacingSettings Settings;
	TArray<TWeakObjectPtr<URLEnemyAttackComponent>> AdjustedEnemies;
};
