#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLBossSummonComponent.generated.h"

class ARLRobotMinionCharacter;
class URLBossSummonDataAsset;
class URLHealthComponent;
class URLBossSummonVisualComponent;

UCLASS(ClassGroup=(ReflectionLab), meta=(BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLBossSummonComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URLBossSummonComponent();
	void Initialize(URLHealthComponent* InHealth);
	void Reset();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|Summon") TObjectPtr<URLBossSummonDataAsset> Settings;
protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	struct FPendingMinion
	{
		FTransform Transform;
		float Remaining = 0.6f;
		int32 MarkerIndex = INDEX_NONE;
	};
	bool FindSpawnTransform(FTransform& OutTransform) const;
	void Summon();
	void UpdatePendingMinions(float DeltaTime);
	UPROPERTY(Transient) TObjectPtr<URLHealthComponent> Health;
	UPROPERTY(Transient) TObjectPtr<URLBossSummonVisualComponent> Visual;
	TArray<TWeakObjectPtr<ARLRobotMinionCharacter>> Minions;
	TArray<FPendingMinion> PendingMinions;
	float Remaining = 5.0f;
	bool bEnraged = false;
};
