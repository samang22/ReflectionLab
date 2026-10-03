#include "Data/RLPlayerStatsDataAsset.h"

#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

URLPlayerStatsDataAsset::URLPlayerStatsDataAsset()
{
	static ConstructorHelpers::FObjectFinder<USoundBase> Sound(
		TEXT("/Game/ReflectionLab/Audio/SFX/Combat/Parry/SFX_ParrySwing.SFX_ParrySwing"));
	RollSound = Sound.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(
		TEXT("/Game/ReflectionLab/Art/Materials/Projectiles/MI_Projectile_Reflected.MI_Projectile_Reflected"));
	RollAfterimageMaterial = Material.Object;
}
