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
	// Approximate the path as a sphere query at the midpoint with half-length radius
	const FVector Mid = (Start + End) * 0.5f;
	const float HalfLen = FVector::Dist(Start, End) * 0.5f;
	const float SearchDist = FMath::Max(Radius, HalfLen);

	TArray<ASoldierCharacter*> Nearby = QueryRadius(Mid, SearchDist);

	const FVector PathDir = (End - Start).GetSafeNormal();
	const float PathLen = FVector::Dist(Start, End);

	for (ASoldierCharacter* S : Nearby)
	{
		if (S == Ignore) continue;

		// Point-to-line distance
		const FVector ToSoldier = S->GetActorLocation() - Start;
		const float Proj = FVector::DotProduct(ToSoldier, PathDir);
		if (Proj < 0.f || Proj > PathLen) continue; // Behind or past the bullet

		const FVector Closest = Start + PathDir * Proj;
		const float DistToPath = FVector::Dist(S->GetActorLocation(), Closest);

		if (DistToPath <= Radius)
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
