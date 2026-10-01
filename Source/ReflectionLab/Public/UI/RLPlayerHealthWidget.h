#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RLPlayerHealthWidget.generated.h"

class ARLPlayerCharacter;
class UCanvasPanel;
class UHorizontalBox;
class UTextBlock;
class UUniformGridPanel;

UCLASS()
class REFLECTIONLAB_API URLPlayerHealthWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindToPlayer(ARLPlayerCharacter* PlayerCharacter);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);

	void BuildWidgetTree();
	void RebuildHealthSegments();
	void RefreshHealthSegments();
	void SpawnDamageFragments(int32 SegmentIndex);
	void UpdateDamageFragments(float DeltaTime);

	struct FHealthFragment
	{
		TWeakObjectPtr<UTextBlock> Widget;
		FVector2D Position = FVector2D::ZeroVector;
		FVector2D Velocity = FVector2D::ZeroVector;
		float Rotation = 0.0f;
		float RotationSpeed = 0.0f;
		float TimeRemaining = 0.0f;
		float MaxLifetime = 0.0f;
	};

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> HealthContainer;

	UPROPERTY(Transient)
	TObjectPtr<UUniformGridPanel> HealthSegmentsContainer;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HealthLabel;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> HealthSegments;

	UPROPERTY(Transient)
	TObjectPtr<ARLPlayerCharacter> BoundPlayerCharacter;

	UPROPERTY(EditDefaultsOnly, Category = "Health UI")
	FLinearColor FilledColor = FLinearColor(0.1f, 0.9f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Health UI")
	FLinearColor EmptyColor = FLinearColor(0.12f, 0.14f, 0.18f, 0.75f);

	UPROPERTY(EditDefaultsOnly, Category = "Health UI")
	FLinearColor DamageColor = FLinearColor(1.0f, 0.04f, 0.02f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Health UI")
	FLinearColor RecoveryColor = FLinearColor(0.1f, 1.0f, 0.35f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Health UI", meta = (ClampMin = "0.0"))
	float FeedbackDuration = 0.3f;

	static constexpr int32 SegmentsPerRow = 10;

	UPROPERTY(EditDefaultsOnly, Category = "Health UI|Layout", meta = (ClampMin = "24.0", ClampMax = "96.0"))
	float SegmentDisplaySize = 48.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Health UI|Layout")
	float SegmentVerticalOffset = 14.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Health UI|Damage Fragments", meta = (ClampMin = "1", ClampMax = "24"))
	int32 DamageFragmentCount = 8;

	UPROPERTY(EditDefaultsOnly, Category = "Health UI|Damage Fragments", meta = (ClampMin = "0.1"))
	float DamageFragmentLifetime = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Health UI|Damage Fragments", meta = (ClampMin = "0.0"))
	float DamageFragmentGravity = 650.0f;

	float DisplayedCurrentHealth = 0.0f;
	float DisplayedMaxHealth = 0.0f;
	float FeedbackTimeRemaining = 0.0f;
	int32 FeedbackSegmentIndex = INDEX_NONE;
	int32 NextFragmentId = 0;
	bool bRecoveryFeedback = false;
	TArray<FHealthFragment> ActiveFragments;
};
