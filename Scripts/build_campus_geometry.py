"""Build the complete InterVerseSG campus geometry data pipeline.

Run with normal system Python from the repository root:
    python Scripts/build_campus_geometry.py

Steps:
1. Download verified OSM ways listed in Data/campus_geometry_sources.json.
2. Write Data/campus_geometry.geojson.
3. Convert WGS84 lon/lat to local Unreal centimeters.
4. Write Config/InterVerseCampusGeometry.local.json.
"""

from __future__ import annotations

import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCRIPTS = ROOT / "Scripts"


def run(script_name: str) -> None:
    script = SCRIPTS / script_name
    print(f"\n=== Running {script_name} ===")
    subprocess.run([sys.executable, str(script)], cwd=ROOT, check=True)


def main() -> None:
    run("fetch_osm_footprints.py")
    run("convert_geojson_to_unreal.py")
    print("\nInterVerseSG campus geometry pipeline completed successfully.")
    print("Next: open Unreal Editor and rebuild the InterVerse campus geometry actor.")


if __name__ == "__main__":
    main()
