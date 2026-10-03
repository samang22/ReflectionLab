#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RLRollAfterimagePoolSubsystem.generated.h"

class ARLRollAfterimage;
class USkeletalMeshComponent;
class UMaterialInterface;

UCLASS()
class REFLECTIONLAB_API URLRollAfterimagePoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Desired total capacity, including active images: repeated rolls don't
	// grow the pool simply because earlier images are still fading.
	void PrewarmPool(int32 DesiredCount, USkeletalMeshComponent* Source, UMaterialInterface* Material);
	ARLRollAfterimage* AcquireAfterimage(USkeletalMeshComponent* Source, UMaterialInterface* Material,
		const FLinearColor& Color, float Duration, float Opacity);
	void ReleaseAfterimage(ARLRollAfterimage* Afterimage);
	virtual void Deinitialize() override;

private:
	ARLRollAfterimage* SpawnAfterimage();
	void RemoveInvalidEntries();

	UPROPERTY(Transient)
	TArray<TObjectPtr<ARLRollAfterimage>> AllAfterimages;
	UPROPERTY(Transient)
	TArray<TObjectPtr<ARLRollAfterimage>> InactiveAfterimages;
	bool bShuttingDown = false;
};
