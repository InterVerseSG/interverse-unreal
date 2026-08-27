"""Build the InterVerseSG terrain grid and apply terrain Z to NAV anchors.

Run from repository root:
    python Scripts/build_campus_terrain.py

This queries USGS 3DEP through EPQS. It may take a few minutes because the
campus grid is sampled point-by-point and intentionally rate-limited.
"""

from __future__ import annotations

import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCRIPTS = ROOT / "Scripts"


def run(name: str) -> None:
    print(f"\n=== Running {name} ===")
    subprocess.run([sys.executable, str(SCRIPTS / name)], cwd=ROOT, check=True)


def main() -> None:
    run("fetch_usgs_terrain_grid.py")
    run("apply_terrain_to_anchors.py")
    print("\nInterVerseSG terrain pipeline completed.")
    print("Next: open Unreal and rerun bootstrap_interverse_level.py.")


if __name__ == "__main__":
    main()
