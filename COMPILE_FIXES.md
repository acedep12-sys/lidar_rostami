# Compile Error Categories and Fixes

## Error 1: CoverTypes.generated.h not found
CoverTypes.h has GENERATED_BODY() but it's not a UCLASS/USTRUCT that needs UHT.
FIX: Remove GENERATED_BODY() from the non-reflected struct, or remove the .generated.h include.

## Error 2: FGenericTeamId not found in SharedThreatMemory
Missing #include for GenericTeamAgentInterface.h
FIX: Add include.

## Error 3: EAISignificance not found in SoldierAIController  
Missing #include for AISignificanceManager.h
FIX: Add forward declaration or include.

## Error 4: FSignificanceManagerModule undefined
The API for accessing SignificanceManager differs in installed vs source builds.
FIX: Use USignificanceManager::Get() instead of FSignificanceManagerModule::Get().

## Error 5: bShouldCallExitState undeclared
This member doesn't exist in FStateTreeTaskCommonBase in UE 5.7.
FIX: Remove it — ExitState is always called.

## Error 6: ToPaintGeometry deprecated API
UE 5.7 changed the signature.
FIX: Use the new overload with FVector2f.

## Error 7: TObjectPtr<UTextBlock> can't convert to UWidget*
Need to use .Get() or dereference.
FIX: Use .Get() on TObjectPtr in function calls.

## Error 8: FDamageEvent undefined
Missing #include.
FIX: Add #include "Engine/DamageEvents.h"

## Error 9: TActorIterator undefined
Missing #include "EngineUtils.h"

## Error 10: FAIDataProviderFloatValue in EQS
SearchRadius uses FAIDataProviderFloatValue which needs special handling.
FIX: Change to plain float.
