"""Convert campus GeoJSON (WGS84 lon/lat) to local Unreal coordinates.

This is a standard Python script and does NOT require Unreal Engine.

Input:
    Data/campus_geometry.geojson
Output:
    Config/InterVerseCampusGeometry.local.json

Supported geometries:
- Point
- LineString
- MultiLineString
- Polygon
- MultiPolygon

Coordinate convention:
- Origin: Marquis Science Hall by default
- +X = east
- +Y = north
- 1 meter = 100 Unreal units (centimeters)

No geometry is invented. The script only transforms coordinates that exist in
the source GeoJSON.
"""

from __future__ import annotations

import json
import math
from pathlib import Path

PROJECT_DIR = Path(__file__).resolve().parents[1]
INPUT_PATH = PROJECT_DIR / "Data" / "campus_geometry.geojson"
OUTPUT_PATH = PROJECT_DIR / "Config" / "InterVerseCampusGeometry.local.json"
ANCHORS_PATH = PROJECT_DIR / "Config" / "InterVerseCampusAnchors.json"

EARTH_RADIUS_M = 6_378_137.0


def load_origin() -> tuple[float, float]:
    data = json.loads(ANCHORS_PATH.read_text(encoding="utf-8"))
    origin = data["origin"]
    return float(origin["longitude"]), float(origin["latitude"])


def lonlat_to_local_cm(lon: float, lat: float, origin_lon: float, origin_lat: float) -> list[float]:
    lat0 = math.radians(origin_lat)
    dlon = math.radians(lon - origin_lon)
    dlat = math.radians(lat - origin_lat)
    east_m = EARTH_RADIUS_M * math.cos(lat0) * dlon
    north_m = EARTH_RADIUS_M * dlat
    return [round(east_m * 100.0, 2), round(north_m * 100.0, 2), 0.0]


def convert_coordinates(coords, depth: int, origin_lon: float, origin_lat: float):
    if depth == 0:
        lon, lat = coords[0], coords[1]
        return lonlat_to_local_cm(float(lon), float(lat), origin_lon, origin_lat)
    return [convert_coordinates(item, depth - 1, origin_lon, origin_lat) for item in coords]


def geometry_depth(geometry_type: str) -> int:
    return {
        "Point": 0,
        "LineString": 1,
        "MultiLineString": 2,
        "Polygon": 2,
        "MultiPolygon": 3,
    }[geometry_type]


def main() -> None:
    if not INPUT_PATH.exists():
        raise SystemExit(
            f"Missing {INPUT_PATH}. Add a GeoJSON export before running this converter."
        )

    source = json.loads(INPUT_PATH.read_text(encoding="utf-8"))
    origin_lon, origin_lat = load_origin()

    converted_features = []
    skipped = []

    for feature in source.get("features", []):
        geometry = feature.get("geometry")
        props = feature.get("properties") or {}
        if not geometry:
            skipped.append(props.get("id") or props.get("name") or "unnamed")
            continue

        geometry_type = geometry.get("type")
        if geometry_type not in {"Point", "LineString", "MultiLineString", "Polygon", "MultiPolygon"}:
            skipped.append(props.get("id") or props.get("name") or geometry_type or "unknown")
            continue

        converted_features.append(
            {
                "type": "Feature",
                "properties": props,
                "geometry": {
                    "type": geometry_type,
                    "coordinates_cm": convert_coordinates(
                        geometry["coordinates"],
                        geometry_depth(geometry_type),
                        origin_lon,
                        origin_lat,
                    ),
                },
            }
        )

    output = {
        "version": "0.1.0",
        "source": str(INPUT_PATH.relative_to(PROJECT_DIR)).replace("\\", "/"),
        "coordinate_system": "local Unreal centimeters",
        "origin": {
            "longitude": origin_lon,
            "latitude": origin_lat,
            "poi_id": "MarquisScienceHall",
        },
        "axis_mapping": {"east": "+X", "north": "+Y", "up": "+Z"},
        "features": converted_features,
        "skipped_features": skipped,
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(json.dumps(output, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Converted {len(converted_features)} features -> {OUTPUT_PATH}")
    if skipped:
        print(f"Skipped {len(skipped)} features without supported geometry.")


if __name__ == "__main__":
    main()
