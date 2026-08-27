"""Fetch verified OpenStreetMap way geometry and build campus_geometry.geojson.

This script is intentionally separate from Unreal Editor scripting so it can be
run with the system Python that ships with Windows/macOS/Linux. It reads
Data/campus_geometry_sources.json, downloads only sources explicitly marked as
OpenStreetMap ways, reconstructs their node order, and writes a GeoJSON file.

Usage from the repository root:
    python Scripts/fetch_osm_footprints.py

OpenStreetMap attribution must be preserved in downstream products:
    © OpenStreetMap contributors, ODbL 1.0
"""

from __future__ import annotations

import json
import pathlib
import time
import urllib.error
import urllib.request
import xml.etree.ElementTree as ET

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCES_PATH = ROOT / "Data" / "campus_geometry_sources.json"
OUTPUT_PATH = ROOT / "Data" / "campus_geometry.geojson"

OSM_API = "https://api.openstreetmap.org/api/0.6/way/{way_id}/full"
USER_AGENT = "InterVerseSG-campus-geometry/0.1 (https://github.com/InterVerseSG/interverse-unreal)"
REQUEST_DELAY_SECONDS = 1.0


def _load_sources() -> dict:
    with SOURCES_PATH.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def _fetch_way_xml(way_id: int) -> bytes:
    request = urllib.request.Request(
        OSM_API.format(way_id=way_id),
        headers={"User-Agent": USER_AGENT, "Accept": "application/xml"},
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        return response.read()


def _way_to_feature(source: dict, xml_bytes: bytes) -> dict:
    root = ET.fromstring(xml_bytes)
    nodes: dict[str, tuple[float, float]] = {}
    for node in root.findall("node"):
        nodes[node.attrib["id"]] = (
            float(node.attrib["lon"]),
            float(node.attrib["lat"]),
        )

    target_way = None
    for way in root.findall("way"):
        if int(way.attrib["id"]) == int(source["osm_id"]):
            target_way = way
            break
    if target_way is None:
        raise ValueError(f"OSM way {source['osm_id']} not found in API response")

    refs = [nd.attrib["ref"] for nd in target_way.findall("nd")]
    coordinates = [nodes[ref] for ref in refs if ref in nodes]
    if len(coordinates) < 2:
        raise ValueError(f"OSM way {source['osm_id']} has insufficient geometry")

    tags = {tag.attrib["k"]: tag.attrib["v"] for tag in target_way.findall("tag")}
    is_closed = coordinates[0] == coordinates[-1]
    geometry_type = "Polygon" if is_closed and len(coordinates) >= 4 else "LineString"
    geometry_coordinates = [coordinates] if geometry_type == "Polygon" else coordinates

    return {
        "type": "Feature",
        "properties": {
            "id": source["id"],
            "display_name": source.get("name", source["id"]),
            "source": "OpenStreetMap",
            "osm_type": "way",
            "osm_id": int(source["osm_id"]),
            "geometry_status": "verified_from_osm_api",
            "osm_tags": tags,
        },
        "geometry": {
            "type": geometry_type,
            "coordinates": geometry_coordinates,
        },
    }


def _point_feature(source: dict) -> dict:
    return {
        "type": "Feature",
        "properties": {
            "id": source["id"],
            "display_name": source.get("name", source["id"]),
            "source": source.get("source", "project_owner"),
            "geometry_status": source.get("geometry_status", "verified_point_only"),
            "navigation_anchor": source.get("navigation_anchor"),
        },
        "geometry": {
            "type": "Point",
            "coordinates": [float(source["longitude"]), float(source["latitude"])],
        },
    }


def build_geojson() -> dict:
    manifest = _load_sources()
    features: list[dict] = []
    failures: list[dict] = []

    for source in manifest.get("sources", []):
        if source.get("source") == "OpenStreetMap" and source.get("osm_type") == "way":
            way_id = int(source["osm_id"])
            print(f"Fetching OSM way {way_id}: {source.get('name', source['id'])}")
            try:
                feature = _way_to_feature(source, _fetch_way_xml(way_id))
                features.append(feature)
            except (urllib.error.URLError, urllib.error.HTTPError, TimeoutError, ValueError) as exc:
                failures.append({"id": source["id"], "osm_id": way_id, "error": str(exc)})
                print(f"  WARNING: {exc}")
            time.sleep(REQUEST_DELAY_SECONDS)
        elif "latitude" in source and "longitude" in source:
            features.append(_point_feature(source))

    return {
        "type": "FeatureCollection",
        "name": "InterVerseSG campus geometry",
        "metadata": {
            "version": "0.1.0",
            "generated_from": "Data/campus_geometry_sources.json",
            "attribution": "© OpenStreetMap contributors, ODbL 1.0",
            "canonical_visual_reference": manifest.get("canonical_visual_reference"),
            "failed_sources": failures,
        },
        "features": features,
    }


def main() -> None:
    data = build_geojson()
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT_PATH.open("w", encoding="utf-8") as handle:
        json.dump(data, handle, ensure_ascii=False, indent=2)
        handle.write("\n")

    print(f"Wrote {len(data['features'])} features to {OUTPUT_PATH}")
    failures = data["metadata"]["failed_sources"]
    if failures:
        print(f"Completed with {len(failures)} source failures; inspect GeoJSON metadata.")


if __name__ == "__main__":
    main()
