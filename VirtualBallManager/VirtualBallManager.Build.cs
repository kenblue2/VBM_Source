// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class VirtualBallManager : ModuleRules
{
	public VirtualBallManager(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        //PrivateIncludePaths.Add("Runtime/AnimGraphRuntime/Private");

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "AnimationCore", "AnimGraph", "AnimGraphRuntime"});

        PrivateDependencyModuleNames.AddRange(new string[] {
              "UnrealED", "GraphEditor", "AnimGraph", "AnimGraphRuntime", "BlueprintGraph"});

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
