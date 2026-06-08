// QuestLog.cpp — Mission history with byte serialization
#include "Quest/QuestLog.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"

void UQuestLogSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UQuestLogSubsystem::RecordMission(FText Title, int32 Order, EMissionState State, float Duration)
{
	FMissionLogEntry Entry;
	Entry.Title = Title;
	Entry.Order = Order;
	Entry.FinalState = State;
	Entry.DurationSeconds = Duration;
	Entry.CompletionTime = FDateTime::Now();
	Entries.Add(Entry);

	UE_LOG(LogSquadAI, Log, TEXT("QuestLog: recorded '%s' — %s (%.1fs)"),
		*Title.ToString(),
		*UEnum::GetValueAsString(State),
		Duration);
}

TArray<uint8> UQuestLogSubsystem::ToBytes() const
{
	TArray<uint8> Data;
	FMemoryWriter Writer(Data);

	int32 Count = Entries.Num();
	Writer << Count;

	for (const FMissionLogEntry& E : Entries)
	{
		FString TitleStr = E.Title.ToString();
		Writer << TitleStr;
		
		int32 OrderCopy = E.Order;
		Writer << OrderCopy;

		uint8 StateVal = static_cast<uint8>(E.FinalState);
		Writer << StateVal;

		float DurationCopy = E.DurationSeconds;
		Writer << DurationCopy;

		int64 Ticks = E.CompletionTime.GetTicks();
		Writer << Ticks;
	}

	return Data;
}

void UQuestLogSubsystem::FromBytes(const TArray<uint8>& Data)
{
	if (Data.Num() == 0) return;

	Entries.Empty();
	FMemoryReader Reader(Data);

	int32 Count = 0;
	Reader << Count;

	for (int32 i = 0; i < Count; ++i)
	{
		FMissionLogEntry E;

		FString TitleStr;
		Reader << TitleStr;
		E.Title = FText::FromString(TitleStr);

		Reader << E.Order;

		uint8 StateVal = 0;
		Reader << StateVal;
		E.FinalState = static_cast<EMissionState>(StateVal);

		Reader << E.DurationSeconds;

		int64 Ticks = 0;
		Reader << Ticks;
		E.CompletionTime = FDateTime(Ticks);

		Entries.Add(E);
	}
}
