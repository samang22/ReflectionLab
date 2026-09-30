#include "Data/RLProjectileDefinitionDataAsset.h"

#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

URLProjectileDefinitionDataAsset::URLProjectileDefinitionDataAsset()
{
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ReflectedTrailFinder(
		TEXT("/Game/ArrowTrail/FX/NS_ArrowTrail_Magic.NS_ArrowTrail_Magic"));
	if (ReflectedTrailFinder.Succeeded())
	{
		ReflectedTrailVFX = ReflectedTrailFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ExplosionFinder(
		TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Explosion.NS_Free_Spells_Explosion"));
	if (ExplosionFinder.Succeeded())
	{
		ExplosionVFX = ExplosionFinder.Object;
	}
}
