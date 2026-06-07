# SquadAI — Complete Beginner Setup Guide
## Step-by-step from "I copied the files" to "it works"

---

## STEP 0: Verify Your Folder Structure

Your project should look like this right now:

```
YourProject/
├── Config/
├── Content/
├── Source/
│   ├── YourProject/          ← Your existing C++ module
│   │   ├── YourProject.Build.cs
│   │   ├── YourProject.h
│   │   ├── YourProject.cpp
│   │   ├── (your existing .h and .cpp files)
│   │   └── ...
│   │
│   ├── SquadAI/              ← The folder you just copied
│   │   ├── SquadAI.Build.cs
│   │   ├── SquadAI.h
│   │   ├── SquadAI.cpp
│   │   ├── Public/
│   │   │   ├── SquadTypes.h
│   │   │   ├── SquadAITuning.h
│   │   │   ├── CoverSystem/
│   │   │   ├── Components/
│   │   │   ├── Characters/
│   │   │   ├── AI/
│   │   │   ├── Performance/
│   │   │   ├── Quest/
│   │   │   └── HUD/
│   │   └── Private/
│   │       └── (matching .cpp files)
│   │
│   ├── YourProject.Target.cs
│   └── YourProjectEditor.Target.cs
│
├── YourProject.uproject
└── YourProject.sln
```

**If it looks different, fix it now before continuing.**

---

## STEP 1: Register SquadAI as a Module in Your .uproject

1. Open **`YourProject.uproject`** in a text editor (Notepad, VS Code, etc.)

2. Find the `"Modules"` section. It looks like this:
```json
{
    "FileVersion": 3,
    "EngineAssociation": "5.7",
    "Category": "",
    "Description": "",
    "Modules": [
        {
            "Name": "YourProject",
            "Type": "Runtime",
            "LoadingPhase": "Default"
        }
    ]
}
```

3. **ADD the SquadAI module** to the Modules array (add a comma after the first module):
```json
{
    "FileVersion": 3,
    "EngineAssociation": "5.7",
    "Category": "",
    "Description": "",
    "Modules": [
        {
            "Name": "YourProject",
            "Type": "Runtime",
            "LoadingPhase": "Default"
        },
        {
            "Name": "SquadAI",
            "Type": "Runtime",
            "LoadingPhase": "Default"
        }
    ],
    "Plugins": [
        {
            "Name": "StateTree",
            "Enabled": true
        },
        {
            "Name": "GameplayStateTree",
            "Enabled": true
        },
        {
            "Name": "SignificanceManager",
            "Enabled": true
        },
        {
            "Name": "SmartObjectsModule",
            "Enabled": true
        },
        {
            "Name": "GameplayBehaviors",
            "Enabled": true
        }
    ]
}
```

**Important:** If you already have a `"Plugins"` section, just ADD the missing plugins
to it (don't duplicate plugins that are already there). If you don't have a `"Plugins"`
section at all, add the whole block as shown above.

4. **Save the file.**

---

## STEP 2: Add SquadAI as a Dependency to Your Module

1. Open **`Source/YourProject/YourProject.Build.cs`** in a text editor

2. Find the line that says `PublicDependencyModuleNames.AddRange` — it looks like:
```csharp
PublicDependencyModuleNames.AddRange(new string[] { 
    "Core", "CoreUObject", "Engine", "InputCore" 
});
```

3. **ADD `"SquadAI"` to the list:**
```csharp
PublicDependencyModuleNames.AddRange(new string[] { 
    "Core", "CoreUObject", "Engine", "InputCore",
    "SquadAI"
});
```

4. **Save the file.**

---

## STEP 3: Add Navigation Settings to DefaultEngine.ini

1. Open **`Config/DefaultEngine.ini`** in a text editor

2. **Add these lines at the END of the file** (scroll to the very bottom):
```ini
[/Script/NavigationSystem.NavigationSystemV1]
bGenerateNavigationOnlyAroundNavigationInvokers=True
RuntimeGeneration=Dynamic

[/Script/Engine.Engine]
+ActiveClassRedirects=(OldClassName="",NewClassName="")
```

3. **Save the file.**

**What this does:** Tells UE to only generate NavMesh around the leader
(via NavigationInvoker), not the entire 8km map. Without this, large maps
crash during cooking.

---

## STEP 4: Regenerate Project Files and Build

### Method A: From File Explorer (Easiest)

1. **Close Unreal Engine** if it's open
2. **Close Visual Studio** if it's open
3. Find your **`YourProject.uproject`** file in File Explorer
4. **Right-click** on it → **"Generate Visual Studio project files"**
5. Wait for it to finish (a few seconds)
6. **Double-click** the new **`YourProject.sln`** file to open Visual Studio
7. Set Configuration to **`Development Editor`** and **`Win64`**
8. Press **`Ctrl+Shift+B`** to Build
9. Wait for compilation (first build takes 3-8 minutes)

### Method B: From Unreal Engine

1. **Close Unreal Engine** if it's open
2. Delete these folders from your project (if they exist):
   - `Binaries/`
   - `Intermediate/`
3. Double-click `YourProject.uproject` to open UE
4. UE will detect the new module and ask to compile — click **Yes**
5. Wait for compilation

### If You Get Errors:

**"SetEnv task failed"** → See the fix from earlier in our conversation:
- Open `%AppData%\Unreal Engine\UnrealBuildTool\BuildConfiguration.xml`
- Delete the file or remove `MaxSharedIncludePaths` entries
- Regenerate project files

**"Module X not found"** → Check that the plugin names in .uproject match exactly:
`StateTree`, `GameplayStateTree`, `SignificanceManager`, `SmartObjectsModule`, `GameplayBehaviors`

**Compile errors in SquadAI code** → Post the exact error messages and I'll help fix them.

---

## STEP 5: Enable Plugins in the Editor

After the project compiles and UE opens:

1. Go to **Edit → Plugins**
2. Search for each of these and make sure they're **enabled** (checkmark):
   - ✅ **State Tree**
   - ✅ **Gameplay State Tree**
   - ✅ **Significance Manager**
   - ✅ **Smart Objects**
   - ✅ **Gameplay Behaviors**
   - ✅ **Environment Query Editor** (for EQS visual debugging)
3. **Restart the editor** if prompted

Most of these should already be enabled from the .uproject changes.

---

## STEP 6: Verify SquadAI Tuning Panel

1. Go to **Edit → Project Settings**
2. Scroll down the left sidebar to **Plugins**
3. You should see **"Squad AI Tuning"** with all the tuning categories:
   - Cover Scanner
   - Combat | Weapon
   - Combat | Health
   - Combat | Aim
   - Perception
   - Faction | Allies
   - Faction | Enemies
   - Leader
   - Performance
   - Difficulty
   - Quest

**If you see this panel, SquadAI is working!**

---

## STEP 7: Create Blueprint Subclasses

You need to create Blueprint children of the C++ classes so you can assign
meshes, animations, and configure them visually.

### Create BP_EnemySoldier:
1. Content Browser → Right-click → **Blueprint Class**
2. In the picker, search for **`EnemySoldier`** → select it → **Create**
3. Name it **`BP_EnemySoldier`**
4. Open it and assign:
   - Skeletal Mesh (your soldier model)
   - Animation Blueprint
   - AI Controller Class → create a new one (see below)

### Create BP_AllySoldier:
1. Same process → parent: **`AllySoldier`**
2. Name it **`BP_AllySoldier`**

### Create BP_Leader:
1. Same process → parent: **`LeaderCharacter`**
2. Name it **`BP_Leader`**

### Create AI Controllers:
1. Content Browser → Right-click → **Blueprint Class**
2. Search for **`SoldierAIController`** → Create
3. Name it **`BP_AIController_Soldier`**
4. Open it → in the Details panel, assign your **StateTree asset** to the
   StateTree component (you'll create the StateTree in Step 8)

### Assign AI Controller to Characters:
1. Open **BP_EnemySoldier** → Class Defaults
2. Find **"AI Controller Class"** → set to **`BP_AIController_Soldier`**
3. Find **"Auto Possess AI"** → set to **"Placed in World or Spawned"**
4. Repeat for BP_AllySoldier and BP_Leader

---

## STEP 8: Create StateTree Assets

### Create Enemy StateTree:
1. Content Browser → Right-click → **AI → State Tree**
2. Name it **`ST_EnemyBehavior`**
3. Open it → set Schema to **"StateTree AI Component Schema"**
4. Set **Context Actor Class** to your **BP_EnemySoldier** (or **EnemySoldier**)

5. Build the tree (this is done visually in the editor):
```
Root
├── [Evaluator] Evaluate Threat
├── [Evaluator] Evaluate Suppression
├── [Evaluator] Evaluate Health
│
├── State: IDLE
│   └── Task: Hold Position
│   └── Transition → TAKE_COVER when: ThreatConfidence > 0
│
├── State: TAKE_COVER
│   └── Task: Take Cover (SearchRadius = 2000)
│   └── On Success → ENGAGE
│   └── On Fail → IDLE
│
├── State: ENGAGE
│   └── Task: Engage Target (PeekMin=0.8, PeekMax=2.0, MaxCycles=3)
│   └── On Success → TAKE_COVER (find new cover, maybe closer)
│
└── Global: → IDLE when ThreatConfidence = 0
```

### Create Ally StateTree:
1. Same process → **`ST_AllyBehavior`**
2. Build:
```
Root
├── [Evaluator] Evaluate Threat
│
├── State: FOLLOW
│   └── Task: Follow Leader
│   └── Transition → TAKE_COVER when: ThreatConfidence > 0.5
│
├── State: TAKE_COVER
│   └── Task: Take Cover → Engage Target
│   └── Transition → FOLLOW when: ThreatConfidence = 0
```

### Assign StateTrees to AI Controllers:
1. Open **BP_AIController_Soldier**
2. Select the **StateTree Component** in the Components panel
3. Set **"State Tree"** to your **ST_EnemyBehavior** (or ST_AllyBehavior)

---

## STEP 9: Set Up Your Level

### Place a NavMeshBoundsVolume:
1. Place Actors → Volumes → **NavMeshBoundsVolume**
2. Scale it to cover your entire play area
3. Build navigation: **Build → Build Paths**

### Place Characters:
1. Drag **BP_Leader** into the level near the Player Start
2. Drag 3-4 **BP_AllySoldier** near the leader
3. Drag several **BP_EnemySoldier** wherever you want enemies

### Set Up a Quest (Optional):
1. Drag **QuestDirector** into the level
2. Drag **AutoSpawnZone** → set Tag to `ZoneA`, assign BP_EnemySoldier class
3. Drag **QuestMission** → set Order=1, add an AutoZone objective with tag `ZoneA`
4. That's it — tag-based auto-discovery does the rest

---

## STEP 10: Press Play!

You should see:
- 🔍 Cover points auto-detected (if debug draw is on)
- 🧠 Enemies finding cover, peeking, shooting
- 🛡️ Allies following the leader, shooting (missing a lot)
- 👑 Leader walking toward waypoints, waiting for you

---

## TROUBLESHOOTING

### "Unresolved external symbol" errors
Your project's Build.cs might be missing dependencies. Make sure it has `"SquadAI"`
in PublicDependencyModuleNames.

### "Cannot find module SquadAI"
Check that:
- The SquadAI folder is at `Source/SquadAI/` (not `Source/SquadAI/SquadAI/`)
- The .uproject has the SquadAI module entry
- You regenerated project files after editing .uproject

### "Plugin X not found"
The plugin names are case-sensitive. Check spelling in .uproject:
- `StateTree` (not "StateTrees")
- `SmartObjectsModule` (not "SmartObjects")
- `GameplayBehaviors` (not "GameplayBehavior")
- `SignificanceManager` (not "Significance")

### AI doesn't move / stands still
- Check NavMeshBoundsVolume covers the area
- Check AI Controller is assigned on the character
- Check StateTree asset is assigned on the AI Controller
- Check "Auto Possess AI" is set to "Placed in World or Spawned"

### AI doesn't see the player
- Check the player character has a team ID different from the AI
- The player needs an AIPerceptionStimuliSourceComponent (or be a Pawn — UE auto-registers pawns)

### Build takes forever / runs out of memory
- Make sure `bGenerateNavigationOnlyAroundNavigationInvokers=True` is in DefaultEngine.ini
- This is critical for large maps

---

## WHAT'S NEXT

After the basic setup works:

1. **Import your Persian font** for the HUD (drag .ttf into Content Browser)
2. **Create WBP_MissionBriefing** widget (parent: MissionBriefingWidget)
3. **Create WBP_Minimap** widget (parent: MissionMinimapWidget)
4. **Create WBP_Compass** widget (parent: CompassWidget)
5. **Assign your HUD pack border textures** in the widget Details panels
6. **Create WaveTemplate DataAssets** for your enemy waves
7. **Tune everything** in Project Settings → Plugins → Squad AI Tuning
