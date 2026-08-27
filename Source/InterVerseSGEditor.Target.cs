using UnrealBuildTool;
using System.Collections.Generic;

public class InterVerseSGEditorTarget : TargetRules
{
    public InterVerseSGEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("InterVerseRuntime");
    }
}
