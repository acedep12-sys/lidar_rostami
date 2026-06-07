// SquadAI.Build.cs — Complete module dependencies for the merged system
using UnrealBuildTool;

public class SquadAI : ModuleRules
{
	public SquadAI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// ---- Runtime dependencies ----
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",

			// AI
			"AIModule",
			"NavigationSystem",
			"GameplayTasks",

			// StateTree
			"StateTreeModule",
			"GameplayStateTreeModule",

			// Performance
			"SignificanceManager",

			// Smart Objects
			"SmartObjectsModule",

			// Tags
			"GameplayTags",

			// Settings
			"DeveloperSettings",

			// UMG (for HUD)
			"UMG",
			"Slate",
			"SlateCore",
		});

		// Optional modules — only add if they exist in your UE build
		// If any of these cause "module not found", remove them
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"GameplayBehaviorsModule",
		});

		// ---- Editor-only dependencies ----
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd",
				"AssetTools",
				"AssetRegistry",
				"ToolMenus",
			});
		}
	}
}
