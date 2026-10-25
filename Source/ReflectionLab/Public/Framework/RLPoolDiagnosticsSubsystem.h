#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RLPoolDiagnosticsSubsystem.generated.h"

struct FRLPoolSnapshot
{
	int32 Active = 0;
	int32 Inactive = 0;
	int32 PeakActive = 0;
	int64 Created = 0;
	int64 Reused = 0;
	int64 Acquired = 0;
	int64 Released = 0;
};

// Weak inventory never keeps pooled actors alive. Counters cover the explicit
// measurement window, including prewarming performed inside that window.
UCLASS()
class REFLECTIONLAB_API URLPoolDiagnosticsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	static void Created(AActor* Actor, FName Pool);
	static void Acquired(AActor* Actor, FName Pool, bool bReused);
	static void Released(AActor* Actor, FName Pool);
	void StartMeasurement();
	void StopMeasurement();
	void WriteSnapshot();
	FRLPoolSnapshot GetSnapshot(FName Pool) const;
	bool IsMeasuring() const { return bMeasuring; }
private:
	UFUNCTION() void HandleActorDestroyed(AActor* Actor);
	struct FInventory
	{
		TSet<TWeakObjectPtr<AActor>> All;
		TSet<TWeakObjectPtr<AActor>> Active;
		FRLPoolSnapshot Counters;
	};
	TMap<FName,FInventory> Pools;
	bool bMeasuring = false;
	double StartedAt = 0;
	double StoppedAt = 0;
	FString ReportPath;
};
