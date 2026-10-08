#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RLPaperBurnDataAsset.generated.h"

class UMaterialInterface;

UCLASS(BlueprintType)
class REFLECTIONLAB_API URLPaperBurnDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.1")) float Duration = 1.3f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.001", ClampMax="0.2")) float EdgeWidth = 0.05f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FLinearColor EdgeColor = FLinearColor(1.0f, 0.12f, 0.02f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UMaterialInterface> Material;
};
