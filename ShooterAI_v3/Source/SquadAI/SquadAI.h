// SquadAI.h — Module header
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSquadAIModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
