"""Create the initial InterVerseSG San German editor level.

Run inside Unreal Editor with Python Editor Script Plugin enabled.
The script creates/opens /Game/Maps/LV_InterVerse_SanGerman, adds a basic
lighting rig if missing, runs the NAV anchor generator, optionally adds the
verified campus geometry visualization actor, and saves the level.
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
    _ensure_actor(
        unreal.SkyLight,
        "IV_SkyLight",
        unreal.Vector(0.0, 0.0, 300.0),
    )
    _ensure_actor(
        unreal.SkyAtmosphere,
        "IV_SkyAtmosphere",
        unreal.Vector(0.0, 0.0, 0.0),
    )
    _ensure_actor(
        unreal.ExponentialHeightFog,
        "IV_HeightFog",
        unreal.Vector(0.0, 0.0, 0.0),
    )


def _run_anchor_generator():
    path = os.path.join(
        unreal.Paths.project_dir(),
        "Scripts",
        "generate_nav_anchors.py",
    )
    namespace = {"__name__": "__main__"}
    with open(path, "r", encoding="utf-8") as handle:
        exec(compile(handle.read(), path, "exec"), namespace)


def _ensure_geometry_actor():
    config_path = os.path.join(
        unreal.Paths.project_dir(),
        "Config",
        "InterVerseCampusGeometry.local.json",
    )
    if not os.path.exists(config_path):
        unreal.log_warning(
            "InterVerseSG geometry config is not built yet. Run "
            "python Scripts/build_campus_geometry.py outside Unreal first."
        )
        return

    actor_class = getattr(unreal, "InterVerseCampusGeometryActor", None)
    if actor_class is None:
        unreal.log_warning(
            "InterVerseCampusGeometryActor C++ class is unavailable. Compile the project, "
            "then rerun the bootstrap."
        )
        return

    actor = _ensure_actor(
        actor_class,
        "IV_CampusGeometry",
        unreal.Vector(0.0, 0.0, 0.0),
    )
    try:
        actor.rebuild_geometry()
    except Exception as exc:
        unreal.log_warning("InterVerseSG geometry rebuild warning: {}".format(exc))


def bootstrap():
    _ensure_level()
    _ensure_environment()
    _run_anchor_generator()
    _ensure_geometry_actor()
    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("InterVerseSG bootstrap complete: {}".format(LEVEL_PATH))


if __name__ == "__main__":
    bootstrap()
