# SoldierAI — Technical Architecture & Developer Guide

## 1. System Overview
SquadAI is a modular, high-performance AI framework for Unreal Engine 5.7. It is designed to handle up to 50+ intelligent agents in a tactical shooter environment with minimal CPU overhead.

### Core Modules:
*   **Characters**: Standardized base classes for Allies, Enemies, and Leaders.
*   **AI (Brains)**: StateTree-based decision making with custom tasks.
*   **Combat**: Physics-based aiming (Springs) and ballistic hitscan weapons.
*   **Quest**: A tag-based automated mission and wave system.
*   **Performance**: Spatial hashing (Registry) and 6-bucket LOD (Significance).

---

## 2. File Connections & Logic Flow

### A. How AI "Sees" (Perception)
*   **SoldierAIController**: Owns the `AIPerceptionComponent`.
*   **OnPerceptionUpdated**: Fills the `Memory` array with targets. 
*   **Global Attitude Solver**: Located in `SquadAI.cpp`, it defines Team 0 vs Team 1 as hostiles.
*   **PerceptionMemory**: A standalone object that scores threats based on "Sense Fusion" (Sight/Hearing/Damage).

### B. How AI "Moves" (Decision to Feet)
1.  **StateTree**: Evaluates `ThreatConfidence` and `Health`.
2.  **Tasks**: `STTask_LeaderPath` or `STTask_EnemyMission` are triggered.
3.  **UAITask_MoveTo**: These tasks call the engine's movement engine.
4.  **SoldierCharacter**: Receives the velocity and rotation commands.
    *   `bRequestedMoveUseAcceleration = true` ensures animations play in Manny.
    *   `bOrientRotationToMovement = true` ensures the body turns naturally.

### C. The Quest System (The Command Chain)
*   **QuestDirector**: The "God" actor. It finds all `QuestMission` actors in the map.
*   **QuestMission**: Contains an array of `UQuestObjective`.
*   **Auto Zone Objective**: 
    1.  Starts and finds the nearest `QuestZone`.
    2.  **Broadcasts**: Automatically updates the `CurrentActiveGoal` GPS Beacon in `UQuestRegistry`.
    3.  **Polling**: Every AI task (Enemy and Leader) polls this GPS Beacon every tick to find their destination.

---

## 3. Key C++ Files for Developers

| File | Responsibility |
| :--- | :--- |
| `SoldierAIController.cpp` | Perception processing and brain activation. |
| `SoldierCharacter.cpp` | Movement physics, rotation rates, and health/death. |
| `STTask_LeaderPath.cpp` | Smooth, jitter-free movement for the Leader via GPS Beacon. |
| `STTask_EnemyMission.cpp` | Automatic hunting and zone defense for Enemies via GPS Beacon. |
| `WeaponComponent.cpp` | Hitscan math, target leading, and muzzle clearance. |
| `SoldierRegistrySubsystem.cpp` | The O(K) "Radar" that makes counters and suppression fast. |

---

## 4. Updates & Recent Edits
*   **Anti-Jitter (June 2026)**: Added "Path-Lock" logic. AI stays on its path until the target moves > 10 meters.
*   **GPS Beacon**: Added `CurrentActiveGoal` to `QuestRegistry` so all AI can find the objective instantly.
*   **Manny Sync**: Forced `bRequestedMoveUseAcceleration` to ensure Manny's legs move with the character's speed.
*   **Global Team Solver**: Moved attitude logic to the Module level to bypass UE 5.7 virtual function conflicts.

---

## 5. Setup for Beginners
1.  Place **QuestDirector** and **QuestMission**.
2.  Add a **QuestObjective_AutoZone** to the mission.
3.  Tag it (e.g. `Zone1`).
4.  Place a **QuestZone** with the same tag far away.
5.  **Press Play.** The system will handle the rest.
