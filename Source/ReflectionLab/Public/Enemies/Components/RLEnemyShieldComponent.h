#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLEnemyShieldComponent.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS(ClassGroup = (ReflectionLab), meta = (BlueprintSpawnableComponent))
class REFLECTIONLAB_API URLEnemyShieldComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URLEnemyShieldComponent();
	void Configure(UStaticMeshComponent* NewVisual, float NewProtectionRadius,
		float NewVisualRadius, const FLinearColor& Color);
	void SetEmitter(bool bEnabled);
	bool IsEmitterActive() const;
	bool TryAbsorbReflectedProjectile();

private:
	bool IsProtectedByShield() const;
	void UpdateVisual();

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Visual;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;
	float ProtectionRadius = 500.0f;
	float VisualRadius = 99.0f;
	bool bEmitterActive = false;
};

