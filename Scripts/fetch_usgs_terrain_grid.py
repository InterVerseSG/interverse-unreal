"""Sample USGS 3DEP elevations for the InterVerseSG campus.

Uses the USGS Elevation Point Query Service (EPQS):
https://epqs.nationalmap.gov/v1/json

The generated grid is intended for visualization / digital-twin terrain,
not engineering or survey use.

Run from repository root:
    python Scripts/fetch_usgs_terrain_grid.py

Output:
    Data/campus_terrain_grid.json
"""

from __future__ import annotations

import json
import math
import pathlib
import time
import urllib.parse
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parents[1]
ANCHORS_PATH = ROOT / "Config" / "InterVerseCampusAnchors.json"
OUTPUT_PATH = ROOT / "Data" / "campus_terrain_grid.json"

EPQS_URL = "https://epqs.nationalmap.gov/v1/json"
USER_AGENT = "InterVerseSG-terrain/0.1 (https://github.com/InterVerseSG/interverse-unreal)"

GRID_SPACING_M = 30.0
MARGIN_M = 60.0
REQUEST_DELAY_SECONDS = 0.08
MAX_RETRIES = 3
EARTH_RADIUS_M = 6_378_137.0


def load_anchors() -> dict:
    return json.loads(ANCHORS_PATH.read_text(encoding="utf-8"))


def local_m_to_lonlat(east_m: float, north_m: float, origin_lon: float, origin_lat: float) -> tuple[float, float]:
    lat0 = math.radians(origin_lat)
    lon = origin_lon + math.degrees(east_m / (EARTH_RADIUS_M * math.cos(lat0)))
    lat = origin_lat + math.degrees(north_m / EARTH_RADIUS_M)
    return lon, lat


def query_epqs(lon: float, lat: float) -> float:
    params = urllib.parse.urlencode(
        {
            "x": f"{lon:.10f}",
            "y": f"{lat:.10f}",
            "wkid": 4326,
            "units": "Meters",
            "includeDate": "false",
        }
    )
    url = f"{EPQS_URL}?{params}"
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT, "Accept": "application/json"})

    last_error = None
    for attempt in range(MAX_RETRIES):
        try:
            with urllib.request.urlopen(request, timeout=30) as response:
                payload = json.loads(response.read().decode("utf-8"))
            value = float(payload["value"])
            if not math.isfinite(value):
                raise ValueError(f"non-finite elevation {value}")
            return value
        except Exception as exc:
            last_error = exc
            if attempt + 1 < MAX_RETRIES:
                time.sleep(0.5 * (attempt + 1))
    raise RuntimeError(f"EPQS failed for {lat},{lon}: {last_error}")


def axis_values(min_value: float, max_value: float, spacing: float) -> list[float]:
    count = int(math.ceil((max_value - min_value) / spacing))
    return [min_value + index * spacing for index in range(count + 1)]


def main() -> None:
    config = load_anchors()
    anchors = [a for a in config.get("anchors", []) if a.get("coordinate_status") == "verified"]
    if not anchors:
        raise SystemExit("No verified campus anchors found.")

    origin = config["origin"]
    origin_lon = float(origin["longitude"])
    origin_lat = float(origin["latitude"])

    east_values = [float(a["east_m"]) for a in anchors]
    north_values = [float(a["north_m"]) for a in anchors]
    min_east = math.floor((min(east_values) - MARGIN_M) / GRID_SPACING_M) * GRID_SPACING_M
    max_east = math.ceil((max(east_values) + MARGIN_M) / GRID_SPACING_M) * GRID_SPACING_M
    min_north = math.floor((min(north_values) - MARGIN_M) / GRID_SPACING_M) * GRID_SPACING_M
    max_north = math.ceil((max(north_values) + MARGIN_M) / GRID_SPACING_M) * GRID_SPACING_M

    xs = axis_values(min_east, max_east, GRID_SPACING_M)
    ys = axis_values(min_north, max_north, GRID_SPACING_M)

    origin_elevation_m = query_epqs(origin_lon, origin_lat)
    print(f"USGS 3DEP origin elevation: {origin_elevation_m:.3f} m")
    print(f"Sampling {len(xs)} x {len(ys)} = {len(xs) * len(ys)} terrain points...")

    rows = []
    total = len(xs) * len(ys)
    completed = 0
    for north_m in ys:
        row = []
        for east_m in xs:
            lon, lat = local_m_to_lonlat(east_m, north_m, origin_lon, origin_lat)
            elevation_m = query_epqs(lon, lat)
            row.append(
                {
                    "east_m": round(east_m, 3),
                    "north_m": round(north_m, 3),
                    "longitude": round(lon, 10),
                    "latitude": round(lat, 10),
                    "elevation_m": round(elevation_m, 3),
                    "relative_z_m": round(elevation_m - origin_elevation_m, 3),
                }
            )
            completed += 1
            if completed % 25 == 0 or completed == total:
                print(f"  {completed}/{total}")
            time.sleep(REQUEST_DELAY_SECONDS)
        rows.append(row)

    output = {
        "version": "0.1.0",
        "source": "USGS 3DEP Elevation Point Query Service",
        "source_url": EPQS_URL,
        "usage": "visualization_only_not_survey",
        "origin": {
            "poi_id": origin.get("poi_id", "MarquisScienceHall"),
            "longitude": origin_lon,
            "latitude": origin_lat,
            "elevation_m": round(origin_elevation_m, 3),
        },
        "grid": {
            "spacing_m": GRID_SPACING_M,
            "columns": len(xs),
            "rows": len(ys),
            "min_east_m": min_east,
            "max_east_m": max_east,
            "min_north_m": min_north,
            "max_north_m": max_north,
            "samples": rows,
        },
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(json.dumps(output, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Wrote terrain grid to {OUTPUT_PATH}")


if __name__ == "__main__":
    main()
