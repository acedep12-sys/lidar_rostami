// EnvQueryGenerator_CoverPoints.cpp — Produces cover point locations as EQS items
#include "CoverSystem/EnvQueryGenerator_CoverPoints.h"
#include "CoverSystem/CoverSystemSubsystem.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "Engine/World.h"

UEnvQueryGenerator_CoverPoints::UEnvQueryGenerator_CoverPoints()
{
	ItemType = UEnvQueryItemType_Point::StaticClass();
}

void UEnvQueryGenerator_CoverPoints::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
	UWorld* World = GEngine->GetWorldFromContextObject(
		QueryInstance.Owner.Get(), EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return;

	UCoverSystemSubsystem* CoverSys = World->GetSubsystem<UCoverSystemSubsystem>();
	if (!CoverSys) return;

	TArray<FVector> QuerierLocations;
	QueryInstance.PrepareContext(UEnvQueryContext_Querier::StaticClass(), QuerierLocations);
	if (QuerierLocations.Num() == 0) return;

	const FVector QuerierLoc = QuerierLocations[0];
	const float Radius = SearchRadius;

	// Get all cover points in range (synchronous — for EQS visual debugging)
	TArray<FCoverPoint> Covers = CoverSys->GetAllCoverPointsInRadius(QuerierLoc, Radius);

	for (const FCoverPoint& CP : Covers)
	{
		if (CP.IsReserved()) continue; // Skip claimed covers

		QueryInstance.AddItemData<UEnvQueryItemType_Point>(CP.Location);
	}
}

FText UEnvQueryGenerator_CoverPoints::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("Cover Points (Squad AI)"));
}

FText UEnvQueryGenerator_CoverPoints::GetDescriptionDetails() const
{
	return FText::FromString(FString::Printf(TEXT("Radius: %.0f"), SearchRadius));
}
