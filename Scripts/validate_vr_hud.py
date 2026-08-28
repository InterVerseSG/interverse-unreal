"""Validate the source-level Quest navigation HUD without Unreal Engine."""

from __future__ import annotations

import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
HUD_H = ROOT / "Source" / "InterVerseRuntime" / "Public" / "InterVerseVRHudWidget.h"
HUD_CPP = ROOT / "Source" / "InterVerseRuntime" / "Private" / "InterVerseVRHudWidget.cpp"
HUD_ACTOR_H = ROOT / "Source" / "InterVerseRuntime" / "Public" / "InterVerseVRHudActor.h"
HUD_ACTOR_CPP = ROOT / "Source" / "InterVerseRuntime" / "Private" / "InterVerseVRHudActor.cpp"
NAV_H = ROOT / "Source" / "InterVerseRuntime" / "Public" / "InterVerseNavigationComponent.h"
NAV_CPP = ROOT / "Source" / "InterVerseRuntime" / "Private" / "InterVerseNavigationComponent.cpp"
BUILD = ROOT / "Source" / "InterVerseRuntime" / "InterVerseRuntime.Build.cs"
BOOTSTRAP = ROOT / "Scripts" / "bootstrap_interverse_level.py"


def require(path: pathlib.Path, token: str) -> None:
    if not path.exists():
        raise AssertionError(f"Missing HUD source: {path.relative_to(ROOT)}")
    text = path.read_text(encoding="utf-8")
    if token not in text:
        raise AssertionError(f"Missing {token!r} in {path.relative_to(ROOT)}")


def main() -> int:
    for token in (
        "NativeTick",
        "FriendlyNameForAnchor",
        "DirectionText",
        "ArrivalThresholdCm",
        "HandleValidatedCommand",
        "HandleCloudError",
    ):
        require(HUD_H, token)

    for token in (
        "InterVerseCampusAnchors.json",
        "Llegaste al destino",
        "m restantes",
        "Continúa al frente",
        "Gira hacia la derecha",
        "Gira hacia la izquierda",
        "El destino está detrás de ti",
        "Gemini + Builder · comando validado",
        "Builder rechazó la instrucción",
        "FLinearColor(0.0f, 0.31f, 0.24f, 0.94f)",
        "FLinearColor(0.996f, 0.82f, 0.25f, 1.0f)",
    ):
        require(HUD_CPP, token)

    for token in ("UWidgetComponent", "RelativeLocationCm", "DrawSize"):
        require(HUD_ACTOR_H, token)

    for token in (
        "InterVerseVRHudWidget::StaticClass()",
        "AttachToComponent(Pawn->Camera",
        "SetOwnerPlayer",
        "SetCollisionEnabled(ECollisionEnabled::NoCollision)",
    ):
        require(HUD_ACTOR_CPP, token)

    require(BUILD, '"NavigationSystem"')
    for token in (
        "bUseNavMeshGuidance",
        "GuidanceWaypointAcceptanceCm",
        "IsUsingNavMeshGuidance",
        "GuidancePathPoints",
        "GuidancePointIndex",
    ):
        require(NAV_H, token)
    for token in (
        "FindPathToLocationSynchronously",
        "BuildGuidancePath",
        "RemainingPathDistanceFrom",
        "GuidancePathPoints = Path->PathPoints",
        "GuidanceDistanceCm = RemainingPathDistanceFrom(CurrentLocation)",
        "bGuidanceUsingNavMesh = false",
    ):
        require(NAV_CPP, token)

    for token in (
        "_ensure_vr_navigation_hud",
        "InterVerseVRHudActor",
        "IV_VRNavigationHUD",
        "_ensure_green_area_actor",
        "IV_CampusGreenAreas",
    ):
        require(BOOTSTRAP, token)

    print("PASS: Quest navigation HUD source architecture")
    print("PASS: friendly destination + distance + direction + arrival state")
    print("PASS: Gemini/Builder status surfaced in HUD")
    print("PASS: NavMesh waypoint route guidance with direct fallback")
    print("PASS: HUD and green areas included in editor bootstrap")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}")
        raise SystemExit(1)
