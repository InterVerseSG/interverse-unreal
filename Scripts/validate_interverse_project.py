"""Cloud-safe validation for InterVerseSG.

This script intentionally does not import Unreal Engine. It validates data and
configuration that can be checked in GitHub Actions or any normal Python 3
runtime.
"""

from __future__ import annotations

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

ANCHORS = ROOT / "Config" / "InterVerseCampusAnchors.json"
ENGINE = ROOT / "Config" / "DefaultEngine.ini"
UPROJECT = ROOT / "InterVerseSG.uproject"

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


def validate_uproject() -> None:
    data = load_json(UPROJECT)
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


def validate_generated_json_if_present() -> None:
    optional_json = [
        ROOT / "Config" / "InterVerseCampusGeometry.local.json",
        ROOT / "Config" / "InterVerseCampusSurfaces.local.json",
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
        ("engine config", validate_engine_config),
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
