"""Convert campus_surfaces.geojson from WGS84 to local Unreal centimeters.

Input:
    Data/campus_surfaces.geojson
Output:
    Config/InterVerseCampusSurfaces.local.json

Uses the same Marquis Science Hall origin as InterVerseCampusAnchors.json.
"""

from __future__ import annotations

import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INPUT = ROOT / "Data" / "campus_surfaces.geojson"
OUTPUT = ROOT / "Config" / "InterVerseCampusSurfaces.local.json"
ANCHORS = ROOT / "Config" / "InterVerseCampusAnchors.json"
EARTH_RADIUS_M = 6_378_137.0


def _origin() -> tuple[float, float]:
    data = json.loads(ANCHORS.read_text(encoding="utf-8"))
    origin = data["origin"]
    return float(origin["longitude"]), float(origin["latitude"])


def _point(lon: float, lat: float, origin_lon: float, origin_lat: float) -> list[float]:
    lat0 = math.radians(origin_lat)
    east_m = EARTH_RADIUS_M * math.cos(lat0) * math.radians(lon - origin_lon)
    north_m = EARTH_RADIUS_M * math.radians(lat - origin_lat)
    return [round(east_m * 100.0, 2), round(north_m * 100.0, 2), 0.0]


def _convert(coords, depth: int, origin_lon: float, origin_lat: float):
    if depth == 0:
        return _point(float(coords[0]), float(coords[1]), origin_lon, origin_lat)
    return [_convert(item, depth - 1, origin_lon, origin_lat) for item in coords]


def main() -> None:
    if not INPUT.exists():
        raise SystemExit(f"Missing {INPUT}. Run fetch_osm_campus_surfaces.py first.")
    source = json.loads(INPUT.read_text(encoding="utf-8"))
    origin_lon, origin_lat = _origin()

    out_features = []
    for feature in source.get("features", []):
        geometry = feature.get("geometry") or {}
        geometry_type = geometry.get("type")
        if geometry_type not in {"LineString", "Polygon"}:
            continue
        depth = 1 if geometry_type == "LineString" else 2
        out_features.append({
            "type": "Feature",
            "properties": feature.get("properties") or {},
            "geometry": {
                "type": geometry_type,
                "coordinates_cm": _convert(geometry["coordinates"], depth, origin_lon, origin_lat),
            },
        })

    result = {
        "version": "0.1.0",
        "source": "Data/campus_surfaces.geojson",
        "coordinate_system": "local Unreal centimeters",
        "origin": {"longitude": origin_lon, "latitude": origin_lat, "poi_id": "MarquisScienceHall"},
        "features": out_features,
    }
    OUTPUT.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Converted {len(out_features)} campus surface features -> {OUTPUT}")


if __name__ == "__main__":
    main()
