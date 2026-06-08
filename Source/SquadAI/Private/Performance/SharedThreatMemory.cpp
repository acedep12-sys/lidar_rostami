// SharedThreatMemory.cpp — Squad-wide threat awareness
#include "Performance/SharedThreatMemory.h"

void USharedThreatMemorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Tick at 4Hz — decay doesn't need per-frame updates
	// Note: UTickableWorldSubsystem doesn't have TickInterval,
	// so we throttle manually in Tick()
}

void USharedThreatMemorySubsystem::Deinitialize()
{
	KnownThreats.Empty();
	Super::Deinitialize();
}

void USharedThreatMemorySubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Throttle to 4Hz — decay doesn't need per-frame precision
	ThrottleTimer += DeltaTime;
	if (ThrottleTimer < 0.25f) return;

	DecayAndCleanup(ThrottleTimer, GetWorld()->GetTimeSeconds());
	ThrottleTimer = 0.f;
}

void USharedThreatMemorySubsystem::ReportThreat(
	AActor* Threat, FVector Location, FVector Velocity, float Confidence, uint8 ReporterTeamId)
{
	if (!Threat) return;

	TWeakObjectPtr<AActor> Key = Threat;
	FSharedThreatInfo* Existing = KnownThreats.Find(Key);

	if (Existing)
	{
		// Update if new report has higher confidence
		if (Confidence > Existing->Confidence)
		{
			Existing->LastKnownLocation = Location;
			Existing->PredictedVelocity = Velocity;
			Existing->Confidence = Confidence;
			Existing->LastUpdateTime = GetWorld()->GetTimeSeconds();
			Existing->ReporterTeamId = ReporterTeamId;
		}
	}
	else
	{
		FSharedThreatInfo Info;
		Info.Threat = Threat;
		Info.LastKnownLocation = Location;
		Info.PredictedVelocity = Velocity;
		Info.Confidence = Confidence;
		Info.LastUpdateTime = GetWorld()->GetTimeSeconds();
		Info.ReporterTeamId = ReporterTeamId;
		KnownThreats.Add(Key, Info);
	}
}

TArray<FSharedThreatInfo> USharedThreatMemorySubsystem::GetKnownThreatsForTeam(uint8 QuerierTeamId) const
{
	TArray<FSharedThreatInfo> Result;

	for (const auto& Pair : KnownThreats)
	{
		const FSharedThreatInfo& Info = Pair.Value;

		if (Info.ReporterTeamId == QuerierTeamId && Info.Confidence > 0.05f)
		{
			Result.Add(Info);
		}
	}

	return Result;
}

bool USharedThreatMemorySubsystem::GetBestKnownThreat(uint8 QuerierTeamId, FSharedThreatInfo& OutThreat) const
{
	float BestConf = 0.f;
	bool bFound = false;

	for (const auto& Pair : KnownThreats)
	{
		const FSharedThreatInfo& Info = Pair.Value;
		if (Info.ReporterTeamId == QuerierTeamId && Info.Confidence > BestConf)
		{
			BestConf = Info.Confidence;
			OutThreat = Info;
			bFound = true;
		}
	}

	return bFound;
}

void USharedThreatMemorySubsystem::DecayAndCleanup(float DeltaTime, float CurrentTime)
{
	TArray<TWeakObjectPtr<AActor>> ToRemove;

	for (auto& Pair : KnownThreats)
	{
		FSharedThreatInfo& Info = Pair.Value;

		// Remove invalid actors
		if (!Info.Threat.IsValid())
		{
			ToRemove.Add(Pair.Key);
			continue;
		}

		// Decay confidence
		Info.Confidence -= ConfidenceDecayRate * DeltaTime;

		// Remove if forgotten
		if (Info.Confidence <= 0.f || (CurrentTime - Info.LastUpdateTime) > ForgetAfterSeconds)
		{
			ToRemove.Add(Pair.Key);
		}
	}

	for (const auto& Key : ToRemove)
	{
		KnownThreats.Remove(Key);
	}
}
