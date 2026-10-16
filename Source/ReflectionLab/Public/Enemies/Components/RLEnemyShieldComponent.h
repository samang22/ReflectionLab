#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLEnemyShieldComponent.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class ARLEnemyCharacter;

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLEnemyShieldComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLEnemyShieldComponent();
	void Configure(UStaticMeshComponent* NewVisual, float NewVisualRadius, const FLinearColor& Color);
	void SetEmitter(bool bEnabled);
	bool IsEmitterActive() const;
	bool TryAbsorbReflectedProjectile();
	void SetSupportSource(ARLEnemyCharacter* Source, const FLinearColor& Color = FLinearColor(0.65f,0.15f,1));
	bool IsSupportProtected() const;

private:
	TWeakObjectPtr<ARLEnemyCharacter> SupportSource;
	uint64 SupportSourceSerial = 0;
	FLinearColor PersonalColor = FLinearColor::White;
	FLinearColor SupportColor = FLinearColor(0.65f,0.15f,1);
	void UpdateVisual();

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Visual;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;
	float VisualRadius = 99.0f;
	bool bEmitterActive = false;
};

