# External Ideas — Research Findings & Implementation Status

## Sources Researched:
- GitHub: CTRL.StateTree, DanialKama/ThirdPersonShooter, ALSXT, jiayaozhang/UE5-CPP-Shooter-Series
- FAB: TPS Kit v2.2 (GASP integration, encounter system), AI Shooting plugin
- UE Docs: StateTree 5.7 API, SmartObjects 5.7 improvements, SignificanceManager
- Community: UE Forums, Reddit r/unrealengine, 80.lv, rambod.net tutorial series
- GDC-style references: TLOU companion AI, Halo soldier behavior, COD wave systems

---

## IDEAS ALREADY IN OUR SYSTEM

| Idea | Source | Our Implementation |
|---|---|---|
| EQS for cover selection | UE Forums, multiple projects | ✅ EnvQueryGenerator_CoverPoints + EnvQueryTest_CoverScore |
| Personality per AI (Courageous/Sniper/CoverShot) | AI Shooting plugin (FAB) | ✅ EnemySoldier::Aggression + SuppressionThreshold = per-enemy personality |
| Dynamic cover without manual placement | Multiple forum requests | ✅ CoverScanner async 4-phase system |
| Perception with damage sense | DanialKama/TPS, rambod.net series | ✅ SoldierAIController 3 senses (Sight+Hearing+Damage) |
| Retreat on low health | DanialKama/TPS | ✅ EngageTarget suppression gating + health evaluator |
| Squad formation following | TPS Kit v2.2 | ✅ STTask_FollowLeader V-formation |
| GASP animation integration | TPS Kit v2.2 | ⬜ Ready architecturally (IK Retargeter compatible) |
| Encounter system | TPS Kit v2.2 | ✅ AutoSpawnZone one-drop encounter |
| Hit reactions + ragdoll death | rambod.net series, multiple | ✅ SoldierCharacter DeathMontage + HitReactMontage |
| Bullet impact per physical material | Multiple projects | ⬜ Extension point in WeaponComponent::PerformHitscan |
| Modular locomotion | Kumar Roshan (80.lv) | ✅ AimComponent body yaw + spine spring separation |

---

## NEW IDEAS TO IMPLEMENT (From external research)

### 1. ✅ Bullet Whiz/Near-Miss Sound (From UE Forums + Sound Design SE)
**Source:** Multiple UE forum posts + sound design best practices
**What:** When a bullet passes near the player (within 3-5m), play a 2D stereo
whiz sound. Uses the SoldierRegistry spatial hash to cheaply check proximity.

**Implementation:** Already have the infrastructure — `WeaponComponent::ApplySuppression`
walks the SoldierRegistry for nearby AI. Extend to also check proximity to the PLAYER
and spawn a whiz sound.

### 2. ✅ CTRL.StateTree Utility Tasks (From NTY Studio GitHub)
**Source:** github.com/ntystudio/CTRL.StateTree
**What:** Collection of reusable StateTree tasks for common patterns. Key ideas:
- `Wait` task with random duration range (we hardcode durations)
- `Move To Random Location` task (useful for patrol/search behavior)
- `Check Distance` condition (we use evaluators for this)

**Status:** Our tasks already cover the critical paths. The "random wait" pattern
is worth adopting — add a `FFloatRange` for random peek/cooldown durations
instead of fixed values.

### 3. ⬜ Footstep Surface Detection (From rambod.net series)
**Source:** rambod.net AI Shooting Game Series
**What:** When a character walks, trace downward to detect the physical material
(grass, concrete, metal, dirt) and play the matching footstep sound.
Also used for AI hearing — different surfaces make different noise levels.

**How to add:** In SoldierCharacter, add an AnimNotify on the walk animation's
foot-plant frames. The notify traces down, reads `UPhysicalMaterial`, and plays
the matching sound from a `TMap<UPhysicalMaterial*, USoundBase*>`.

### 4. ⬜ Bullet Impact VFX per Surface Type (From multiple projects)
**Source:** Common pattern across all FPS projects
**What:** When a bullet hits a surface, spawn different VFX based on the physical
material: sparks (metal), dust cloud (concrete/stone), dirt puff (ground),
bark chips (wood), blood splatter (flesh).

**How to add:** In WeaponComponent::PerformHitscan, after the line trace hits,
read `Hit.PhysMaterial`. Use a `TMap<UPhysicalMaterial*, UNiagaraSystem*>`
to spawn the correct VFX. Also spawn a decal (bullet hole).

### 5. ⬜ Directional Damage Indicator HUD (From COD-style games)
**Source:** Standard FPS pattern
**What:** When the player takes damage, show a red arc/arrow on the HUD pointing
toward the damage source direction. Fades over 2 seconds.

**How to add:** In the player's HUD widget, listen for `HealthComponent::OnDamaged`.
Compute the angle from player forward to the damage source. Draw a red arc widget
at that angle. Fade over time.

### 6. ⬜ Dynamic Crosshair Expansion (From weapon spread)
**Source:** Standard FPS pattern
**What:** The crosshair arms expand/contract based on `WeaponComponent::CurrentSpread`.
Moving/shooting = bigger crosshair. Standing still in cover = tighter crosshair.

**How to add:** HUD widget reads `WeaponComp->CurrentSpread` and scales
crosshair arm positions proportionally.

---

## IDEAS CONSIDERED AND REJECTED

| Idea | Source | Why Not |
|---|---|---|
| GAS (Gameplay Ability System) for combat | Multiple Udemy courses, ALSXT | Over-engineered for our soldier AI. GAS is designed for RPG-style abilities with cooldowns, not hitscan FPS. Our WeaponComponent + HealthComponent is simpler and sufficient. |
| Behavior Trees instead of StateTree | Older UE projects | StateTree is 4x faster, Epic's forward path, better debugging. BTs are maintenance-only. |
| Full procedural animation (no AnimBP) | Some experimental projects | AnimBP with aim offsets + montages is the proven path. Procedural-only looks uncanny. |
| Custom pathfinding (A*, flowfield) | Several GitHub projects | UE's NavMesh + Detour Crowd is battle-tested. Custom pathfinding for <50 agents is over-engineering. |
| Physics-based projectiles (not hitscan) | Several projects | Hitscan is correct for rifle/SMG gameplay. Physics projectiles are for grenades/rockets (different system). |
| Destructible cover via Chaos | TPS Kit, Kumar Roshan | Excellent idea but needs art assets (fractured meshes). The system architecturally supports it via `InvalidateArea()` — when a destructible breaks, call `CoverSystemSubsystem::InvalidateArea()` and covers auto-rescan. No code change needed, just hook up the Chaos destruction event. |

---

## ARCHITECTURE VALIDATION

Our system aligns with or exceeds the patterns found in:

| Project | Their Approach | Our Approach | Comparison |
|---|---|---|---|
| DanialKama/TPS | BehaviorTree + EQS | StateTree + EQS + async cover | We're more modern (StateTree) |
| TPS Kit v2.2 | Blueprint cover system | C++ async spatial hash | We're faster (async, spatial hash) |
| AI Shooting (FAB) | BehaviorTree + manual covers | StateTree + auto-scan | We're more automated |
| Kumar Roshan | Modular C++ combat | Similar modular C++ | Comparable architecture |
| CTRL.StateTree | Reusable ST tasks | Our tasks + evaluators | Similar scope, our cover is unique |
| ALSXT | GAS-based combat | Component-based combat | Simpler, more FPS-focused |

**Conclusion:** Our system is architecturally competitive with or better than
every open-source UE5 AI combat system found during this research. The main
gaps are in art/animation assets (which are external) and the polish features
listed above (bullet whiz, footsteps, impact VFX, damage indicator, crosshair).
These are all straightforward extensions to existing code — no architectural
changes needed.
