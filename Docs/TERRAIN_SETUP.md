# InterVerseSG terrain workflow

InterVerseSG uses USGS 3DEP elevation data through the Elevation Point Query Service (EPQS) to build a lightweight terrain mesh for the San Germán campus.

## Important accuracy note

The terrain is for immersive visualization, navigation, and digital-twin prototyping. EPQS elevations are interpolated from 3DEP and must not be treated as engineering/survey measurements.

## 1. Generate the terrain grid

From the repository root, run:

```powershell
python Scripts/build_campus_terrain.py
```

This process:

1. Reads the verified campus NAV anchors.
2. Creates a campus bounding rectangle plus a 60 m margin.
3. Samples USGS 3DEP elevation at a 30 m grid spacing.
4. Stores the grid in `Data/campus_terrain_grid.json`.
5. Uses the ground elevation at Marquis Science Hall as Unreal Z=0.
6. Bilinearly interpolates a terrain height for every verified NAV anchor.
7. Updates `Config/InterVerseCampusAnchors.json` with `unreal_z_cm`.

The sampler is intentionally rate-limited and may take several minutes.

## 2. Build geometry and surfaces

If not already done:

```powershell
python Scripts/build_campus_geometry.py
```

## 3. Open Unreal Engine

Open `InterVerseSG.uproject`, allow C++ compilation, then run in the Unreal Python console:

```python
exec(open(unreal.Paths.project_dir() + "Scripts/bootstrap_interverse_level.py", encoding="utf-8").read())
```

The bootstrap will create/update:

- `IV_CampusTerrain` — USGS 3DEP procedural terrain mesh
- `NAV_*` — navigation anchors with terrain-aware Z values
- `IV_CampusGeometry` — verified geometry outlines
- `IV_CampusBuildings` — procedural OSM building extrusions
- `IV_CampusSurfaces` — mapped roads, paths, and parking

## Terrain settings

Current development settings:

- terrain source: USGS 3DEP EPQS
- grid spacing: 30 m
- margin around verified POIs: 60 m
- horizontal scale: 1 m = 100 Unreal units
- vertical scale: 1 m = 100 Unreal units
- Z datum: relative to the terrain elevation at Marquis Science Hall

The 30 m grid is intentionally lightweight for the Meta Quest prototype. A denser grid can be used later for a desktop/high-detail build or for selected areas of the campus.
