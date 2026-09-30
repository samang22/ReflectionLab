#include "Data/RLParryTuningDataAsset.h"

#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

URLParryTuningDataAsset::URLParryTuningDataAsset()
{
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> OverdriveAuraFinder(
		TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Lightning.NS_Free_Spells_Aura_Lightning"));
	if (OverdriveAuraFinder.Succeeded())
	{
		OverdriveAuraVFX = OverdriveAuraFinder.Object;
	}
}
