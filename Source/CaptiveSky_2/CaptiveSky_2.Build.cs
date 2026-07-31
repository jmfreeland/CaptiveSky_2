// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CaptiveSky_2 : ModuleRules
{
	public CaptiveSky_2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"HTTP",
			"Json",
			"JsonUtilities",
			"RenderCore",
			"RHI",
			"DeveloperSettings"
		});

		PublicIncludePaths.AddRange(new string[] {
			"CaptiveSky_2",
			"CaptiveSky_2/Variant_Platforming",
			"CaptiveSky_2/Variant_Platforming/Animation",
			"CaptiveSky_2/Variant_Combat",
			"CaptiveSky_2/Variant_Combat/AI",
			"CaptiveSky_2/Variant_Combat/Animation",
			"CaptiveSky_2/Variant_Combat/Gameplay",
			"CaptiveSky_2/Variant_Combat/Interfaces",
			"CaptiveSky_2/Variant_Combat/UI",
			"CaptiveSky_2/Variant_SideScrolling",
			"CaptiveSky_2/Variant_SideScrolling/AI",
			"CaptiveSky_2/Variant_SideScrolling/Gameplay",
			"CaptiveSky_2/Variant_SideScrolling/Interfaces",
			"CaptiveSky_2/Variant_SideScrolling/UI",
			"CaptiveSky_2/Agent"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
