# InterVerseSG — Quest Building Material Strategy

## Objective

Improve campus readability on Meta Quest without increasing draw calls or shader cost unnecessarily.

The procedural building actor exposes two shared materials only:

- `NearBuildingMaterial`
- `FarBuildingMaterial`

The actor switches materials per campus sector using the same 0.5 s sector visibility update used for runtime culling. No per-frame material LOD tick is required.

## Near material

Recommended asset name:

`M_IV_Building_Near_Mobile`

Recommended properties:

- Opaque blend mode.
- Default Lit or Unlit only if performance testing requires it.
- One facade atlas texture maximum.
- Vertex Color multiplied into Base Color.
- UV0 used for facade/roof mapping.
- Roughness: constant or one inexpensive scalar.
- Metallic: 0.
- No normal map initially.
- No parallax.
- No pixel depth offset.
- No transparency for windows.
- No clear coat.
- No refraction.
- No world-position-offset animation.

The generated mesh encodes:

- wall vertex color approximately `(0.82, 0.82, 0.78)`
- roof vertex color approximately `(0.48, 0.48, 0.46)`
- wall UV repetition using `FacadeURepeatCm` and `FacadeVRepeatCm`
- roof UVs using planar XY projection

Default rhythm:

- horizontal facade repeat: 400 cm
- vertical facade repeat: 300 cm

This allows windows and facade rhythm to be represented by texture rather than extra geometry.

## Far material

Recommended asset name:

`M_IV_Building_Far_Mobile`

Recommended properties:

- Opaque.
- Flat color multiplied by Vertex Color.
- No texture sample required.
- No normal map.
- Constant roughness.
- Metallic 0.

This is the material used for sectors beyond `NearMaterialDistanceCm`.

Default material transition distance:

`18000 cm` = approximately 180 m.

## Runtime behavior

The building actor already performs:

1. sector visibility culling;
2. nearest-sector fallback;
3. near/far material selection;
4. material changes only when the sector crosses the near/far threshold.

The same campus sector remains a single procedural mesh section, so adding facade appearance does not create a draw call per building.

## Quest constraints

Avoid the following unless measured on-device:

- translucent glass windows;
- multiple materials per building;
- one material instance per building;
- high-resolution unique textures per building;
- dynamic reflections;
- planar reflections;
- tessellation/displacement;
- expensive procedural noise networks;
- dynamic shadow casting on every facade detail.

## Recommended atlas

Start with one 1024×1024 or 2048×2048 atlas containing a small number of facade patterns:

- light concrete / institutional wall;
- green-tinted window rhythm;
- neutral window rhythm;
- roof/general utility surface.

Use color and UV variation before introducing additional texture sets.

## Validation target

Before increasing visual complexity, validate on Quest with:

- stable frame timing;
- sector transitions without visible stalls;
- acceptable texture memory;
- readable facade rhythm at normal walking distance;
- no material-switch hitch when moving between sectors.

The visual target is a recognizable digital twin, not architectural photorealism at the expense of standalone VR performance.
