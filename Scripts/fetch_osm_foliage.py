"""Fetch mapped campus trees from OpenStreetMap and convert them to Unreal local cm.

This script does not invent vegetation. It requests only OSM nodes tagged
natural=tree inside a bounding box derived from verified campus anchors.

Output:
    Config/InterVerseFoliage.local.json

Coordinate convention matches convert_geojson_to_unreal.py:
- origin = anchors["origin"] (Marquis Science Hall)
- +X east, +Y north, +Z up
- 1 meter = 100 cm
"""

from __future__ import annotations

import json
import math
import random
import urllib.parse
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ANCHORS = ROOT / "Config" / "InterVerseCampusAnchors.json"
OUTPUT = ROOT / "Config" / "InterVerseFoliage.local.json"
OVERPASS = "https://overpass-api.de/api/interpreter"
EARTH_RADIUS_M = 6_378_137.0
MARGIN_DEG = 0.0012


def local_cm(lon: float, lat: float, origin_lon: float, origin_lat: float) -> tuple[float, float]:
    lat0 = math.radians(origin_lat)
    x = EARTH_RADIUS_M * math.cos(lat0) * math.radians(lon - origin_lon) * 100.0
    y = EARTH_RADIUS_M * math.radians(lat - origin_lat) * 100.0
    return round(x, 2), round(y, 2)


def main() -> None:
    data = json.loads(ANCHORS.read_text(encoding="utf-8"))
    origin = data["origin"]
    origin_lon = float(origin["longitude"])
    origin_lat = float(origin["latitude"])

    anchors = [a for a in data.get("anchors", []) if a.get("latitude") is not None and a.get("longitude") is not None]
    if not anchors:
        raise SystemExit("No georeferenced anchors available.")

    south = min(float(a["latitude"]) for a in anchors) - MARGIN_DEG
    north = max(float(a["latitude"]) for a in anchors) + MARGIN_DEG
    west = min(float(a["longitude"]) for a in anchors) - MARGIN_DEG
    east = max(float(a["longitude"]) for a in anchors) + MARGIN_DEG

    query = f'[out:json][timeout:30];node["natural"="tree"]({south},{west},{north},{east});out body;'
    body = urllib.parse.urlencode({"data": query}).encode("utf-8")
    request = urllib.request.Request(OVERPASS, data=body, headers={"User-Agent": "InterVerseSG/0.1 educational digital twin"})

    with urllib.request.urlopen(request, timeout=45) as response:
        osm = json.load(response)

    instances = []
    for node in osm.get("elements", []):
        if node.get("type") != "node" or "lat" not in node or "lon" not in node:
            continue
        node_id = int(node["id"])
        x, y = local_cm(float(node["lon"]), float(node["lat"]), origin_lon, origin_lat)
        rng = random.Random(node_id)
        instances.append({
            "osm_id": node_id,
            "x_cm": x,
            "y_cm": y,
            "z_cm": 0.0,
            "yaw_deg": round(rng.uniform(0.0, 360.0), 1),
            "scale": round(rng.uniform(0.85, 1.15), 2),
            "source": "OpenStreetMap natural=tree",
        })

    payload = {
        "version": "0.1.0",
        "source": "OpenStreetMap Overpass natural=tree",
        "origin": {"longitude": origin_lon, "latitude": origin_lat},
        "bounds": {"south": south, "west": west, "north": north, "east": east},
        "instance_count": len(instances),
        "instances": instances,
    }
    OUTPUT.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Wrote {len(instances)} mapped tree instances -> {OUTPUT}")


if __name__ == "__main__":
    main()
