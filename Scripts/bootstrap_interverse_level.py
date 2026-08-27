"""Create the initial InterVerseSG San German editor level.

Run inside Unreal Editor with Python Editor Script Plugin enabled.
The script creates/opens /Game/Maps/LV_InterVerse_SanGerman, adds a basic
lighting rig if missing, runs the NAV anchor generator, and saves the level.
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


def bootstrap():
    _ensure_level()
    _ensure_environment()
    _run_anchor_generator()
    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("InterVerseSG bootstrap complete: {}".format(LEVEL_PATH))


if __name__ == "__main__":
    bootstrap()
