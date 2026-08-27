"""Apply sampled terrain elevation to verified InterVerse navigation anchors.

Input:
    Data/campus_terrain_grid.json
    Config/InterVerseCampusAnchors.json

Output:
    Config/InterVerseCampusAnchors.json (adds/updates unreal_z_cm and terrain_elevation_m)

The grid stores elevation relative to the Marquis Science Hall terrain datum,
so Unreal Z=0 remains the terrain elevation at that origin.
"""

from __future__ import annotations

import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
TERRAIN_PATH = ROOT / "Data" / "campus_terrain_grid.json"
ANCHORS_PATH = ROOT / "Config" / "InterVerseCampusAnchors.json"


def clamp(value: float, low: float, high: float) -> float:
    return max(low, min(high, value))


def sample_bilinear(grid: dict, east_m: float, north_m: float) -> tuple[float, float]:
    meta = grid["grid"]
    samples = meta["samples"]
    spacing = float(meta["spacing_m"])
    min_east = float(meta["min_east_m"])
    min_north = float(meta["min_north_m"])
    cols = int(meta["columns"])
    rows = int(meta["rows"])

    fx = (east_m - min_east) / spacing
    fy = (north_m - min_north) / spacing
    x0 = int(clamp(int(fx), 0, cols - 1))
    y0 = int(clamp(int(fy), 0, rows - 1))
    x1 = min(x0 + 1, cols - 1)
    y1 = min(y0 + 1, rows - 1)
    tx = clamp(fx - x0, 0.0, 1.0)
    ty = clamp(fy - y0, 0.0, 1.0)

    def val(x: int, y: int, key: str) -> float:
        return float(samples[y][x][key])

    def interp(key: str) -> float:
        a = val(x0, y0, key) * (1.0 - tx) + val(x1, y0, key) * tx
        b = val(x0, y1, key) * (1.0 - tx) + val(x1, y1, key) * tx
        return a * (1.0 - ty) + b * ty

    return interp("relative_z_m"), interp("elevation_m")


def main() -> None:
    if not TERRAIN_PATH.exists():
        raise SystemExit(f"Missing terrain grid: {TERRAIN_PATH}")

    terrain = json.loads(TERRAIN_PATH.read_text(encoding="utf-8"))
    anchors = json.loads(ANCHORS_PATH.read_text(encoding="utf-8"))

    updated = 0
    for entry in anchors.get("anchors", []):
        if entry.get("coordinate_status") != "verified":
            continue
        east_m = float(entry["east_m"])
        north_m = float(entry["north_m"])
        relative_z_m, elevation_m = sample_bilinear(terrain, east_m, north_m)
        entry["terrain_elevation_m"] = round(elevation_m, 3)
        entry["unreal_z_cm"] = round(relative_z_m * 100.0, 2)
        entry["z_source"] = "USGS_3DEP_EPQS_interpolated"
        updated += 1

    anchors["terrain"] = {
        "source": terrain.get("source"),
        "origin_elevation_m": terrain["origin"]["elevation_m"],
        "grid_spacing_m": terrain["grid"]["spacing_m"],
        "vertical_reference": "relative_to_origin_terrain_elevation",
    }
    ANCHORS_PATH.write_text(json.dumps(anchors, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Applied terrain Z to {updated} verified navigation anchors.")


if __name__ == "__main__":
    main()
