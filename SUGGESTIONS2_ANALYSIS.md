# SUGGESTIONS_2.md Analysis — Performance-Aware AI Logic

## Core Principle: "Fixed precision, variable cost"
**This is the right philosophy.** Never make AI dumber on slow hardware.
Reduce update frequency, not decision quality.

---

## AUDIT: What We Already Have vs. What's Missing

### ✅ ALREADY IMPLEMENTED (Architecturally correct in our code)

| Suggestion | Where in Our Code | Status |
|---|---|---|
| **1.1** Distance LOD buckets | `AISignificanceManager.h` — 6 buckets (Critical→Dormant) | ✅ Done |
| **3.4** Decoupled motor and decision | AimComponent ticks at frame rate, StateTree at lower rate | ✅ Done |
| **4.4** Cooperative claim of cover | CoverSystemSubsystem reservation map | ✅ Done |
| **9.2** Aim spring at frame rate | AimComponent::TickComponent runs per-frame | ✅ Done |
| **9.3** Suppression is wall-clock | SuppressionDecayPerSec × DeltaTime | ✅ Done |
| **9.4** Fire rate is wall-clock | WeaponComponent uses GetTimeSeconds() | ✅ Done |
| **11.2** Uses GetTimeSeconds() | All timers use world time, not tick counts | ✅ Done |
| **11.3** Animation decoupled from decision | MoveTo runs at frame rate, StateTree lower | ✅ Done |
| **13.1-13.4** Anti-patterns avoided | Never reduce sight/accuracy/cover on slow HW | ✅ Correct |

### ⚠️ PARTIALLY IMPLEMENTED (Need small fixes)

| Suggestion | What We Have | What's Missing |
|---|---|---|
| **1.3** Reactive tick override | Perception delegates exist | Need `bForceFullTick` flag on event |
| **7.1** LOD for aim spring | AimComponent ticks always | Need bucket check to skip for Ambient |
| **8.1** Spatial hash for perception | SoldierRegistrySubsystem exists | Not wired to perception yet |
| **11.1** No tick counts | Most use wall-clock | Need audit of StateTree task timers |

### ❌ NOT IMPLEMENTED (Need new code)

| Suggestion | Priority | Effort | Impact |
|---|---|---|---|
| **1.2** Angular LOD (view cone) | MEDIUM | XS | Better perceived quality |
| **1.4** Combat-event promotion | MEDIUM | XS | Combat responsiveness |
| **2.1** Player trajectory prediction | HIGH | XS | Massive aim quality improvement |
| **2.2** Cover prediction (future threat pos) | HIGH | XS | Better cover selection |
| **4.1** Group cover queries | HIGH | M | Biggest wave perf win |
| **4.3** Precomputed threat map | HIGH | M | Cover scoring O(1) |
| **5.1** Shared threat memory | HIGH | S | Squad awareness |
| **6.1** Proximity alarm (30Hz cheap check) | HIGH | XS | Instant responsiveness |
| **6.2** Visual contact (cheap LOS) | HIGH | XS | Detection quality |
| **7.2** LOD pathfinding | HIGH | S | Pathfinding savings |
| **7.3** LOD cover queries | HIGH | XS | Cover query savings |
| **7.4** LOD EQS | HIGH | S | EQS savings |
| **9.1** Reaction time wall-clock | HIGH | XS | Correctness |
| **9.5** Lead prediction w/ bullet TOF | HIGH | XS | Aim quality |

---

## WHAT TO IMPLEMENT NOW

### Tier 1 — XS effort, HIGH impact (implement immediately):

1. **Reactive tick override (1.3)** — wake ambient AI on damage/perception events
2. **LOD for aim spring (7.1)** — skip spring for Ambient, snap instead
3. **LOD for cover queries (7.3)** — Ambient AI skip cover entirely
4. **Player trajectory prediction (2.1)** — lead target by velocity × bullet TOF
5. **Proximity alarm (6.1)** — 30Hz distance check promotes bucket
6. **Visual contact cheap LOS (6.2)** — per-frame LOS for Critical
7. **Reaction time wall-clock (9.1)** — audit StateTree timers
8. **Angular LOD (1.2)** — view cone dot product for bucket promotion

### Tier 2 — S effort, HIGH impact:
9. **Shared threat memory (5.1)** — squad-wide known threats
10. **Combat-event promotion (1.4)** — nearby gunfire promotes bucket
11. **LOD pathfinding (7.2)** — straight-line for Ambient
12. **LOD EQS (7.4)** — reduced radius for Engaged, skip for Ambient
13. **Cover prediction (2.2)** — future quality scoring

### Tier 3 — M effort, HIGH impact:
14. **Group cover queries (4.1)** — batched for wave spawns
15. **Precomputed threat map (4.3)** — O(1) cover scoring

---

## IMPLEMENTATION PLAN

Rather than writing 15 new files, most of these are small
additions to EXISTING files. The changes are:

### AISignificanceManager.cpp — Add angular LOD + combat promotion
### SoldierAIController.h/.cpp — Add reactive tick, proximity alarm, shared memory
### AimComponent.cpp — Add LOD bucket check
### CoverSystemSubsystem.cpp — Add LOD-aware query budgeting
### WeaponComponent.cpp — Add player lead prediction
### New: SharedThreatMemory.h/.cpp — Squad-wide threat awareness
