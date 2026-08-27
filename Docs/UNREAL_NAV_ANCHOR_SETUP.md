# InterVerseSG — Automatic campus NAV anchors in Unreal Engine

This procedure creates the verified campus navigation points from `Config/InterVerseCampusAnchors.json`.

## 1. Required Unreal plugin

In Unreal Engine:

1. Open **Edit → Plugins**.
2. Search for **Python Editor Script Plugin**.
3. Enable it.
4. Restart Unreal Engine when prompted.

This plugin is used only in the Editor to build/update the campus map. It is not required as a runtime dependency in the Quest APK.

## 2. Open the campus level

Create or open the main InterVerse campus level. Recommended name:

`LV_InterVerse_SanGerman`

Do not run the generator in an unrelated test level because the generated TargetPoint actors are saved into the current level.

## 3. Run the generator

Open:

**Window → Output Log**

Switch the command input to Python if needed, then execute:

```python
exec(open(unreal.Paths.project_dir() + "Scripts/generate_nav_anchors.py", encoding="utf-8").read())
```

The script reads:

`Config/InterVerseCampusAnchors.json`

and creates or updates `TargetPoint` actors for every entry whose `coordinate_status` is `verified`.

## 4. Expected behavior

Actors are labeled using the official InterVerse navigation anchor, for example:

- `NAV_MarquisScienceHall`
- `NAV_CAI`
- `NAV_CentroEstudiantes`
- `NAV_PolideportivoSambolin`
- `NAV_PistaSambolin`
- `NAV_CarlosJTorres`

Every managed actor receives these tags:

- `InterVerseNavAnchor`
- `CoordinateVerified`
- the canonical POI id

Pending POIs are **not** placed at guessed coordinates. The Output Log will show a warning instead. Example:

`Escuela de Estudios Graduados e Investigación (NAV_EscuelaGraduada) - not placed.`

## 5. Idempotent updates

The generator is safe to run again after adding or correcting coordinates. It finds existing InterVerse-managed actors by label and updates them rather than creating duplicates.

It will never move another actor that merely happens to have a similar name unless it also carries the `InterVerseNavAnchor` tag.

## 6. Validation

After generation:

1. Search the World Outliner for `NAV_`.
2. Verify that the points form the expected campus footprint.
3. Check Marquis Science Hall at the temporary local origin `(0,0)`.
4. Verify CAI, Centro de Estudiantes, Sambolín, Carlos J. Torres and the other known POIs relative to Marquis.
5. Save the level.

## 7. Runtime use

At runtime, `interverse-builder` returns a symbolic anchor such as:

```json
{
  "accepted": true,
  "action": "navigate",
  "target": "CAI",
  "navigation_anchor": "NAV_CAI"
}
```

`BP_InterVerseController` must resolve that label against the locally generated TargetPoint actors. The network response must never be treated as an arbitrary Unreal class or asset path.

## 8. EEGEI status

The **Escuela de Estudios Graduados e Investigación (EEGEI)** is already part of the official InterVerse location registry as:

`NAV_EscuelaGraduada`

Its coordinate is intentionally pending until a sufficiently reliable location is verified. Once added to `InterVerseCampusAnchors.json`, rerunning the generator will create it automatically.
