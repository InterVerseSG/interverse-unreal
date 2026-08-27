# InterVerseSG — Georeferenced campus prototype

This stage validates campus scale and navigation before final 3D building models are produced.

## What is authoritative

- POI positions in `Config/InterVerseCampusAnchors.json` when `coordinate_status` is `verified`.
- Navigation anchor names such as `NAV_CAI`, `NAV_CentroEstudiantes`, and `NAV_EscuelaGraduada`.
- Coordinate conversion rule: 1 real meter = 100 Unreal units; east = +X; north = +Y.

## What is NOT authoritative yet

- Placeholder building width, depth, height, facade, roof, orientation, doors, windows, terrain elevation, roads, and sidewalks.
- The generated cubes are only spatial markers for validating distance and navigation.

## Run in Unreal Editor

1. Open `InterVerseSG.uproject`.
2. Open or create `LV_InterVerse_SanGerman`.
3. Enable **Python Editor Script Plugin** if it is not already enabled, then restart Unreal.
4. Open **Window > Output Log** and use Python mode.
5. Generate/update navigation anchors:

```python
exec(open(unreal.Paths.project_dir() + "Scripts/generate_nav_anchors.py", encoding="utf-8").read())
```

6. Generate the visual prototype:

```python
exec(open(unreal.Paths.project_dir() + "Scripts/generate_campus_prototype.py", encoding="utf-8").read())
```

7. In the World Outliner search for:

```text
NAV_
BLDG_
PAD_
LBL_
```

## EEGEI

The School of Graduate Studies and Research is now a verified coordinate:

- ID: `EscuelaGraduada`
- Navigation anchor: `NAV_EscuelaGraduada`
- Latitude: `18.08518322539467`
- Longitude: `-67.05382825210272`

The placeholder is labeled `BLDG_EscuelaGraduada` and is deliberately not a claim about the real building footprint or height.

## Validation checklist

Before detailed modeling, inspect whether the relative positions make sense visually, especially:

- `NAV_MarquisScienceHall`
- `NAV_CAI`
- `NAV_CentroEstudiantes`
- `NAV_CarlosJTorres`
- `NAV_EscuelaGraduada`
- `NAV_PistaSambolin`
- `NAV_PolideportivoSambolin`

If an anchor is wrong, correct the source coordinates/configuration rather than dragging the managed actor manually. Re-running the generator will restore the configured position.
