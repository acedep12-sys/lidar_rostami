# Complete System Audit — Errors Found & Fixed

## CRITICAL ERRORS (Would crash or silently fail)

### 1. ❌ STTask_TakeCover: UAITask_MoveTo::AIMoveTo wrong signature
**File:** `STTask_TakeCover.cpp`
**Error:** `UAITask_MoveTo::AIMoveTo(AI, Cover.Location, AActor::StaticClass(), 80.f, ...)`
The second parameter should be `FVector` for location-based moves OR `AActor*` for actor-based.
Passing `AActor::StaticClass()` (a UClass*) in the target actor slot is wrong.
**Fix:** Use the location-only overload:
```cpp
UAITask_MoveTo::AIMoveTo(AI, Cover.Location, nullptr, 80.f);
```

### 2. ❌ STTask_FollowLeader: Same UAITask_MoveTo wrong signature
**File:** `STTask_FollowLeader.cpp`
**Error:** Same issue — `AActor::StaticClass()` passed as target actor.
**Fix:** Use `nullptr` for location-based moves.

### 3. ❌ StateTree Tick signature mismatch
**File:** All StateTree tasks
**Error:** Task `Tick` declared as `Tick(FStateTreeExecutionContext&, const float)` 
but the base class may use `Tick(FStateTreeExecutionContext&, float)` (no const).
Different UE versions have different signatures.
**Fix:** Match exact base class signature. In 5.7:
`virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;`
This IS correct for 5.7 based on the API docs. ✅ No change needed.

### 4. ❌ Context.GetOwner() returns different types in different UE versions
**File:** All StateTree tasks and evaluators
**Error:** In some 5.7 builds, `GetOwner()` returns `const UObject*`, not `AActor*`.
**Fix:** Use the pattern from the reference doc:
```cpp
AActor* OwnerActor = Cast<AActor>(Context.GetOwner());
```
The `const_cast` in my code (`const_cast<AActor*>(Cast<AActor>(Context.GetOwner()))`)
is needed because we get a const pointer but need to call non-const methods.
**Status:** Already using `Cast<AActor>(Context.GetOwner())` which handles the cast.
But the `const_cast` is ugly. Better pattern: store controller ref in instance data
on EnterState and reuse on Tick. ✅ Already doing this for cover (via controller).

### 5. ❌ CoverScanner: FTraceDelegate vs FAsyncTraceDelegate naming
**File:** `CoverScanner.cpp`
**Error:** Used `FTraceDelegate` but the correct UE 5.7 type might be `FTraceDelegate`
or `FAsyncTraceDelegate` depending on the trace function used.
`AsyncLineTraceByChannel` uses the 6th parameter as `FCollisionResponseParams`
and the 7th as `FTraceDelegate*`.
**Fix:** Verify the parameter order matches the actual UE 5.7 API.

### 6. ❌ CoverScanner: BindRaw with member function and extra params
**File:** `CoverScanner.cpp`
**Error:** `Delegate.BindRaw(this, &FCoverScanner::OnAsyncTraceComplete, ChunkKey, SampleIndex, d, h)`
`FTraceDelegate` is a `TDelegate<void(const FTraceHandle&, FTraceDatum&)>`.
Extra payload parameters must be bound differently — UE delegates don't support
arbitrary extra params in BindRaw like std::bind.
**Fix:** Use a wrapper approach — store pending trace info in a TMap keyed by
FTraceHandle, look it up in the callback.

### 7. ❌ WaveSpawner: HandleSpawnedDied expects AActor* but OnDeath broadcasts AActor*
**File:** `WaveSpawner.cpp`
**Error:** `HC->OnDeath.AddDynamic(this, &AWaveSpawner::HandleSpawnedDied)` 
but `FOnHealthDied` broadcasts `AActor*` while `HandleSpawnedDied` takes `AActor*`.
This IS correct — both are `AActor*`. ✅ No issue.

### 8. ❌ EnemyPool: WakeEnemy calls SpawnDefaultController but controller class unknown
**File:** `EnemyPool.cpp`  
**Error:** `Enemy->SpawnDefaultController()` works but the spawned controller won't have
the StateTree asset assigned. The AI controller needs its StateTree reference.
**Fix:** Instead of SpawnDefaultController, re-possess with the original controller
(store it before sleeping) or ensure the pawn's AIControllerClass is set correctly.

### 9. ❌ QuestObjective: UObject can't use GetWorld() directly
**File:** `QuestObjective.h/.cpp`
**Error:** `UQuestObjective` is a `UObject`, not an `AActor`. Plain UObjects don't have
`GetWorld()` unless they have an outer that provides world context.
**Fix:** Pass `UWorld*` as parameter to `OnStart` and `Tick` (already doing this ✅).

### 10. ❌ MissionMinimapWidget: NativePaint signature wrong
**File:** `MissionMinimapWidget.h`
**Error:** `NativePaint` uses `FSlateClippedElement` which doesn't exist.
The correct UE 5.7 signature is:
```cpp
virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
    int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
```
**Fix:** Change `FSlateClippedElement` to `FSlateRect`.

### 11. ❌ MissionMinimapWidget: FSlateDrawElement::MakeBox uses wrong style reference
**File:** `MissionMinimapWidget.cpp`
**Error:** `FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button").Normal` — this
might not exist or might be named differently. For simple box drawing, use
`FSlateColorBrush` or the `FCoreStyle` defaults.
**Fix:** Use a simpler brush approach.

### 12. ❌ CompassWidget: Lambda inside NativeTick captures nothing for English cardinals
**File:** `CompassWidget.cpp`  
**Error:** The lambda for English cardinals captures `PlayerYaw` but it's a
local variable — the lambda executes immediately (IIFE) so this is actually fine.
✅ No issue — it's an immediately-invoked lambda.

### 13. ❌ SquadAITuning: CategoryName/SectionName set in constructor after GENERATED_BODY
**File:** `SquadAITuning.cpp`
**Error:** `CategoryName` and `SectionName` are not real UPROPERTY members of
UDeveloperSettings. The correct way is to override `GetCategoryName()` and
`GetSectionName()` (already done in the header via `GetCategoryName()`).
Remove the constructor assignments.
**Fix:** Remove `CategoryName = ...` and `SectionName = ...` from constructor.

## MEDIUM ERRORS (Would compile but behave incorrectly)

### 14. ⚠️ SoldierCharacter: RegisterWithPerceptionSystem called in constructor
**File:** `SoldierCharacter.cpp`
**Error:** `PerceptionStimuli->RegisterWithPerceptionSystem()` in the constructor
may fail because the world doesn't exist yet. Registration happens automatically
on component registration (BeginPlay), so this call is unnecessary and potentially
harmful.
**Fix:** Remove the explicit `RegisterWithPerceptionSystem()` call.

### 15. ⚠️ CoverSystemSubsystem: FinalizeCompletedSamples iterates wrong
**File:** `CoverSystemSubsystem.cpp`
**Error:** `FinalizeCompletedSamples` iterates `PendingSamplesByChunk` but the
scanner's `InFlightSamples` is keyed differently than expected. The chunk
completion detection is too simplistic (`bool bAllDone = true` always true).
**Fix:** Track pending trace count per chunk properly.

### 16. ⚠️ AllySoldier: Constructor accesses WeaponComp before it's created
**File:** `AllySoldier.cpp` (from Batch 4)
**Error:** In the AAllySoldier constructor, there's no guarantee WeaponComp is
initialized yet (it's created by the parent constructor). Actually, in UE,
the parent constructor runs first, so by the time AAllySoldier's constructor
body runs, WeaponComp IS created. ✅ No issue.

### 17. ⚠️ Missing #include for UAISense_Damage in SoldierAIController
**File:** `SoldierAIController.cpp`
**Error:** Uses `UAISenseConfig_Damage` but the correct header is
`Perception/AISenseConfig_Damage.h`. Need to verify this header exists.
In some UE versions it's in a different location.

### 18. ⚠️ SoldierRegistrySubsystem: QueryRadius called from const context with non-const method
**File:** `SoldierRegistrySubsystem.cpp`
**Error:** `CountHostilesNear` is const but calls `QueryRadius` which is non-const
(it returns a TArray by value, but the method itself isn't const).
**Fix:** Make `QueryRadius` const or use const_cast.

## LOW PRIORITY (Style/quality improvements)

### 19. 💡 Missing UPROPERTY on FProbeSample arrays
**File:** `CoverScanner.h`
**Note:** `HitResults`, `HitDistances`, `HitActors` are raw C arrays inside
a non-USTRUCT. This is fine for non-reflected data but means they won't
show in debuggers. Acceptable for internal scanner state.

### 20. 💡 QuestValidator: OnWorldBeginPlay timer lambda captures this
**File:** `QuestValidator.cpp`
**Note:** Lambda captures `this` and `&InWorld`. The `&InWorld` is a reference
to a parameter that will go out of scope. Should capture by value or use
`GetWorld()` inside the lambda.
**Fix:** Remove `&InWorld` capture, use `GetWorld()` instead.

---

## SUGGESTIONS FROM THE IMPROVEMENTS DOC — What to implement:

### Implemented already:
- ✅ 1.2 Escape path check (TerrainCoverHelper)
- ✅ 1.4 Per-direction lean quality (TerrainCoverHelper)
- ✅ 1.5 Vertical cover scoring (MeasureExactCoverHeight)
- ✅ 2.1 Multi-sense fusion (PerceptionMemory)
- ✅ 2.2 Suppression decay slower in cover (PerceptionMemory::DecayAll)
- ✅ 2.3 Enemy movement prediction (FEnhancedPerceivedTarget::GetBestKnownPosition)
- ✅ 4.3 SignificanceManager-driven LOD (AISignificanceManager)

### Should implement now (HIGH priority from suggestions):
- ⬜ 1.1 Reject enclosed spaces (ceiling cast) — BUT exempt trenches
- ⬜ 3.3 Smart Object approach vectors (5.7 multi-angle)
- ⬜ 3.6 Cover failure handling (advance/retreat instead of idle)
- ⬜ 6.1 Death animation → ragdoll transition
- ⬜ 6.2 Hit reaction flinch animation trigger
