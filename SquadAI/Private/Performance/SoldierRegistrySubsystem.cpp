// SoldierRegistrySubsystem.cpp — Sparse 2D hash for O(K) spatial queries
#include "Performance/SoldierRegistrySubsystem.h"
#include "Characters/SoldierCharacter.h"
#include "AI/SoldierAIController.h"
#include "SquadAITuning.h"

void USoldierRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	const USquadAITuning* T = USquadAITuning::Get();
	CellSize = T->RegistryCellSize;
	RebuildInterval = T->RegistryRebucketInterval;
}

void USoldierRegistrySubsystem::Deinitialize()
{
	AllSoldiers.Empty();
	Buckets.Empty();
	Super::Deinitialize();
}

void USoldierRegistrySubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceRebuild += DeltaTime;
	if (TimeSinceRebuild >= RebuildInterval)
	{
		TimeSinceRebuild = 0.f;
		RebuildBuckets();
	}
}

// =====================================================================
//  REGISTRATION
// =====================================================================
void USoldierRegistrySubsystem::RegisterSoldier(ASoldierCharacter* Soldier)
{
	if (Soldier)
	{
		AllSoldiers.AddUnique(Soldier);
	}
}

void USoldierRegistrySubsystem::UnregisterSoldier(ASoldierCharacter* Soldier)
{
	AllSoldiers.Remove(Soldier);
}

// =====================================================================
//  REBUILD BUCKETS — every 0.2s, re-hash all soldier positions
// =====================================================================
void USoldierRegistrySubsystem::RebuildBuckets()
{
	Buckets.Empty();

	for (int32 i = AllSoldiers.Num() - 1; i >= 0; --i)
	{
		ASoldierCharacter* S = AllSoldiers[i].Get();
		if (!S || !S->IsAlive())
		{
			AllSoldiers.RemoveAtSwap(i, EAllowShrinking::No);
			continue;
		}

		FIntPoint Cell = WorldToCell(S->GetActorLocation());
		Buckets.FindOrAdd(Cell).Soldiers.Add(S);
	}
}

// =====================================================================
//  QUERY — O(K) where K = soldiers in nearby cells
// =====================================================================
TArray<ASoldierCharacter*> USoldierRegistrySubsystem::QueryRadius(FVector Center, float Radius) const
{
	TArray<ASoldierCharacter*> Result;
	const float RadiusSq = Radius * Radius;

	TArray<FIntPoint> Cells = GetCellsInRadius(Center, Radius);
	for (const FIntPoint& Cell : Cells)
	{
		const FBucket* Bucket = Buckets.Find(Cell);
		if (!Bucket) continue;

		for (ASoldierCharacter* S : Bucket->Soldiers)
		{
			if (S && S->IsAlive() && FVector::DistSquared(S->GetActorLocation(), Center) <= RadiusSq)
			{
				Result.Add(S);
			}
		}
	}

	return Result;
}

// =====================================================================
//  SUPPRESSION — apply along a bullet path
// =====================================================================
void USoldierRegistrySubsystem::ApplySuppressionAlongPath(
	FVector Start, FVector End, float Radius, float Amount, AActor* Ignore)
{
	// Query cells along the path line
	TArray<ASoldierCharacter*> Nearby;
	
	// A bullet path is a line segment. We query a sequence of spheres along it
	// to ensure we cover the whole capsule volume without O(N) checking everyone.
	const float PathLen = FVector::Dist(Start, End);
	const FVector Dir = (End - Start) / PathLen;
	
	// Large steps for the spatial hash (CellSize/2), but small enough to not miss anyone
	for (float Dist = 0.f; Dist <= PathLen; Dist += CellSize * 0.5f)
	{
		FVector SamplePt = Start + Dir * Dist;
		TArray<ASoldierCharacter*> CellSoldiers = QueryRadius(SamplePt, CellSize * 0.7f);
		for (ASoldierCharacter* S : CellSoldiers) Nearby.AddUnique(S);
	}

	for (ASoldierCharacter* S : Nearby)
	{
		if (S == Ignore || !S->IsAlive()) continue;

		FVector SoldierLoc = S->GetActorLocation();
		
		// Closest point on line segment
		const FVector ToSoldier = SoldierLoc - Start;
		float t = FVector::DotProduct(ToSoldier, Dir);
		t = FMath::Clamp(t, 0.f, PathLen);
		
		FVector ClosestPoint = Start + Dir * t;
		if (FVector::DistSquared(SoldierLoc, ClosestPoint) <= (Radius * Radius))
		{
			if (ASoldierAIController* AI = Cast<ASoldierAIController>(S->GetController()))
			{
				AI->AddSuppression(Amount);
			}
		}
	}
}

// =====================================================================
//  COUNT HOSTILES — for leader area-clear check
// =====================================================================
int32 USoldierRegistrySubsystem::CountHostilesNear(
	FVector Center, float Radius, uint8 FriendlyTeamId) const
{
	int32 Count = 0;
	TArray<ASoldierCharacter*> Nearby = QueryRadius(Center, Radius);

	for (ASoldierCharacter* S : Nearby)
	{
		if (S->GetGenericTeamId().GetId() != FriendlyTeamId && S->IsAlive())
		{
			Count++;
		}
	}

	return Count;
}

// =====================================================================
//  SPATIAL HASH HELPERS
// =====================================================================
FIntPoint USoldierRegistrySubsystem::WorldToCell(FVector Pos) const
{
	return FIntPoint(
		FMath::FloorToInt(Pos.X / CellSize),
		FMath::FloorToInt(Pos.Y / CellSize)
	);
}

TArray<FIntPoint> USoldierRegistrySubsystem::GetCellsInRadius(FVector Center, float Radius) const
{
	TArray<FIntPoint> Result;
	const FIntPoint Min = WorldToCell(Center - FVector(Radius, Radius, 0));
	const FIntPoint Max = WorldToCell(Center + FVector(Radius, Radius, 0));

	for (int32 X = Min.X; X <= Max.X; ++X)
	{
		for (int32 Y = Min.Y; Y <= Max.Y; ++Y)
		{
			Result.Add(FIntPoint(X, Y));
		}
	}

	return Result;
}
