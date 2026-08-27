"""Apply the generated USGS terrain grid to local campus geometry.

Inputs:
  Config/InterVerseCampusTerrain.json
  Config/InterVerseCampusGeometry.local.json
  Config/InterVerseCampusSurfaces.local.json

Behavior:
- Every local coordinate [x_cm, y_cm, z_cm] receives an interpolated terrain Z.
- Building polygons receive properties.terrain_base_z_cm using the average
  terrain elevation of their outer footprint. Buildings remain level while
  following the campus elevation.
- Roads, paths and parking outlines keep per-vertex Z so they follow slopes.

The terrain grid is for visualization/navigation, not engineering use.
"""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TERRAIN_PATH = ROOT / "Config" / "InterVerseCampusTerrain.json"
GEOMETRY_PATH = ROOT / "Config" / "InterVerseCampusGeometry.local.json"
SURFACES_PATH = ROOT / "Config" / "InterVerseCampusSurfaces.local.json"


def load_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def build_samples(terrain: dict):
    grid = terrain["grid"]
    samples = []
    for row in grid["samples"]:
        for sample in row:
            samples.append((
                float(sample["east_m"]) * 100.0,
                float(sample["north_m"]) * 100.0,
                float(sample["relative_z_m"]) * 100.0,
            ))
    return samples


def terrain_z_cm(x_cm: float, y_cm: float, samples) -> float:
    """Inverse-distance interpolation from the four nearest terrain samples."""
    nearest = sorted(
        samples,
        key=lambda s: (s[0] - x_cm) ** 2 + (s[1] - y_cm) ** 2,
    )[:4]
    weighted = 0.0
    total = 0.0
    for sx, sy, sz in nearest:
        d2 = (sx - x_cm) ** 2 + (sy - y_cm) ** 2
        if d2 < 0.0001:
            return sz
        weight = 1.0 / d2
        weighted += sz * weight
        total += weight
    return weighted / total if total else 0.0


def map_point(point, samples):
    if len(point) < 2:
        return point
    x = float(point[0])
    y = float(point[1])
    return [x, y, round(terrain_z_cm(x, y, samples), 2)]


def map_nested(value, depth: int, samples):
    if depth == 0:
        return map_point(value, samples)
    return [map_nested(item, depth - 1, samples) for item in value]


def depth_for_geometry(kind: str) -> int:
    return {
        "Point": 0,
        "LineString": 1,
        "MultiLineString": 2,
        "Polygon": 2,
        "MultiPolygon": 3,
    }[kind]


def outer_z_values(kind: str, coordinates):
    if kind == "Polygon" and coordinates:
        return [float(p[2]) for p in coordinates[0] if len(p) >= 3]
    if kind == "MultiPolygon":
        values = []
        for polygon in coordinates:
            if polygon:
                values.extend(float(p[2]) for p in polygon[0] if len(p) >= 3)
        return values
    return []


def conform_file(path: Path, samples, building_mode: bool):
    if not path.exists():
        print(f"Skipping missing {path}")
        return

    data = load_json(path)
    changed = 0
    for feature in data.get("features", []):
        geometry = feature.get("geometry") or {}
        kind = geometry.get("type")
        coords = geometry.get("coordinates_cm")
        if kind not in {"Point", "LineString", "MultiLineString", "Polygon", "MultiPolygon"} or coords is None:
            continue

        mapped = map_nested(coords, depth_for_geometry(kind), samples)
        geometry["coordinates_cm"] = mapped
        changed += 1

        if building_mode and kind in {"Polygon", "MultiPolygon"}:
            zs = outer_z_values(kind, mapped)
            if zs:
                props = feature.setdefault("properties", {})
                props["terrain_base_z_cm"] = round(sum(zs) / len(zs), 2)
                props["terrain_conform_mode"] = "level_base_average"

    data.setdefault("metadata", {})["terrain_conformed"] = True
    data["metadata"]["terrain_source"] = "Config/InterVerseCampusTerrain.json"
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Terrain-conformed {changed} features -> {path}")


def main():
    if not TERRAIN_PATH.exists():
        raise SystemExit("Terrain config missing. Run python Scripts/build_campus_terrain.py first.")

    terrain = load_json(TERRAIN_PATH)
    samples = build_samples(terrain)
    if not samples:
        raise SystemExit("Terrain grid contains no samples.")

    conform_file(GEOMETRY_PATH, samples, building_mode=True)
    conform_file(SURFACES_PATH, samples, building_mode=False)


if __name__ == "__main__":
    main()
