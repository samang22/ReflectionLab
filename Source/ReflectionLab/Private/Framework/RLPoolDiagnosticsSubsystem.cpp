#include "Framework/RLPoolDiagnosticsSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogRLPoolDiagnostics, Log, All);

bool URLPoolDiagnosticsSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && Super::ShouldCreateSubsystem(Outer);
#endif
}

void URLPoolDiagnosticsSubsystem::Created(AActor* Actor, FName Pool)
{
#if !UE_BUILD_SHIPPING
	if (IsValid(Actor) && Actor->GetWorld())
	{
		if (auto* Diagnostics = Actor->GetWorld()->GetSubsystem<URLPoolDiagnosticsSubsystem>())
		{
			auto& Entry = Diagnostics->Pools.FindOrAdd(Pool);
			const bool bKnown = Entry.All.Contains(Actor);
			Entry.All.Add(Actor);
			Actor->OnDestroyed.AddUniqueDynamic(Diagnostics,&URLPoolDiagnosticsSubsystem::HandleActorDestroyed);
			if (Diagnostics->bMeasuring && !bKnown) { ++Entry.Counters.Created; }
		}
	}
#endif
}

void URLPoolDiagnosticsSubsystem::Acquired(AActor* Actor, FName Pool, bool bReused)
{
#if !UE_BUILD_SHIPPING
	if (IsValid(Actor) && Actor->GetWorld())
	{
		if (auto* Diagnostics = Actor->GetWorld()->GetSubsystem<URLPoolDiagnosticsSubsystem>())
		{
			auto& Entry = Diagnostics->Pools.FindOrAdd(Pool);
			Entry.All.Add(Actor);
			Actor->OnDestroyed.AddUniqueDynamic(Diagnostics,&URLPoolDiagnosticsSubsystem::HandleActorDestroyed);
			const bool bAlreadyActive = Entry.Active.Contains(Actor);
			Entry.Active.Add(Actor);
			if (Diagnostics->bMeasuring && !bAlreadyActive)
			{
				++Entry.Counters.Acquired;
				if (bReused) { ++Entry.Counters.Reused; }
				Entry.Counters.PeakActive = FMath::Max(Entry.Counters.PeakActive, Entry.Active.Num());
			}
		}
	}
#endif
}

void URLPoolDiagnosticsSubsystem::Released(AActor* Actor, FName Pool)
{
#if !UE_BUILD_SHIPPING
	if (IsValid(Actor) && Actor->GetWorld())
	{
		if (auto* Diagnostics = Actor->GetWorld()->GetSubsystem<URLPoolDiagnosticsSubsystem>())
		{
			auto& Entry = Diagnostics->Pools.FindOrAdd(Pool);
			Entry.All.Add(Actor);
			if (Entry.Active.Remove(Actor) && Diagnostics->bMeasuring) { ++Entry.Counters.Released; }
		}
	}
#endif
}

FRLPoolSnapshot URLPoolDiagnosticsSubsystem::GetSnapshot(FName Pool) const
{
	FRLPoolSnapshot Result;
	if (const auto* Entry = Pools.Find(Pool))
	{
		Result = Entry->Counters;
		for (const auto& Actor : Entry->All)
		{
			if (!Actor.IsValid()) { continue; }
			if (Entry->Active.Contains(Actor)) { ++Result.Active; }
			else { ++Result.Inactive; }
		}
	}
	return Result;
}

void URLPoolDiagnosticsSubsystem::StartMeasurement()
{
	for (auto& Pair : Pools)
	{
		for (auto It = Pair.Value.All.CreateIterator(); It; ++It) { if (!It->IsValid()) { It.RemoveCurrent(); } }
		for (auto It = Pair.Value.Active.CreateIterator(); It; ++It) { if (!It->IsValid()) { It.RemoveCurrent(); } }
		Pair.Value.Counters = FRLPoolSnapshot();
		Pair.Value.Counters.PeakActive = GetSnapshot(Pair.Key).Active;
	}
	bMeasuring = true;
	StartedAt = FPlatformTime::Seconds(); StoppedAt = 0;
	ReportPath = FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Profiling/PoolDiagnostics"),
		FString::Printf(TEXT("Pools_%s_%s.csv"),*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")),*FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	UE_LOG(LogRLPoolDiagnostics,Display,TEXT("Measurement started world=%s; wall-clock seconds; counters include prewarm; diagnostics overhead included"),*GetWorld()->GetName());
	WriteSnapshot();
}

void URLPoolDiagnosticsSubsystem::HandleActorDestroyed(AActor* Actor)
{
	for (auto& Pair : Pools) { Pair.Value.Active.Remove(Actor); Pair.Value.All.Remove(Actor); }
}

void URLPoolDiagnosticsSubsystem::StopMeasurement()
{
	if (!bMeasuring) { return; }
	StoppedAt = FPlatformTime::Seconds(); bMeasuring = false;
	WriteSnapshot();
}

void URLPoolDiagnosticsSubsystem::WriteSnapshot()
{
	const double Elapsed = StartedAt > 0 ? (bMeasuring ? FPlatformTime::Seconds() : StoppedAt)-StartedAt : 0;
	FString Rows;
	TArray<FName> Names; Pools.GetKeys(Names);
	Names.Sort(FNameLexicalLess());
	for (const FName Pool : Names)
	{
		const auto S = GetSnapshot(Pool);
		const FString Row = FString::Printf(TEXT("%.3f,%s,%s,%d,%d,%d,%lld,%lld,%lld,%lld\n"),Elapsed,
			bMeasuring ? TEXT("running") : TEXT("stopped"),*Pool.ToString(),S.Active,S.Inactive,S.PeakActive,S.Created,S.Reused,S.Acquired,S.Released);
		Rows += Row;
		UE_LOG(LogRLPoolDiagnostics,Display,TEXT("%s"),*Row.TrimEnd());
	}
	if (ReportPath.IsEmpty()) { return; }
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(ReportPath),true);
	if (!IFileManager::Get().FileExists(*ReportPath))
	{
		Rows = TEXT("elapsed_wall_seconds,state,pool,active,inactive,peak_active,created,reused,acquired,released\n") + Rows;
	}
	if (!FFileHelper::SaveStringToFile(Rows,*ReportPath,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(),FILEWRITE_Append))
	{ UE_LOG(LogRLPoolDiagnostics,Warning,TEXT("Cannot write %s"),*ReportPath); }
	else { UE_LOG(LogRLPoolDiagnostics,Display,TEXT("CSV: %s"),*FPaths::ConvertRelativePathToFull(ReportPath)); }
}

#if !UE_BUILD_SHIPPING
namespace
{
	void RunPoolCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (auto* D = World ? World->GetSubsystem<URLPoolDiagnosticsSubsystem>() : nullptr)
		{
			if (Args.Num() == 1 && Args[0] == TEXT("start")) { D->StartMeasurement(); }
			else if (Args.Num() == 1 && Args[0] == TEXT("stop")) { D->StopMeasurement(); }
			else { D->WriteSnapshot(); }
		}
		else { UE_LOG(LogRLPoolDiagnostics,Warning,TEXT("Run rl.Pool start|snapshot|stop in a game/PIE world")); }
	}
	FAutoConsoleCommandWithWorldAndArgs PoolCommand(TEXT("rl.Pool"),
		TEXT("Development-only pool diagnostics: rl.Pool start|snapshot|stop. CSV is written under Saved/Profiling/PoolDiagnostics."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RunPoolCommand));
}
#endif
