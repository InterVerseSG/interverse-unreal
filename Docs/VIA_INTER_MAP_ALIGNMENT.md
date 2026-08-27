# Vía Inter - SG map alignment

Vía Inter - SG (`https://viaintersg.org/index.html`) is the canonical 2D campus map for the production InterVerseSG environment.

## Production rule

The Unreal campus must preserve the verified geographic layout and POI names from Vía Inter - SG. Test destinations such as `Classroom101` are not production campus data.

## Coordinate pipeline

Each verified POI should be stored as:

```json
{
  "id": "stable-id",
  "display_name": "Official map label",
  "latitude": 0.0,
  "longitude": 0.0,
  "category": "building",
  "navigation_anchor": "NAV_StableId"
}
```

For a campus-scale Quest build, convert latitude/longitude into a local tangent-plane coordinate system around one campus origin rather than rendering the entire globe. Unreal units are centimeters.

Recommended transform pipeline:

1. Choose a verified campus origin latitude/longitude from the Vía Inter data.
2. Convert every POI into local East/North meters relative to that origin.
3. Convert meters to Unreal centimeters (`1 m = 100 uu`).
4. Create one `BP_NavigationPoint` per verified POI using its `navigation_anchor`.
5. Place buildings and paths against the same calibrated coordinate frame.

## Required POI fields

- stable `id`
- official `display_name`
- latitude
- longitude
- category
- aliases (optional)
- navigation anchor
- source = `via-inter-sg`

## Quest design

Use the geographic coordinates for layout and navigation, but use optimized local meshes for the standalone Quest build. High-detail photogrammetry or Gaussian splats should be segmented and loaded only when needed.

## Import gate

Do not create production navigation anchors until the POI list and coordinates have been exported or otherwise verified from the Vía Inter - SG source data.
