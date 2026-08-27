# InterVerseSG — First Meta Quest Navigation Test

This test validates the campus terrain/navigation stack before adding advanced locomotion or final building art.

## 1. Rebuild generated campus data

From the repository root with normal system Python:

```powershell
python Scripts/build_campus_terrain.py
python Scripts/build_campus_geometry.py
```

The second command automatically applies the generated terrain Z to building bases, roads, paths and parking when the terrain config is present.

## 2. Open/compile Unreal

Open `InterVerseSG.uproject` in Unreal Engine and allow the C++ module to compile.

Run in Unreal's Python console:

```python
exec(open(unreal.Paths.project_dir() + "Scripts/bootstrap_interverse_level.py", encoding="utf-8").read())
```

Expected level actors include:

- `IV_CampusTerrain`
- `IV_CampusGeometry`
- `IV_CampusBuildings`
- `IV_CampusSurfaces`
- `IV_XRPawn`
- `NAV_*` TargetPoints

`NAV_EscuelaGraduada` must be present and carry these runtime tags:

- `InterVerseNavAnchor`
- `CoordinateVerified`
- `EscuelaGraduada`
- `NAV_EscuelaGraduada`

## 3. Local navigation test (no cloud)

Create a temporary Blueprint derived from `InterVerseXRPawn`, or use the placed `IV_XRPawn` and expose a temporary input/event that calls:

```text
InterVerseNavigation -> NavigateToAnchor("NAV_EscuelaGraduada")
```

Expected behavior: the XR Pawn teleports to the verified EEGEI navigation point while preserving its current rotation.

This test proves that TargetPoint lookup and terrain-aware destination Z work independently of Gemini/Render.

## 4. End-to-end Gemini test

The `InterVerseXRPawn` already contains:

- `InterVerseCloudClient`
- `InterVerseNavigationComponent`

`InterVerseCloudClient` defaults to:

```text
bAutoValidateAssistantCommands = true
bAutoExecuteValidatedNavigation = true
```

Call `AskAssistant` with:

```text
Message: Llévame a la Escuela Graduada
Context: El usuario está en el Recinto de San Germán.
SessionId: quest-nav-001
```

Expected flow:

```text
Meta Quest / Unreal
  -> interverse-api
  -> Gemini
  -> navigate + Escuela Graduada
  -> interverse-builder
  -> NAV_EscuelaGraduada
  -> InterVerseNavigationComponent
  -> TeleportTo verified EEGEI point
```

## 5. Safety behavior

- Unknown `NAV_*` destinations are not teleported to.
- A command rejected by Builder is not executed.
- Non-`navigate` commands are ignored by the navigation component.
- Commands that need confirmation remain subject to Builder confirmation logic.
- Runtime lookup uses actor tags rather than editor-only actor labels, so it remains available in packaged Meta Quest builds.

## 6. Current locomotion scope

The first implementation deliberately uses destination teleportation. It is the lowest-risk way to validate AI navigation in VR and helps reduce motion sickness during the prototype phase.

Later phases can add:

- controller-based teleport arc
- smooth locomotion
- snap turning
- guided path visualization
- accessible route selection
- spoken destination confirmation
