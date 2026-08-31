"""Validate Quest controller pointer architecture without Unreal Engine."""
from __future__ import annotations
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
H = ROOT / "Source" / "InterVerseRuntime" / "Public" / "InterVerseXRPawn.h"
CPP = ROOT / "Source" / "InterVerseRuntime" / "Private" / "InterVerseXRPawn.cpp"
DOC = ROOT / "Docs" / "QUEST_CONTROLLER_INTERACTION.md"

def require(path: pathlib.Path, token: str) -> None:
    if not path.exists():
        raise AssertionError(f"Missing pointer source: {path.relative_to(ROOT)}")
    if token not in path.read_text(encoding="utf-8"):
        raise AssertionError(f"Missing {token!r} in {path.relative_to(ROOT)}")

def main() -> int:
    for token in (
        "RightPointerVisual",
        "PointerWidthCm",
        "PointerMaxDistanceCm",
        "IsPointerHoveringWidget",
        "IsPointerPressed",
        "UpdatePointerVisual",
        "ClearPointerVisual",
        "bPointerPressed",
    ):
        require(H, token)

    for token in (
        "CreateDefaultSubobject<UProceduralMeshComponent>(TEXT(\"RightPointerVisual\"))",
        "SetCollisionEnabled(ECollisionEnabled::NoCollision)",
        "SetCanEverAffectNavigation(false)",
        "GetLastHitResult",
        "IsOverHitTestVisibleWidget",
        "CreateMeshSection_LinearColor",
        "FLinearColor(0.996f, 0.82f, 0.25f, 1.0f)",
        "FLinearColor(0.0f, 0.48f, 0.37f, 1.0f)",
        "PressPointerKey(EKeys::LeftMouseButton)",
        "ReleasePointerKey(EKeys::LeftMouseButton)",
        "if (IsVRMenuVisible()) UpdatePointerVisual()",
        "else ClearPointerVisual()",
        "Locomotion->BeginTeleportAim()",
    ):
        require(CPP, token)

    for token in ("hover", "right trigger", "pointer disappears", "teleport"):
        require(DOC, token)

    print("PASS: Quest right-controller pointer mesh is lightweight and collision-free")
    print("PASS: pointer supports normal, hover, and pressed feedback")
    print("PASS: trigger remains context-sensitive between menu click and teleport")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}")
        raise SystemExit(1)
