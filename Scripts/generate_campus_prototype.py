"""Generate a lightweight 3D campus prototype from verified InterVerse anchors.

This is intentionally a validation model, not final architecture.
It creates one simple platform + placeholder building block + label per verified POI,
using the real local ENU positions already stored in Config/InterVerseCampusAnchors.json.

Requirements:
- Unreal Engine Editor
- Python Editor Script Plugin enabled
- Run after generate_nav_anchors.py / bootstrap_interverse_level.py
"""

import json
import os
import unreal

CONFIG_PATH = os.path.normpath(
    os.path.join(unreal.Paths.project_dir(), "Config", "InterVerseCampusAnchors.json")
)

TAG_MANAGED = "InterVersePrototype"
GROUND_Z_CM = 0.0
PLATFORM_Z_CM = 10.0
BUILDING_BASE_Z_CM = 60.0

# Placeholder dimensions only. They are NOT real building footprints.
DEFAULT_BUILDING_SIZE_CM = unreal.Vector(1800.0, 1200.0, 700.0)
EEGEI_BUILDING_SIZE_CM = unreal.Vector(2200.0, 1500.0, 900.0)


def _load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as handle:
        return json.load(handle)


def _managed_actors():
    return [
        actor
        for actor in unreal.EditorLevelLibrary.get_all_level_actors()
        if TAG_MANAGED in [str(tag) for tag in actor.tags]
    ]


def clear_existing():
    for actor in _managed_actors():
        unreal.EditorLevelLibrary.destroy_actor(actor)


def _spawn_cube(label, location, scale, extra_tags):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor,
        location,
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    actor.set_actor_label(label)
    actor.tags = [unreal.Name(TAG_MANAGED), *[unreal.Name(tag) for tag in extra_tags]]

    mesh = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    component.set_world_scale3d(scale)
    component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    return actor


def _spawn_label(text, location, poi_id):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.TextRenderActor,
        location,
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    actor.set_actor_label(f"LBL_{poi_id}")
    actor.tags = [unreal.Name(TAG_MANAGED), unreal.Name("InterVerseLabel"), unreal.Name(poi_id)]
    component = actor.text_render
    component.set_text(text)
    component.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    component.set_world_size(120.0)
    component.set_text_render_color(unreal.Color(255, 255, 255, 255))
    return actor


def _building_size(entry):
    if entry.get("id") == "EscuelaGraduada":
        return EEGEI_BUILDING_SIZE_CM
    return DEFAULT_BUILDING_SIZE_CM


def generate():
    config = _load_config()
    anchors = [a for a in config.get("anchors", []) if a.get("coordinate_status") == "verified"]

    clear_existing()

    # Campus validation plane: 800 m x 800 m around the current origin.
    _spawn_cube(
        "PROTO_CampusGround",
        unreal.Vector(-20000.0, 15000.0, GROUND_Z_CM - 25.0),
        unreal.Vector(400.0, 400.0, 0.25),
        ["InterVerseGround"],
    )

    count = 0
    for entry in anchors:
        x = float(entry["unreal_x_cm"])
        y = float(entry["unreal_y_cm"])
        poi_id = entry["id"]
        display_name = entry.get("display_name", poi_id)

        # 6 m x 6 m marker platform.
        _spawn_cube(
            f"PAD_{poi_id}",
            unreal.Vector(x, y, PLATFORM_Z_CM),
            unreal.Vector(3.0, 3.0, 0.1),
            ["InterVersePOIPad", poi_id],
        )

        size = _building_size(entry)
        # Engine cube is 100 cm per side, so scale = desired cm / 100.
        scale = unreal.Vector(size.x / 100.0, size.y / 100.0, size.z / 100.0)
        _spawn_cube(
            f"BLDG_{poi_id}",
            unreal.Vector(x, y, BUILDING_BASE_Z_CM + size.z / 2.0),
            scale,
            ["InterVerseBuildingPlaceholder", poi_id],
        )

        _spawn_label(
            display_name,
            unreal.Vector(x, y, BUILDING_BASE_Z_CM + size.z + 250.0),
            poi_id,
        )
        count += 1

    unreal.EditorLevelLibrary.save_current_level()
    unreal.log(f"InterVerseSG: generated {count} verified campus prototype POIs.")
    unreal.log_warning(
        "Prototype building blocks are placeholders only; positions are georeferenced, footprints and heights are not yet authoritative."
    )


if __name__ == "__main__":
    generate()
