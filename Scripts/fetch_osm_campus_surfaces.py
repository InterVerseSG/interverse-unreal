"""Fetch verified campus circulation/surface features from OpenStreetMap.

The script first loads the verified campus boundary way from
Data/campus_geometry_sources.json, derives a bounding box, then queries the
Overpass API for mapped circulation and surface features inside that box.

Output:
    Data/campus_surfaces.geojson

Included OSM features (only when actually mapped):
- highway: footway, path, pedestrian, service, steps, residential, living_street
- amenity=parking
- parking=surface
- area:highway=*

No routes, paths, parking lots, or connections are invented.
Attribution: © OpenStreetMap contributors, ODbL 1.0
"""

from __future__ import annotations

import json
import pathlib
import time
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCES = ROOT / "Data" / "campus_geometry_sources.json"
OUTPUT = ROOT / "Data" / "campus_surfaces.geojson"
OSM_WAY_FULL = "https://api.openstreetmap.org/api/0.6/way/{way_id}/full"
OVERPASS = "https://overpass-api.de/api/interpreter"
USER_AGENT = "InterVerseSG-campus-surfaces/0.1 (https://github.com/InterVerseSG/interverse-unreal)"


def _campus_boundary_way_id() -> int:
    manifest = json.loads(SOURCES.read_text(encoding="utf-8"))
    for item in manifest.get("sources", []):
        if item.get("id") == "CampusMain" and item.get("source") == "OpenStreetMap":
            return int(item["osm_id"])
    raise RuntimeError("CampusMain OSM boundary is missing from campus_geometry_sources.json")


def _request(url: str, data: bytes | None = None, content_type: str | None = None) -> bytes:
    headers = {"User-Agent": USER_AGENT, "Accept": "application/json, application/xml"}
    if content_type:
        headers["Content-Type"] = content_type
    req = urllib.request.Request(url, data=data, headers=headers)
    with urllib.request.urlopen(req, timeout=60) as response:
        return response.read()


def _campus_bbox(way_id: int, margin_deg: float = 0.0010) -> tuple[float, float, float, float]:
    xml = _request(OSM_WAY_FULL.format(way_id=way_id))
    root = ET.fromstring(xml)
    nodes = [(float(n.attrib["lat"]), float(n.attrib["lon"])) for n in root.findall("node")]
    if not nodes:
        raise RuntimeError(f"No nodes returned for campus boundary way {way_id}")
    lats = [p[0] for p in nodes]
    lons = [p[1] for p in nodes]
    return min(lats)-margin_deg, min(lons)-margin_deg, max(lats)+margin_deg, max(lons)+margin_deg


def _overpass_query(bbox: tuple[float, float, float, float]) -> dict:
    south, west, north, east = bbox
    box = f"{south},{west},{north},{east}"
    query = f"""
[out:json][timeout:40];
(
  way[highway~"^(footway|path|pedestrian|service|steps|residential|living_street)$"]({box});
  way[amenity=parking]({box});
  way[parking=surface]({box});
  way["area:highway"]({box});
);
out body geom;
""".strip()
    payload = urllib.parse.urlencode({"data": query}).encode("utf-8")
    return json.loads(_request(OVERPASS, data=payload, content_type="application/x-www-form-urlencoded").decode("utf-8"))


def _feature(element: dict) -> dict | None:
    geometry = element.get("geometry") or []
    coords = [(float(p["lon"]), float(p["lat"])) for p in geometry if "lon" in p and "lat" in p]
    if len(coords) < 2:
        return None

    tags = element.get("tags") or {}
    is_area = (
        len(coords) >= 4
        and coords[0] == coords[-1]
        and (tags.get("amenity") == "parking" or tags.get("parking") == "surface" or "area:highway" in tags)
    )
    geometry_type = "Polygon" if is_area else "LineString"
    geometry_coords = [coords] if is_area else coords

    category = "circulation"
    if tags.get("amenity") == "parking" or tags.get("parking") == "surface":
        category = "parking"
    elif tags.get("highway") in {"footway", "path", "steps", "pedestrian"}:
        category = "pedestrian"
    elif tags.get("highway") in {"service", "residential", "living_street"}:
        category = "road"

    return {
        "type": "Feature",
        "properties": {
            "id": f"osm_way_{element['id']}",
            "display_name": tags.get("name") or tags.get("ref") or f"OSM way {element['id']}",
            "category": category,
            "source": "OpenStreetMap",
            "osm_type": "way",
            "osm_id": int(element["id"]),
            "osm_tags": tags,
            "geometry_status": "verified_from_overpass",
        },
        "geometry": {"type": geometry_type, "coordinates": geometry_coords},
    }


def main() -> None:
    boundary_id = _campus_boundary_way_id()
    bbox = _campus_bbox(boundary_id)
    time.sleep(1.0)
    data = _overpass_query(bbox)

    features = []
    for element in data.get("elements", []):
        if element.get("type") != "way":
            continue
        f = _feature(element)
        if f:
            features.append(f)

    result = {
        "type": "FeatureCollection",
        "name": "InterVerseSG campus circulation and surfaces",
        "metadata": {
            "version": "0.1.0",
            "campus_boundary_osm_way": boundary_id,
            "query_bbox": bbox,
            "attribution": "© OpenStreetMap contributors, ODbL 1.0",
            "policy": "Only mapped OpenStreetMap geometry is included; no inferred connections.",
        },
        "features": features,
    }
    OUTPUT.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(features)} mapped campus circulation/surface features -> {OUTPUT}")


if __name__ == "__main__":
    main()
