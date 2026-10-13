#include "Data/RLRobotBossDataAsset.h"

URLRobotBossDataAsset::URLRobotBossDataAsset()
{
	Overload = CreateDefaultSubobject<URLBossOverloadDataAsset>(TEXT("Overload"));
	Charge = CreateDefaultSubobject<URLBossChargeDataAsset>(TEXT("Charge"));
	Laser = CreateDefaultSubobject<URLBossLaserDataAsset>(TEXT("Laser"));
	Summon = CreateDefaultSubobject<URLBossSummonDataAsset>(TEXT("Summon"));
	PaperBurn = CreateDefaultSubobject<URLPaperBurnDataAsset>(TEXT("PaperBurn"));
}
