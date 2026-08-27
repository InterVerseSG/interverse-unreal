"""Build the complete InterVerseSG campus geometry data pipeline.

Run with normal system Python from the repository root:
    python Scripts/build_campus_geometry.py

Prerequisite for terrain conformity:
    python Scripts/build_campus_terrain.py

Steps:
1. Download verified building/campus OSM ways listed in Data/campus_geometry_sources.json.
2. Write Data/campus_geometry.geojson.
3. Convert WGS84 building geometry to local Unreal centimeters.
4. Write Config/InterVerseCampusGeometry.local.json.
5. Query mapped roads, paths, pedestrian ways and parking inside the campus area.
6. Write Data/campus_surfaces.geojson.
7. Convert those circulation/surface features to local Unreal centimeters.
8. Write Config/InterVerseCampusSurfaces.local.json.
9. If terrain data exists, conform building bases and circulation vertices to terrain Z.
"""

from __future__ import annotations

import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCRIPTS = ROOT / "Scripts"
TERRAIN_CONFIG = ROOT / "Config" / "InterVerseCampusTerrain.json"


def run(script_name: str) -> None:
    script = SCRIPTS / script_name
    print(f"\n=== Running {script_name} ===")
    subprocess.run([sys.executable, str(script)], cwd=ROOT, check=True)


def main() -> None:
    run("fetch_osm_footprints.py")
    run("convert_geojson_to_unreal.py")
    run("fetch_osm_campus_surfaces.py")
    run("convert_surfaces_to_unreal.py")

    if TERRAIN_CONFIG.exists():
        run("apply_terrain_to_geometry.py")
        print("Applied USGS terrain Z to buildings and circulation geometry.")
    else:
        print("WARNING: terrain config not found; geometry remains at Z=0.")
        print("Run python Scripts/build_campus_terrain.py, then rerun this pipeline.")

    print("\nInterVerseSG campus geometry pipeline completed successfully.")
    print("Generated building geometry plus mapped campus roads, paths and parking data.")
    print("Next: open Unreal Editor and rerun bootstrap_interverse_level.py.")


if __name__ == "__main__":
    main()
