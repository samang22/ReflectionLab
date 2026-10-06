#pragma once

#include "CoreMinimal.h"
#include "RLProjectileVisualSettings.generated.h"

class UMaterialInterface;

USTRUCT(BlueprintType)
struct REFLECTIONLAB_API FRLProjectileVisualSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visuals", meta = (ClampMin = "0.0"))
	float FadeOutDuration = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visuals")
	TObjectPtr<UMaterialInterface> HostileMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visuals")
	TObjectPtr<UMaterialInterface> ReflectedMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visuals|Afterimage", meta = (ClampMin = "1", ClampMax = "8"))
	int32 ReflectedAfterimageCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visuals|Afterimage", meta = (ClampMin = "0.01", ClampMax = "0.2"))
	float ReflectedAfterimageSampleInterval = 0.035f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visuals|Afterimage", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float ReflectedAfterimageScaleFalloff = 0.12f;
};
