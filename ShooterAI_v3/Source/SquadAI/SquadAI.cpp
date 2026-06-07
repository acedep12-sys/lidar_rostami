// SquadAI.cpp — Module implementation
#include "SquadAI.h"

#define LOCTEXT_NAMESPACE "FSquadAIModule"

void FSquadAIModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("SquadAI module loaded"));
}

void FSquadAIModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_PRIMARY_GAME_MODULE(FSquadAIModule, SquadAI, "SquadAI");
