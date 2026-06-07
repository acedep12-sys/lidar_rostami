// QuestRegistry.cpp — O(1) tag-based actor lookup
#include "Quest/QuestRegistry.h"

void UQuestRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UQuestRegistry::Deinitialize()
{
	Registry.Empty();
	Super::Deinitialize();
}

void UQuestRegistry::Register(FQuestTag Tag, AActor* Actor)
{
	if (!Tag.IsValid() || !Actor) return;
	Registry.Add(Tag.Tag, Actor);
	UE_LOG(LogSquadAI, Verbose, TEXT("QuestRegistry: registered '%s' → %s"), *Tag.Tag.ToString(), *Actor->GetName());
}

void UQuestRegistry::Unregister(FQuestTag Tag)
{
	if (Tag.IsValid()) Registry.Remove(Tag.Tag);
}

AActor* UQuestRegistry::Find(FQuestTag Tag) const
{
	if (!Tag.IsValid()) return nullptr;
	const TWeakObjectPtr<AActor>* Found = Registry.Find(Tag.Tag);
	return Found ? Found->Get() : nullptr;
}

bool UQuestRegistry::Exists(FQuestTag Tag) const
{
	return Tag.IsValid() && Registry.Contains(Tag.Tag);
}

TArray<FQuestTag> UQuestRegistry::GetAllTags() const
{
	TArray<FQuestTag> Result;
	for (const auto& Pair : Registry)
	{
		FQuestTag T;
		T.Tag = Pair.Key;
		Result.Add(T);
	}
	return Result;
}
