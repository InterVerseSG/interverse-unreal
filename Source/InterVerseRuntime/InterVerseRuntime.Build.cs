using UnrealBuildTool;

public class InterVerseRuntime : ModuleRules
{
    public InterVerseRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "HTTP",
            "Json",
            "JsonUtilities",
            "DeveloperSettings",
            "ProceduralMeshComponent",
            "HeadMountedDisplay",
            "InputCore",
            "EnhancedInput",
            "UMG",
            "Slate",
            "SlateCore"
        });
    }
}
