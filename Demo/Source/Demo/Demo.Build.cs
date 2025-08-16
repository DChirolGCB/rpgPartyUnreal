// Fill out your copyright notice in the Description page of Project Settings.
using UnrealBuildTool;

public class Demo : ModuleRules
{
    public Demo(ReadOnlyTargetRules Target) : base(Target)
    {
        // OBLIGATOIRE pour UE5 - Active IWYU et PCH partagés
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Modules publics essentiels pour ActorComponents
        PublicDependencyModuleNames.AddRange(new string[] {
    "Core", "CoreUObject", "Engine", "InputCore", "UMG", "Slate", "SlateCore", "Paper2D"
});

        // Modules privés pour fonctionnalités avancées
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore"
        });

        // FIX pour UE5.6 : Force C++20 et désactive IWYU strict
        CppStandard = CppStandardVersion.Cpp20;
        bEnforceIWYU = false;
    }
}