# ShooterAI v3 — Merged System Build Order

## This is a 77-file system. Built in 7 batches.

### Batch 1: Foundation (Module + Types + Tuning + Auto-Config)
- SquadAI.Build.cs
- SquadAI.h / .cpp
- SquadTypes.h (shared enums, structs)
- SquadAITuning.h / .cpp (UDeveloperSettings, 40+ values)
- SquadAIAutoConfig.h / .cpp (zero-merge installer)
- SquadAIAutoSetup.h / .cpp (one-click asset creator, editor only)
- SquadAIEditorMenu.cpp (Tools menu, editor only)
**Total: 11 files**

### Batch 2: Cover System (Async Scanner + Spatial Hash + Smart Objects)
- CoverSystem/CoverTypes.h (FCoverPoint POD struct)
- CoverSystem/CoverScanner.h / .cpp (async 4-phase scanner)
- CoverSystem/CoverSystemSubsystem.h / .cpp (spatial hash chunks, query API)
- CoverSystem/EnvQueryTest_CoverScore.h / .cpp (EQS scoring test)
- CoverSystem/EnvQueryGenerator_CoverPoints.h / .cpp (EQS generator)
**Total: 9 files**

### Batch 3: Components (Aim + Weapon + Health)
- Components/AimComponent.h / .cpp (critically damped spring)
- Components/WeaponComponent.h / .cpp (hitscan, spread, suppression)
- Components/HealthComponent.h / .cpp (invincible flag, damage chokepoint)
**Total: 6 files**

### Batch 4: Characters + AI Controllers
- Characters/SoldierCharacter.h / .cpp (base character)
- Characters/AllySoldier.h / .cpp (PowerLevel, immortal)
- Characters/EnemySoldier.h / .cpp (aggression, suppression threshold)
- Characters/LeaderCharacter.h / .cpp (NavInvoker, waypoints, pacing)
- AI/SoldierAIController.h / .cpp (perception, memory, suppression, crowd)
- AI/EnemyAttackCoordinator.h / .cpp (5-phase tactical attacks)
**Total: 12 files**

### Batch 5: StateTree Tasks + Performance
- AI/StateTreeTasks/ShooterAIStateTreeData.h / .cpp (FST_RuntimeData, evaluators)
- AI/StateTreeTasks/STTask_TakeCover.h / .cpp
- AI/StateTreeTasks/STTask_EngageTarget.h / .cpp
- AI/StateTreeTasks/STTask_FollowLeader.h / .cpp
- AI/StateTreeTasks/STTask_LeanOut.h / .cpp
- AI/StateTreeTasks/STTask_HoldPosition.h / .cpp
- Performance/SoldierRegistrySubsystem.h / .cpp (2D spatial hash)
- Performance/AISignificanceManager.h / .cpp (LOD wrapper)
- Performance/DifficultySubsystem.h / .cpp (one-call difficulty)
**Total: 17 files**

### Batch 6: Quest System
- Quest/QuestTypes.h (enums, FWaveDefinition, FQuestHudSnapshot)
- Quest/QuestTags.h (FQuestTag)
- Quest/QuestRegistry.h / .cpp (TMap tag lookup)
- Quest/QuestZone.h / .cpp (trigger volume)
- Quest/WaveSpawner.h / .cpp (timer-driven spawning + pool)
- Quest/WaveTemplate.h (reusable DataAsset)
- Quest/AutoSpawnZone.h / .cpp (one-drop encounter)
- Quest/EnemyPool.h / .cpp (pre-warm + reuse)
- Quest/QuestObjective.h / .cpp (base + 4 subclasses)
- Quest/QuestMission.h / .cpp (ordered objectives + music)
- Quest/QuestDirector.h / .cpp (auto-discover + chain)
- Quest/QuestManagerSubsystem.h / .cpp (HUD updates)
- Quest/QuestTickerSubsystem.h / .cpp (4Hz active-only tick)
- Quest/QuestValidator.h / .cpp (startup error detection)
- Quest/QuestLog.h / .cpp (mission history + serialization)
**Total: 22 files**

### Batch 7: HUD (Farsi + Minimap + Compass)
- HUD/FarsiTextHelper.h / .cpp (Persian digits, RTL, cardinals)
- HUD/MissionBriefingWidget.h / .cpp (top-left card, NineSlice border)
- HUD/MissionMinimapWidget.h / .cpp (NativePaint 2D, no SceneCapture)
- HUD/CompassWidget.h / .cpp (horizontal strip, objective marker)
- HUD/QuestHUDWidget.h / .cpp (master container, BindWidgetOptional)
**Total: 10 files**

### Documentation
- BUILD_ORDER.md (this file)
- SETUP_GUIDE.md (step-by-step integration)
- TECHNICAL_SUMMARY.md (algorithms + decisions)
- TUNING.md (every tweakable value)
- QUEST_RECIPES.md (drag-and-drop quest creation)
- HUD_SETUP.md (Persian font + minimap + compass)

**GRAND TOTAL: 87 files (77 source + 6 docs + 4 guides)**
