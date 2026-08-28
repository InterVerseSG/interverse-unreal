"""Cloud-safe validation for InterVerseSG.

This script intentionally does not import Unreal Engine. It validates data,
Quest configuration, cloud endpoints and source-level performance architecture
that can be checked in GitHub Actions or any normal Python 3 runtime.
"""

from __future__ import annotations

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

ANCHORS = ROOT / "Config" / "InterVerseCampusAnchors.json"
SECTORS = ROOT / "Config" / "InterVerseCampusSectors.json"
ENGINE = ROOT / "Config" / "DefaultEngine.ini"
GAME = ROOT / "Config" / "DefaultGame.ini"
SCALABILITY = ROOT / "Config" / "DefaultScalability.ini"
UPROJECT = ROOT / "InterVerseSG.uproject"
BUILDINGS_H = ROOT / "Source" / "InterVerseRuntime" / "Public" / "InterVerseBuildingExtrusionActor.h"
BUILDINGS_CPP = ROOT / "Source" / "InterVerseRuntime" / "Private" / "InterVerseBuildingExtrusionActor.cpp"
FOLIAGE_H = ROOT / "Source" / "InterVerseRuntime" / "Public" / "InterVerseFoliageActor.h"
FOLIAGE_CPP = ROOT / "Source" / "InterVerseRuntime" / "Private" / "InterVerseFoliageActor.cpp"
FOLIAGE_FETCHER = ROOT / "Scripts" / "fetch_osm_foliage.py"
BOOTSTRAP = ROOT / "Scripts" / "bootstrap_interverse_level.py"

EXPECTED_API = "https://interverse-api-yhqx.onrender.com"
EXPECTED_BUILDER = "https://interverse-builder.onrender.com"


def fail(message: str) -> None:
    raise AssertionError(message)


def load_json(path: pathlib.Path):
    if not path.exists():
        fail(f"Missing required file: {path.relative_to(ROOT)}")
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        fail(f"Invalid JSON in {path.relative_to(ROOT)}: {exc}")


def require_text(text: str, token: str, source: str) -> None:
    if token not in text:
        fail(f"Required Quest setting missing from {source}: {token}")


def validate_uproject() -> None:
    data = load_json(UPROJECT)
    if data.get("EngineAssociation") != "5.8":
        fail("InterVerseSG must remain associated with Unreal Engine 5.8 until the Quest build is validated")

    modules = {m.get("Name") for m in data.get("Modules", [])}
    if "InterVerseRuntime" not in modules:
        fail("InterVerseRuntime module missing from .uproject")

    enabled_plugins = {p.get("Name") for p in data.get("Plugins", []) if p.get("Enabled")}
    for plugin in ("OpenXR", "ProceduralMeshComponent", "EnhancedInput"):
        if plugin not in enabled_plugins:
            fail(f"Required plugin not enabled: {plugin}")


def validate_anchors() -> None:
    data = load_json(ANCHORS)
    anchors = data.get("anchors", [])
    if not anchors:
        fail("No campus anchors found")

    ids: set[str] = set()
    navs: set[str] = set()
    for entry in anchors:
        poi_id = entry.get("id")
        nav = entry.get("navigation_anchor")
        if not poi_id or not nav:
            fail(f"Anchor missing id/navigation_anchor: {entry}")
        if poi_id in ids:
            fail(f"Duplicate POI id: {poi_id}")
        if nav in navs:
            fail(f"Duplicate navigation anchor: {nav}")
        ids.add(poi_id)
        navs.add(nav)

        if not nav.startswith("NAV_"):
            fail(f"Navigation anchor must start with NAV_: {nav}")
        if entry.get("coordinate_status") == "verified":
            for key in ("unreal_x_cm", "unreal_y_cm"):
                if entry.get(key) is None:
                    fail(f"Verified anchor {nav} missing {key}")

    required = {
        "NAV_MarquisScienceHall",
        "NAV_CAI",
        "NAV_CentroEstudiantes",
        "NAV_EscuelaGraduada",
    }
    missing = sorted(required - navs)
    if missing:
        fail("Missing priority NAV anchors: " + ", ".join(missing))

    eegei = next((a for a in anchors if a.get("navigation_anchor") == "NAV_EscuelaGraduada"), None)
    if not eegei:
        fail("EEGEI anchor missing")
    if abs(float(eegei.get("latitude", 0)) - 18.08518322539467) > 1e-9:
        fail("EEGEI latitude differs from project-owner verified coordinate")
    if abs(float(eegei.get("longitude", 0)) - (-67.05382825210272)) > 1e-9:
        fail("EEGEI longitude differs from project-owner verified coordinate")


def validate_sectors() -> None:
    sector_data = load_json(SECTORS)
    sectors = sector_data.get("sectors", [])
    if len(sectors) != 4:
        fail(f"Quest campus profile currently expects 4 sectors, found {len(sectors)}")

    anchor_data = load_json(ANCHORS)
    navs = {a.get("navigation_anchor") for a in anchor_data.get("anchors", [])}
    ids: set[str] = set()
    covered_priority_navs: set[str] = set()

    for sector in sectors:
        sector_id = sector.get("id")
        if not sector_id or not sector_id.startswith("SECTOR_"):
            fail(f"Invalid sector id: {sector_id}")
        if sector_id in ids:
            fail(f"Duplicate sector id: {sector_id}")
        ids.add(sector_id)

        for key in ("center_x_cm", "center_y_cm", "radius_cm"):
            if not isinstance(sector.get(key), (int, float)):
                fail(f"Sector {sector_id} missing numeric {key}")
        if float(sector["radius_cm"]) < 10000:
            fail(f"Sector {sector_id} radius is too small for safe Quest preload")

        for nav in sector.get("priority_anchors", []):
            if nav not in navs:
                fail(f"Sector {sector_id} references unknown navigation anchor: {nav}")
            covered_priority_navs.add(nav)

    for nav in ("NAV_MarquisScienceHall", "NAV_CAI", "NAV_CentroEstudiantes", "NAV_EscuelaGraduada"):
        if nav not in covered_priority_navs:
            fail(f"Priority navigation anchor is not assigned to a Quest sector: {nav}")

    runtime = sector_data.get("runtime") or {}
    if float(runtime.get("update_interval_seconds", 0)) < 0.1:
        fail("Sector update interval must not run at per-frame frequency")
    if float(runtime.get("active_radius_cm", 0)) <= 0 or float(runtime.get("preload_radius_cm", 0)) <= 0:
        fail("Sector runtime radii must be positive")


def validate_engine_config() -> None:
    if not ENGINE.exists():
        fail("Config/DefaultEngine.ini missing")
    text = ENGINE.read_text(encoding="utf-8")

    for value in (EXPECTED_API, EXPECTED_BUILDER):
        if value not in text:
            fail(f"Expected Render service URL missing from DefaultEngine.ini: {value}")

    target_match = re.search(r"^TargetSDKVersion=(\d+)$", text, flags=re.MULTILINE)
    if not target_match:
        fail("TargetSDKVersion missing")
    if int(target_match.group(1)) < 35:
        fail("Android TargetSDKVersion must be at least 35 for current Quest pipeline")

    quest_settings = (
        "bBuildForArm64=True",
        "bBuildForX8664=False",
        "bSupportsVulkan=True",
        "bSupportsVulkanSM5=False",
        "bPackageForMetaQuest=True",
        "r.Mobile.ShadingPath=0",
        "r.MobileHDR=False",
        "r.ForwardShading=True",
        "r.DefaultFeature.AntiAliasing=3",
        "r.MSAACount=4",
        "vr.InstancedStereo=True",
        "vr.MobileMultiView=True",
        "r.DefaultFeature.MotionBlur=False",
        "r.DefaultFeature.Bloom=False",
        "r.DefaultFeature.AmbientOcclusion=False",
    )
    for token in quest_settings:
        require_text(text, token, "DefaultEngine.ini")


def validate_quest_profile() -> None:
    if not GAME.exists():
        fail("Config/DefaultGame.ini missing")
    game = GAME.read_text(encoding="utf-8")
    require_text(game, "bStartInVR=True", "DefaultGame.ini")
    require_text(game, "bCompressed=True", "DefaultGame.ini")
    require_text(game, "bUseIoStore=True", "DefaultGame.ini")

    if not SCALABILITY.exists():
        fail("Config/DefaultScalability.ini missing")
    scalability = SCALABILITY.read_text(encoding="utf-8")
    for token in ("[ViewDistanceQuality@1]", "[ShadowQuality@1]", "[EffectsQuality@1]", "[FoliageQuality@1]"):
        require_text(scalability, token, "DefaultScalability.ini")


def validate_quest_scene_architecture() -> None:
    for path in (BUILDINGS_H, BUILDINGS_CPP, FOLIAGE_H, FOLIAGE_CPP, FOLIAGE_FETCHER, BOOTSTRAP):
        if not path.exists():
            fail(f"Missing Quest optimization source: {path.relative_to(ROOT)}")

    buildings_h = BUILDINGS_H.read_text(encoding="utf-8")
    buildings = BUILDINGS_CPP.read_text(encoding="utf-8")
    for token in ("bEnableRuntimeSectorCulling", "ActiveSectorRadiusCm", "SectorUpdateIntervalSeconds", "UpdateSectorVisibility"):
        require_text(buildings_h, token, "InterVerseBuildingExtrusionActor.h")
    for token in ("LoadSectorDefinitions", "FindNearestSector", "SetMeshSectionVisible", "SetTimer", "BuiltSectorCentersCm"):
        require_text(buildings, token, "InterVerseBuildingExtrusionActor.cpp")
    if "CreateMeshSection_LinearColor(SectionIndex" not in buildings:
        fail("Quest building sector batching is missing; expected one mesh section per active campus sector")

    foliage_h = FOLIAGE_H.read_text(encoding="utf-8")
    foliage_cpp = FOLIAGE_CPP.read_text(encoding="utf-8")
    for token in ("UHierarchicalInstancedStaticMeshComponent", "StartCullDistanceCm", "EndCullDistanceCm"):
        require_text(foliage_h, token, "InterVerseFoliageActor.h")
    for token in ("SetCullDistances", "SetCollisionEnabled(ECollisionEnabled::NoCollision)", "AddInstance"):
        require_text(foliage_cpp, token, "InterVerseFoliageActor.cpp")

    fetcher = FOLIAGE_FETCHER.read_text(encoding="utf-8")
    require_text(fetcher, 'node["natural"="tree"]', "fetch_osm_foliage.py")
    require_text(fetcher, '"source": "OpenStreetMap natural=tree"', "fetch_osm_foliage.py")

    bootstrap = BOOTSTRAP.read_text(encoding="utf-8")
    require_text(bootstrap, "_ensure_foliage_actor", "bootstrap_interverse_level.py")
    require_text(bootstrap, "IV_CampusFoliage", "bootstrap_interverse_level.py")


def validate_generated_json_if_present() -> None:
    optional_json = [
        ROOT / "Config" / "InterVerseCampusGeometry.local.json",
        ROOT / "Config" / "InterVerseCampusSurfaces.local.json",
        ROOT / "Config" / "InterVerseFoliage.local.json",
        ROOT / "Data" / "campus_terrain_grid.json",
        ROOT / "Data" / "campus_geometry.geojson",
        ROOT / "Data" / "campus_surfaces.geojson",
    ]
    for path in optional_json:
        if path.exists():
            load_json(path)


def main() -> int:
    checks = [
        ("uproject", validate_uproject),
        ("anchors", validate_anchors),
        ("Quest sectors", validate_sectors),
        ("engine config", validate_engine_config),
        ("Quest profile", validate_quest_profile),
        ("Quest scene architecture", validate_quest_scene_architecture),
        ("generated JSON", validate_generated_json_if_present),
    ]
    for name, check in checks:
        check()
        print(f"PASS: {name}")
    print("InterVerseSG cloud-safe validation passed.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
