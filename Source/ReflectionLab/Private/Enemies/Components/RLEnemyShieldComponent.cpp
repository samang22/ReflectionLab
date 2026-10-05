#include "Enemies/Components/RLEnemyShieldComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"

URLEnemyShieldComponent::URLEnemyShieldComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URLEnemyShieldComponent::Configure(UStaticMeshComponent* NewVisual,
	float NewVisualRadius, const FLinearColor& Color)
{
	Visual = NewVisual;
	VisualRadius = FMath::IsFinite(NewVisualRadius) ? FMath::Max(1.0f, NewVisualRadius) : 99.0f;
	MaterialInstance = Visual ? Visual->CreateDynamicMaterialInstance(0) : nullptr;
	if (MaterialInstance)
	{
		MaterialInstance->SetVectorParameterValue(TEXT("ProjectileColor"), Color);
		MaterialInstance->SetScalarParameterValue(TEXT("EmissiveIntensity"), 8.0f);
		MaterialInstance->SetScalarParameterValue(TEXT("FadeOpacity"), 0.12f);
	}
	UpdateVisual();
}

bool URLEnemyShieldComponent::IsEmitterActive() const
{
	const ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	return bEmitterActive && Enemy && Enemy->IsPoolActive();
}

void URLEnemyShieldComponent::SetEmitter(bool bEnabled)
{
	const ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(GetOwner());
	bEmitterActive = bEnabled && Enemy && Enemy->IsPoolActive();
	UpdateVisual();
}

bool URLEnemyShieldComponent::TryAbsorbReflectedProjectile()
{
	if (IsEmitterActive())
	{
		SetEmitter(false);
		UE_LOG(LogTemp, Display, TEXT("Shield broken on %s."), *GetOwner()->GetName());
		return true;
	}
	return false;
}

void URLEnemyShieldComponent::UpdateVisual()
{
	if (!Visual) { return; }
	const float SphereMeshRadius = 50.0f;
	Visual->SetRelativeScale3D(FVector(VisualRadius / SphereMeshRadius));
	Visual->SetVisibility(IsEmitterActive(), true);
}

