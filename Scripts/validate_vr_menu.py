"""Validate the source-level Quest VR menu architecture without Unreal Engine."""

from __future__ import annotations

import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
BUILD = ROOT / "Source" / "InterVerseRuntime" / "InterVerseRuntime.Build.cs"
PAWN_H = ROOT / "Source" / "InterVerseRuntime" / "Public" / "InterVerseXRPawn.h"
PAWN_CPP = ROOT / "Source" / "InterVerseRuntime" / "Private" / "InterVerseXRPawn.cpp"
MENU_H = ROOT / "Source" / "InterVerseRuntime" / "Public" / "InterVerseVRMenuWidget.h"
MENU_CPP = ROOT / "Source" / "InterVerseRuntime" / "Private" / "InterVerseVRMenuWidget.cpp"
CLOUD_H = ROOT / "Source" / "InterVerseRuntime" / "Public" / "InterVerseCloudClient.h"
CLOUD_CPP = ROOT / "Source" / "InterVerseRuntime" / "Private" / "InterVerseCloudClient.cpp"
ANCHORS = ROOT / "Config" / "InterVerseCampusAnchors.json"


def require(path: pathlib.Path, token: str) -> None:
    text = path.read_text(encoding="utf-8")
    if token not in text:
        raise AssertionError(f"Missing {token!r} in {path.relative_to(ROOT)}")


def main() -> int:
    for path in (BUILD, PAWN_H, PAWN_CPP, MENU_H, MENU_CPP, CLOUD_H, CLOUD_CPP, ANCHORS):
        if not path.exists():
            raise AssertionError(f"Missing required VR menu file: {path.relative_to(ROOT)}")

    for module in ('"UMG"', '"Slate"', '"SlateCore"'):
        require(BUILD, module)

    for token in (
        "UWidgetComponent", "UWidgetInteractionComponent", "SetVRMenuVisible",
        "IsVRMenuVisible", "MenuAction",
    ):
        require(PAWN_H, token)

    for token in (
        "InterVerseVRMenuWidget::StaticClass()", "EWidgetSpace::World",
        "OculusTouch_Left_X_Click", "OculusTouch_Right_Trigger_Click",
        "PressPointerKey(EKeys::LeftMouseButton)", "ReleasePointerKey(EKeys::LeftMouseButton)",
        "ToggleVRMenu",
    ):
        require(PAWN_CPP, token)

    anchors = json.loads(ANCHORS.read_text(encoding="utf-8"))
    anchor_rows = anchors.get("anchors", [])
    existing_navs = {a.get("navigation_anchor") for a in anchor_rows}
    required_navs = {"NAV_MarquisScienceHall", "NAV_CAI", "NAV_CentroEstudiantes", "NAV_EscuelaGraduada"}
    if not required_navs.issubset(existing_navs):
        raise AssertionError(f"Missing priority anchors: {sorted(required_navs - existing_navs)}")
    if len(existing_navs) < 10:
        raise AssertionError("Campus anchor registry unexpectedly small for dynamic Quest menu")

    menu_text = MENU_CPP.read_text(encoding="utf-8")
    dynamic_tokens = (
        "Config/InterVerseCampusAnchors.json",
        "TryGetArrayField(TEXT(\"anchors\")",
        "UComboBoxString",
        "UEditableTextBox",
        "RebuildCategoryOptions",
        "RebuildDestinationOptions",
        "GuideSelectedDestination",
        "StopActiveGuidance",
        "StartGuidanceToAnchor",
        "Edificios académicos",
        "Servicios estudiantiles",
        "Biblioteca y recursos",
        "Estudios graduados",
        "Deportes",
        "InterGreen",
        "InterYellow",
        "IA: Guíame a Escuela Graduada",
        "AskAssistant(Request)",
        "OnCommandValidated.AddDynamic",
        "OnCloudError.AddDynamic",
        "Consultando Gemini y validando con Builder",
    )
    for token in dynamic_tokens:
        if token not in menu_text:
            raise AssertionError(f"Dynamic Quest menu missing token: {token}")

    require(MENU_H, "TArray<FInterVerseVRMenuDestination> Destinations")
    require(MENU_H, "UComboBoxString")
    require(MENU_H, "UEditableTextBox")
    require(CLOUD_H, "bValidatedNavigationStartsGuidance = true")
    require(CLOUD_CPP, "StartGuidanceToAnchor(Command.NavigationAnchor)")
    require(CLOUD_CPP, "Navigation->ExecuteValidatedCommand(Command)")

    print(f"PASS: dynamic Quest VR destination registry ({len(existing_navs)} NAV anchors)")
    print("PASS: categories + text filter + stop guidance")
    print("PASS: institutional green/yellow runtime UI")
    print("PASS: Gemini -> Builder -> guided Quest navigation")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}")
        raise SystemExit(1)
