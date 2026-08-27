# InterVerseSG campus geometry data

This directory is the handoff point for real campus geometry.

## Canonical input

Place the campus export at:

`Data/campus_geometry.geojson`

The file may contain:

- building footprints as `Polygon` / `MultiPolygon`
- roads, pedestrian paths and sidewalks as `LineString` / `MultiLineString`
- parking areas as `Polygon`
- verified POIs as `Point`

## Required properties

Every feature should include, whenever known:

- `id`: stable InterVerse identifier
- `name`: human-readable name
- `category`: `building`, `road`, `path`, `parking`, `poi`, `green_area`, etc.
- `source`: source of the geometry
- `verified`: boolean

For OpenStreetMap-derived features, preserve attribution metadata such as `osm_type`, `osm_id`, and relevant tags when possible.

## Coordinate policy

The source file remains WGS84 longitude/latitude. Do not manually convert coordinates in the GeoJSON.

Run:

`python Scripts/convert_geojson_to_unreal.py`

This generates:

`Config/InterVerseCampusGeometry.local.json`

using Marquis Science Hall as the current local origin and the project convention `1 meter = 100 Unreal units`.

## Accuracy rule

Do not add guessed building outlines, paths or parking areas. If exact geometry is not yet available, keep using the verified navigation anchor plus the prototype placeholder until the footprint is verified.
