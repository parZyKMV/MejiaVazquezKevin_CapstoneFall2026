// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MejiaVazK_Capstone : ModuleRules
{
	public MejiaVazK_Capstone(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"MejiaVazK_Capstone",
			"MejiaVazK_Capstone/Variant_Platforming",
			"MejiaVazK_Capstone/Variant_Platforming/Animation",
			"MejiaVazK_Capstone/Variant_Combat",
			"MejiaVazK_Capstone/Variant_Combat/AI",
			"MejiaVazK_Capstone/Variant_Combat/Animation",
			"MejiaVazK_Capstone/Variant_Combat/Gameplay",
			"MejiaVazK_Capstone/Variant_Combat/Interfaces",
			"MejiaVazK_Capstone/Variant_Combat/UI",
			"MejiaVazK_Capstone/Variant_SideScrolling",
			"MejiaVazK_Capstone/Variant_SideScrolling/AI",
			"MejiaVazK_Capstone/Variant_SideScrolling/Gameplay",
			"MejiaVazK_Capstone/Variant_SideScrolling/Interfaces",
			"MejiaVazK_Capstone/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
