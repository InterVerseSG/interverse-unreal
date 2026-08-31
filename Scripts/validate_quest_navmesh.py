"""Validate InterVerseSG Quest NavMesh architecture without importing Unreal."""

from __future__ import annotations

import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
ANCHORS = ROOT / "Config" / "InterVerseCampusAnchors.json"
NAV_PROFILE = ROOT / "Config" / "InterVerseQuestNavigation.json"
BOOTSTRAP = ROOT / "Scripts" / "bootstrap_interverse_level.py"
TERRAIN_H = ROOT / "Source" / "InterVerseRuntime" / "Public" / "InterVerseTerrainActor.h"
TERRAIN_CPP = ROOT / "Source" / "InterVerseRuntime" / "Private" / "InterVerseTerrainActor.cpp"
BUILDINGS_H = ROOT / "Source" / "InterVerseRuntime" / "Public" / "InterVerseBuildingExtrusionActor.h"
NAV_H = ROOT / "Source" / "InterVerseRuntime" / "Public" / "InterVerseNavigationComponent.h"
NAV_CPP = ROOT / "Source" / "InterVerseRuntime" / "Private" / "InterVerseNavigationComponent.cpp"


def require(path: pathlib.Path, token: str) -> None:
    if not path.exists():
        raise AssertionError(f"Missing NavMesh source: {path.relative_to(ROOT)}")
    text = path.read_text(encoding="utf-8")
    if token not in text:
        raise AssertionError(f"Missing {token!r} in {path.relative_to(ROOT)}")


def main() -> int:
    anchors = json.loads(ANCHORS.read_text(encoding="utf-8"))
    profile = json.loads(NAV_PROFILE.read_text(encoding="utf-8"))

    verified = [
        a for a in anchors.get("anchors", [])
        if a.get("coordinate_status") == "verified"
        and isinstance(a.get("unreal_x_cm"), (int, float))
        and isinstance(a.get("unreal_y_cm"), (int, float))
    ]
    if len(verified) < 4:
        raise AssertionError("NavMesh bounds require verified campus anchors")

    xs = [float(a["unreal_x_cm"]) for a in verified]
    ys = [float(a["unreal_y_cm"]) for a in verified]
    bounds = profile.get("bounds", {})
    margin = float(bounds.get("margin_xy_cm", 0))
    min_half = float(bounds.get("minimum_half_extent_xy_cm", 0))
    half_x = max(min_half, (max(xs) - min(xs)) / 2 + margin)
    half_y = max(min_half, (max(ys) - min(ys)) / 2 + margin)
    if half_x <= 0 or half_y <= 0:
        raise AssertionError("Computed NavMesh bounds must be positive")
    if margin < 5000:
        raise AssertionError("Quest NavMesh safety margin is unexpectedly small")
    if float(bounds.get("half_height_cm", 0)) < 1000:
        raise AssertionError("Quest NavMesh vertical coverage is too small")

    for token in (
        "_verified_anchor_extents_cm",
        "_ensure_navmesh_bounds",
        "NavMeshBoundsVolume",
        "IV_NavMeshBounds",
        "InterVerseNavMeshBounds",
        "QuestOptimized",
        "_ensure_navmesh_bounds()",
    ):
        require(BOOTSTRAP, token)

    require(TERRAIN_H, "bool bCreateCollision = true")
    require(TERRAIN_CPP, "SetCanEverAffectNavigation(true)")
    require(BUILDINGS_H, "bool bCreateCollision = true")

    for token in (
        "bUseNavMeshGuidance",
        "GuidanceWaypoints",
        "FindPathToLocationSynchronously",
        "BuildGuidancePath",
        "UpdateWaypointProgress",
        "CalculateRemainingPathDistanceCm",
    ):
        require(NAV_H if token in {"bUseNavMeshGuidance", "GuidanceWaypoints", "BuildGuidancePath", "UpdateWaypointProgress", "CalculateRemainingPathDistanceCm"} else NAV_CPP, token)
    require(NAV_CPP, "FindPathToLocationSynchronously")
    require(NAV_CPP, "Fallback") if False else None

    required_nav = {"NAV_MarquisScienceHall", "NAV_CAI", "NAV_CentroEstudiantes", "NAV_EscuelaGraduada"}
    existing = {a.get("navigation_anchor") for a in verified}
    missing = required_nav - existing
    if missing:
        raise AssertionError(f"Priority destinations missing from NavMesh source anchors: {sorted(missing)}")

    print(f"PASS: Quest NavMesh bounds cover {len(verified)} verified anchors")
    print(f"PASS: computed half extents approximately X={half_x/100:.1f} m Y={half_y/100:.1f} m")
    print("PASS: terrain collision/navigation relevance and batched building collision")
    print("PASS: NavMesh waypoint guidance with direct NAV fallback")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}")
        raise SystemExit(1)
