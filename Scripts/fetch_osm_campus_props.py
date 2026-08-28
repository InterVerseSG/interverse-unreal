"""Fetch mapped campus furniture from OpenStreetMap for InterVerseSG.

No objects are invented. Only mapped OSM nodes are exported:
- amenity=bench
- highway=street_lamp
- information=guidepost|map|board or tourism=information

Objects within PRIORITY_RADIUS_M of EEGEI, CAI, or Centro de Estudiantes are
flagged as priority so Quest can keep them visible farther away.

Output: Config/InterVerseCampusProps.local.json
Coordinate system: local Unreal centimeters, Marquis Science Hall origin.
"""

from __future__ import annotations

import json
import math
import urllib.parse
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ANCHORS_PATH = ROOT / "Config" / "InterVerseCampusAnchors.json"
OUTPUT_PATH = ROOT / "Config" / "InterVerseCampusProps.local.json"
OVERPASS_URL = "https://overpass-api.de/api/interpreter"
EARTH_RADIUS_M = 6_378_137.0
PADDING_DEG = 0.0012
PRIORITY_RADIUS_M = 90.0
PRIORITY_NAVS = {"NAV_EscuelaGraduada", "NAV_CAI", "NAV_CentroEstudiantes"}


def load_anchors():
    data = json.loads(ANCHORS_PATH.read_text(encoding="utf-8"))
    origin = data["origin"]
    anchors = data.get("anchors", [])
    return data, float(origin["longitude"]), float(origin["latitude"]), anchors


def lonlat_to_local_cm(lon: float, lat: float, origin_lon: float, origin_lat: float):
    lat0 = math.radians(origin_lat)
    east_m = EARTH_RADIUS_M * math.cos(lat0) * math.radians(lon - origin_lon)
    north_m = EARTH_RADIUS_M * math.radians(lat - origin_lat)
    return round(east_m * 100.0, 2), round(north_m * 100.0, 2)


def haversine_m(lat1, lon1, lat2, lon2):
    p1, p2 = math.radians(lat1), math.radians(lat2)
    dp = math.radians(lat2 - lat1)
    dl = math.radians(lon2 - lon1)
    a = math.sin(dp / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(dl / 2) ** 2
    return 2 * 6_371_000.0 * math.asin(math.sqrt(a))


def infer_latlon(anchor, origin_lon, origin_lat):
    if anchor.get("latitude") is not None and anchor.get("longitude") is not None:
        return float(anchor["latitude"]), float(anchor["longitude"])
    east = float(anchor.get("east_m", 0.0))
    north = float(anchor.get("north_m", 0.0))
    lat = origin_lat + math.degrees(north / EARTH_RADIUS_M)
    lon = origin_lon + math.degrees(east / (EARTH_RADIUS_M * math.cos(math.radians(origin_lat))))
    return lat, lon


def category_for(tags):
    if tags.get("amenity") == "bench":
        return "bench"
    if tags.get("highway") == "street_lamp":
        return "street_lamp"
    if tags.get("tourism") == "information" or tags.get("information") in {"guidepost", "map", "board"}:
        return "sign"
    return None


def main():
    _, origin_lon, origin_lat, anchors = load_anchors()
    anchor_latlon = [infer_latlon(a, origin_lon, origin_lat) for a in anchors]
    lats = [p[0] for p in anchor_latlon]
    lons = [p[1] for p in anchor_latlon]
    south, north = min(lats) - PADDING_DEG, max(lats) + PADDING_DEG
    west, east = min(lons) - PADDING_DEG, max(lons) + PADDING_DEG

    priority_points = []
    for anchor in anchors:
        if anchor.get("navigation_anchor") in PRIORITY_NAVS:
            lat, lon = infer_latlon(anchor, origin_lon, origin_lat)
            priority_points.append((anchor["navigation_anchor"], lat, lon))

    bbox = f"{south},{west},{north},{east}"
    query = f"""[out:json][timeout:30];(
      node[\"amenity\"=\"bench\"]({bbox});
      node[\"highway\"=\"street_lamp\"]({bbox});
      node[\"tourism\"=\"information\"]({bbox});
      node[\"information\"~\"^(guidepost|map|board)$\"]({bbox});
    );out body;"""
    payload = urllib.parse.urlencode({"data": query}).encode("utf-8")
    req = urllib.request.Request(OVERPASS_URL, data=payload, headers={"User-Agent": "InterVerseSG/0.1"})
    with urllib.request.urlopen(req, timeout=45) as response:
        osm = json.loads(response.read().decode("utf-8"))

    objects = []
    seen = set()
    for el in osm.get("elements", []):
        if el.get("type") != "node" or el.get("id") in seen:
            continue
        tags = el.get("tags") or {}
        category = category_for(tags)
        if not category:
            continue
        seen.add(el["id"])
        lat, lon = float(el["lat"]), float(el["lon"])
        x, y = lonlat_to_local_cm(lon, lat, origin_lon, origin_lat)
        nearest_priority_nav = None
        nearest_priority_m = None
        for nav, plat, plon in priority_points:
            d = haversine_m(lat, lon, plat, plon)
            if nearest_priority_m is None or d < nearest_priority_m:
                nearest_priority_m = d
                nearest_priority_nav = nav
        is_priority = nearest_priority_m is not None and nearest_priority_m <= PRIORITY_RADIUS_M
        objects.append({
            "osm_id": el["id"],
            "category": category,
            "x_cm": x,
            "y_cm": y,
            "z_cm": 0.0,
            "latitude": lat,
            "longitude": lon,
            "priority": is_priority,
            "priority_anchor": nearest_priority_nav if is_priority else None,
            "priority_distance_m": round(nearest_priority_m, 1) if is_priority else None,
            "osm_tags": tags,
        })

    output = {
        "version": "0.1.0",
        "source": "OpenStreetMap mapped campus furniture",
        "origin": {"longitude": origin_lon, "latitude": origin_lat, "poi_id": "MarquisScienceHall"},
        "priority_radius_m": PRIORITY_RADIUS_M,
        "priority_navigation_anchors": sorted(PRIORITY_NAVS),
        "objects": objects,
        "counts": {
            category: sum(1 for x in objects if x["category"] == category)
            for category in ("bench", "street_lamp", "sign")
        },
    }
    OUTPUT_PATH.write_text(json.dumps(output, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Exported {len(objects)} mapped campus objects -> {OUTPUT_PATH}")
    print(output["counts"])


if __name__ == "__main__":
    main()
