#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/RLBossChargeDataAsset.h"
#include "Data/RLBossLaserDataAsset.h"
#include "Data/RLBossOverloadDataAsset.h"
#include "Data/RLBossSummonDataAsset.h"
#include "Data/RLPaperBurnDataAsset.h"
#include "RLRobotBossDataAsset.generated.h"

// One editable asset, with independently typed settings for each component.
UCLASS(BlueprintType)
class REFLECTIONLAB_API URLRobotBossDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	URLRobotBossDataAsset();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category="Boss|Overload")
	TObjectPtr<URLBossOverloadDataAsset> Overload;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category="Boss|Charge")
	TObjectPtr<URLBossChargeDataAsset> Charge;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category="Boss|Laser")
	TObjectPtr<URLBossLaserDataAsset> Laser;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category="Boss|Summon")
	TObjectPtr<URLBossSummonDataAsset> Summon;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category="Boss|Death")
	TObjectPtr<URLPaperBurnDataAsset> PaperBurn;
};
