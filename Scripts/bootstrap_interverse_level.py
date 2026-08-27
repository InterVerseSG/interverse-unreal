"""Create the initial InterVerseSG San German editor level.

Run inside Unreal Editor with Python Editor Script Plugin enabled.
The script creates/opens /Game/Maps/LV_InterVerse_SanGerman, adds a basic
lighting rig if missing, runs the NAV anchor generator, and adds verified
terrain, campus geometry, procedural building extrusion, and mapped circulation
surfaces when their generated data files are available.
"""

import os
import unreal

LEVEL_PATH = "/Game/Maps/LV_InterVerse_SanGerman"


def _actor_with_label(label):
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def _ensure_actor(actor_class, label, location, rotation=None):
    actor = _actor_with_label(label)
    if actor:
        return actor
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        actor_class,
        location,
        rotation or unreal.Rotator(0.0, 0.0, 0.0),
    )
    actor.set_actor_label(label)
    return actor


def _ensure_level():
    if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        unreal.EditorLevelLibrary.load_level(LEVEL_PATH)
    else:
        unreal.EditorLevelLibrary.new_level(LEVEL_PATH)


def _ensure_environment():
    _ensure_actor(
        unreal.DirectionalLight,
        "IV_DirectionalLight",
        unreal.Vector(0.0, 0.0, 500.0),
        unreal.Rotator(-45.0, -30.0, 0.0),
    )
    _ensure_actor(unreal.SkyLight, "IV_SkyLight", unreal.Vector(0.0, 0.0, 300.0))
    _ensure_actor(unreal.SkyAtmosphere, "IV_SkyAtmosphere", unreal.Vector(0.0, 0.0, 0.0))
    _ensure_actor(unreal.ExponentialHeightFog, "IV_HeightFog", unreal.Vector(0.0, 0.0, 0.0))


def _run_anchor_generator():
    path = os.path.join(unreal.Paths.project_dir(), "Scripts", "generate_nav_anchors.py")
    namespace = {"__name__": "__main__"}
    with open(path, "r", encoding="utf-8") as handle:
        exec(compile(handle.read(), path, "exec"), namespace)


def _config_exists(filename):
    return os.path.exists(os.path.join(unreal.Paths.project_dir(), "Config", filename))


def _data_exists(filename):
    return os.path.exists(os.path.join(unreal.Paths.project_dir(), "Data", filename))


def _ensure_terrain_actor():
    if not _data_exists("campus_terrain_grid.json"):
        unreal.log_warning(
            "InterVerseSG terrain grid is not built yet. Run "
            "python Scripts/build_campus_terrain.py outside Unreal first."
        )
        return

    actor_class = getattr(unreal, "InterVerseTerrainActor", None)
    if actor_class is None:
        unreal.log_warning(
            "InterVerseTerrainActor C++ class is unavailable. Compile the project, then rerun the bootstrap."
        )
        return

    actor = _ensure_actor(actor_class, "IV_CampusTerrain", unreal.Vector(0.0, 0.0, 0.0))
    try:
        actor.rebuild_terrain()
    except Exception as exc:
        unreal.log_warning("InterVerseSG terrain rebuild warning: {}".format(exc))


def _ensure_geometry_actor():
    if not _config_exists("InterVerseCampusGeometry.local.json"):
        unreal.log_warning(
            "InterVerseSG geometry config is not built yet. Run "
            "python Scripts/build_campus_geometry.py outside Unreal first."
        )
        return

    actor_class = getattr(unreal, "InterVerseCampusGeometryActor", None)
    if actor_class is None:
        unreal.log_warning(
            "InterVerseCampusGeometryActor C++ class is unavailable. Compile the project, then rerun the bootstrap."
        )
        return

    actor = _ensure_actor(actor_class, "IV_CampusGeometry", unreal.Vector(0.0, 0.0, 0.0))
    try:
        actor.rebuild_geometry()
    except Exception as exc:
        unreal.log_warning("InterVerseSG geometry rebuild warning: {}".format(exc))


def _ensure_building_extrusion_actor():
    if not _config_exists("InterVerseCampusGeometry.local.json"):
        return

    actor_class = getattr(unreal, "InterVerseBuildingExtrusionActor", None)
    if actor_class is None:
        unreal.log_warning(
            "InterVerseBuildingExtrusionActor C++ class is unavailable. Ensure ProceduralMeshComponent is enabled, compile the project, then rerun the bootstrap."
        )
        return

    actor = _ensure_actor(actor_class, "IV_CampusBuildings", unreal.Vector(0.0, 0.0, 0.0))
    try:
        actor.rebuild_buildings()
    except Exception as exc:
        unreal.log_warning("InterVerseSG building extrusion warning: {}".format(exc))


def _ensure_surface_actor():
    if not _config_exists("InterVerseCampusSurfaces.local.json"):
        unreal.log_warning(
            "InterVerseSG circulation/surface config is not built yet. Run "
            "python Scripts/build_campus_geometry.py outside Unreal first."
        )
        return

    actor_class = getattr(unreal, "InterVerseCampusSurfaceActor", None)
    if actor_class is None:
        unreal.log_warning(
            "InterVerseCampusSurfaceActor C++ class is unavailable. Compile the project, then rerun the bootstrap."
        )
        return

    actor = _ensure_actor(actor_class, "IV_CampusSurfaces", unreal.Vector(0.0, 0.0, 0.0))
    try:
        actor.rebuild_surfaces()
    except Exception as exc:
        unreal.log_warning("InterVerseSG campus surface rebuild warning: {}".format(exc))


def bootstrap():
    _ensure_level()
    _ensure_environment()
    _ensure_terrain_actor()
    _run_anchor_generator()
    _ensure_geometry_actor()
    _ensure_building_extrusion_actor()
    _ensure_surface_actor()
    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("InterVerseSG bootstrap complete: {}".format(LEVEL_PATH))


if __name__ == "__main__":
    bootstrap()
