// EnvQueryTest_CoverScore.cpp — Scores EQS cover points against a threat
#include "CoverSystem/EnvQueryTest_CoverScore.h"
#include "CoverSystem/CoverSystemSubsystem.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "Engine/World.h"

UEnvQueryTest_CoverScore::UEnvQueryTest_CoverScore()
{
	Cost = EEnvTestCost::Low;
	ValidItemType = UEnvQueryItemType_Point::StaticClass();
	SetWorkOnFloatValues(true);
	ThreatContext = UEnvQueryContext_Querier::StaticClass();
}

void UEnvQueryTest_CoverScore::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!QueryOwner) return;

	UWorld* World = GEngine->GetWorldFromContextObject(QueryOwner, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return;

	UCoverSystemSubsystem* CoverSys = World->GetSubsystem<UCoverSystemSubsystem>();
	if (!CoverSys) return;

	// Get threat location
	TArray<FVector> ThreatLocations;
	if (!QueryInstance.PrepareContext(ThreatContext, ThreatLocations) || ThreatLocations.Num() == 0)
		return;

	const FVector ThreatLoc = ThreatLocations[0];

	// Get all cover points to match items against
	TArray<FVector> QuerierLocations;
	QueryInstance.PrepareContext(UEnvQueryContext_Querier::StaticClass(), QuerierLocations);
	const FVector QuerierLoc = QuerierLocations.Num() > 0 ? QuerierLocations[0] : FVector::ZeroVector;

	TArray<FCoverPoint> AllCovers = CoverSys->GetAllCoverPointsInRadius(QuerierLoc, 3000.f);

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector ItemLoc = GetItemLocation(QueryInstance, It.GetIndex());

		// Find the matching cover point
		float BestScore = 0.f;
		for (const FCoverPoint& CP : AllCovers)
		{
			if (FVector::Dist(CP.Location, ItemLoc) < 50.f)
			{
				// Use the scanner's scoring algorithm
				FCoverScanner TempScanner;
				BestScore = TempScanner.ScoreAgainstThreat(CP, ThreatLoc);
				break;
			}
		}

		It.SetScore(TestPurpose, FilterType, BestScore, 0.f, 1.f);
	}
}

FText UEnvQueryTest_CoverScore::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("Cover Score vs Threat"));
}

FText UEnvQueryTest_CoverScore::GetDescriptionDetails() const
{
	return FText::FromString(TEXT("Scores cover by facing, arc, height, and lean availability"));
}
