// QuestValidator.cpp — Startup validation for quest setup errors
#include "Quest/QuestValidator.h"
#include "Quest/QuestDirector.h"
#include "Quest/QuestMission.h"
#include "Quest/QuestObjective.h"
#include "Quest/QuestRegistry.h"
#include "Quest/AutoSpawnZone.h"
#include "EngineUtils.h"
#include "Engine/World.h"

bool UQuestValidatorSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false; // Zero cost in shipping builds
#else
	return true;
#endif
}

void UQuestValidatorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UQuestValidatorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Delay validation slightly so all actors have called BeginPlay and registered
	FTimerHandle Handle;
	InWorld.GetTimerManager().SetTimer(Handle, [this]()
	{
		ValidateAll(GetWorld()); // Use GetWorld() — safe, doesn't capture dangling reference
	}, 0.5f, false);
}

void UQuestValidatorSubsystem::ValidateAll(UWorld* World)
{
	if (!World) return;
	ErrorCount = 0;

	// 1. Check for at least one QuestDirector
	int32 DirectorCount = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (Cast<AQuestDirector>(*It)) DirectorCount++;
	}

	if (DirectorCount == 0)
	{
		LogWarning(TEXT("No QuestDirector found in the level. Quest system will not run."));
	}
	else if (DirectorCount > 1)
	{
		LogWarning(TEXT("Multiple QuestDirectors found. Only the first will be used."));
	}

	// 2. Check each mission
	TArray<int32> UsedOrders;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AQuestMission* M = Cast<AQuestMission>(*It);
		if (!M) continue;

		if (M->Objectives.Num() == 0)
		{
			LogError(FString::Printf(TEXT("Mission '%s' has no objectives!"), *M->MissionTitle.ToString()));
		}

		if (UsedOrders.Contains(M->Order))
		{
			LogWarning(FString::Printf(TEXT("Mission '%s' has duplicate Order=%d. Check mission ordering."),
				*M->MissionTitle.ToString(), M->Order));
		}
		UsedOrders.Add(M->Order);

		// Check each objective's tag references
		for (UQuestObjective* Obj : M->Objectives)
		{
			if (UQuestObjective_AutoZone* AZ = Cast<UQuestObjective_AutoZone>(Obj))
			{
				if (!AZ->ZoneTag.IsValid())
				{
					LogError(FString::Printf(TEXT("Mission '%s', objective '%s': ZoneTag is empty!"),
						*M->MissionTitle.ToString(), *AZ->DisplayText.ToString()));
				}
				else
				{
					// Check the tag exists in the registry
					UQuestRegistry* Reg = World->GetSubsystem<UQuestRegistry>();
					if (Reg && !Reg->Exists(AZ->ZoneTag))
					{
						LogError(FString::Printf(TEXT("Mission '%s', objective '%s': Tag '%s' doesn't match any zone in the level. Check spelling."),
							*M->MissionTitle.ToString(), *AZ->DisplayText.ToString(), *AZ->ZoneTag.Tag.ToString()));
					}
				}
			}
		}
	}

	// 3. Check AutoSpawnZones
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AAutoSpawnZone* ASZ = Cast<AAutoSpawnZone>(*It);
		if (!ASZ) continue;
		
		if (!ASZ->DefaultEnemyClass)
		{
			LogError(FString::Printf(TEXT("AutoSpawnZone '%s': No DefaultEnemyClass set!"), *ASZ->Tag.Tag.ToString()));
		}
		if (ASZ->NumSpawnPoints <= 0)
		{
			LogError(FString::Printf(TEXT("AutoSpawnZone '%s': NumSpawnPoints is 0!"), *ASZ->Tag.Tag.ToString()));
		}
	}

	if (ErrorCount > 0)
	{
		UE_LOG(LogSquadAI, Error, TEXT("QuestValidator: %d errors found! Check Output Log for details."), ErrorCount);
	}
	else
	{
		UE_LOG(LogSquadAI, Log, TEXT("QuestValidator: all checks passed ✓"));
	}
}

void UQuestValidatorSubsystem::LogError(const FString& Message)
{
	ErrorCount++;
	UE_LOG(LogSquadAI, Error, TEXT("QUEST ERROR: %s"), *Message);

#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Red, FString::Printf(TEXT("QUEST ERROR: %s"), *Message));
	}
#endif
}

void UQuestValidatorSubsystem::LogWarning(const FString& Message)
{
	UE_LOG(LogSquadAI, Warning, TEXT("QUEST WARNING: %s"), *Message);

#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("QUEST WARNING: %s"), *Message));
	}
#endif
}
