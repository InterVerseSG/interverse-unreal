"""Validate lightweight Quest controller visual fallbacks without Unreal Engine."""

from __future__ import annotations

import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
H = ROOT / "Source" / "InterVerseRuntime" / "Public" / "InterVerseXRPawn.h"
CPP = ROOT / "Source" / "InterVerseRuntime" / "Private" / "InterVerseXRPawn.cpp"
DOC = ROOT / "Docs" / "QUEST_CONTROLLER_INTERACTION.md"


def require(path: pathlib.Path, token: str) -> None:
    if not path.exists():
        raise AssertionError(f"Missing controller visual source: {path.relative_to(ROOT)}")
    text = path.read_text(encoding="utf-8")
    if token not in text:
        raise AssertionError(f"Missing {token!r} in {path.relative_to(ROOT)}")


def main() -> int:
    for token in (
        "UStaticMeshComponent",
        "LeftControllerVisual",
        "RightControllerVisual",
        "bShowFallbackControllerMeshes",
        "SetFallbackControllerVisualsEnabled",
    ):
        require(H, token)

    for token in (
        "/Engine/BasicShapes/Cube.Cube",
        "LeftControllerVisual->SetupAttachment(LeftController)",
        "RightControllerVisual->SetupAttachment(RightController)",
        "SetCollisionEnabled(ECollisionEnabled::NoCollision)",
        "SetCanEverAffectNavigation(false)",
        "SetCastShadow(false)",
        "bCastDynamicShadow = false",
        "SetFallbackControllerVisualsEnabled(bShowFallbackControllerMeshes)",
        "SetVisibility(bEnabled, true)",
    ):
        require(CPP, token)

    for token in (
        "controller-first interaction layer",
        "optional articulated hand meshes/hand tracking",
        "pointer disappears",
    ):
        require(DOC, token)

    print("PASS: Quest tracked controller fallback visuals")
    print("PASS: engine-only low-poly mesh, no collision/navigation/shadows")
    print("PASS: fallback can be disabled for future Meta XR hand/controller models")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}")
        raise SystemExit(1)
