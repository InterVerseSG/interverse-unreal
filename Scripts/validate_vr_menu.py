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
ANCHORS = ROOT / "Config" / "InterVerseCampusAnchors.json"


def require(path: pathlib.Path, token: str) -> None:
    text = path.read_text(encoding="utf-8")
    if token not in text:
        raise AssertionError(f"Missing {token!r} in {path.relative_to(ROOT)}")


def main() -> int:
    for path in (BUILD, PAWN_H, PAWN_CPP, MENU_H, MENU_CPP, ANCHORS):
        if not path.exists():
            raise AssertionError(f"Missing required VR menu file: {path.relative_to(ROOT)}")

    for module in ('"UMG"', '"Slate"', '"SlateCore"'):
        require(BUILD, module)

    for token in (
        "UWidgetComponent",
        "UWidgetInteractionComponent",
        "SetVRMenuVisible",
        "IsVRMenuVisible",
        "MenuAction",
    ):
        require(PAWN_H, token)

    for token in (
        "InterVerseVRMenuWidget::StaticClass()",
        "EWidgetSpace::World",
        "OculusTouch_Left_X_Click",
        "OculusTouch_Right_Trigger_Click",
        "PressPointerKey(EKeys::LeftMouseButton)",
        "ReleasePointerKey(EKeys::LeftMouseButton)",
        "ToggleVRMenu",
    ):
        require(PAWN_CPP, token)

    required_navs = {
        "NAV_MarquisScienceHall",
        "NAV_CAI",
        "NAV_CentroEstudiantes",
        "NAV_EscuelaGraduada",
    }
    anchors = json.loads(ANCHORS.read_text(encoding="utf-8"))
    existing_navs = {a.get("navigation_anchor") for a in anchors.get("anchors", [])}
    if not required_navs.issubset(existing_navs):
        raise AssertionError(f"VR menu references missing anchors: {sorted(required_navs - existing_navs)}")

    menu_text = MENU_CPP.read_text(encoding="utf-8")
    for nav in required_navs:
        if nav not in menu_text:
            raise AssertionError(f"VR menu does not include required destination {nav}")
    if "StartGuidanceToAnchor" not in menu_text:
        raise AssertionError("VR menu must start guidance rather than teleporting the user immediately")

    print("PASS: Quest VR menu source architecture")
    print("PASS: runtime UMG + right-controller widget interaction")
    print("PASS: priority NAV destinations are valid")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}")
        raise SystemExit(1)
