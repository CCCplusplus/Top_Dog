// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Top_Dog : ModuleRules
{
	public Top_Dog(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
