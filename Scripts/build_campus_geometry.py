"""Build the complete InterVerseSG campus geometry data pipeline.

Run with normal system Python from the repository root:
    python Scripts/build_campus_geometry.py

Prerequisite for terrain conformity:
    python Scripts/build_campus_terrain.py

Builds verified building geometry, circulation surfaces and mapped green areas.
No missing geometry is invented.
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
    run("fetch_osm_green_areas.py")
    run("convert_green_areas_to_unreal.py")

    if TERRAIN_CONFIG.exists():
        run("apply_terrain_to_geometry.py")
        print("Applied USGS terrain Z to buildings and circulation geometry.")
    else:
        print("WARNING: terrain config not found; geometry remains at Z=0.")
        print("Run python Scripts/build_campus_terrain.py, then rerun this pipeline.")

    print("\nInterVerseSG campus geometry pipeline completed successfully.")
    print("Generated buildings, mapped roads/paths/parking, and mapped green areas.")
    print("Next: open Unreal Editor and rerun bootstrap_interverse_level.py.")


if __name__ == "__main__":
    main()
