"""Create the initial InterVerseSG San German editor level.

Run inside Unreal Editor with Python Editor Script Plugin enabled.
The script creates/opens /Game/Maps/LV_InterVerse_SanGerman, applies a Quest-safe
tropical daylight profile, runs the NAV anchor generator, adds verified terrain,
campus geometry, batched procedural buildings, mapped circulation surfaces,
Quest-optimized HISM foliage and mapped campus props when available, plus a
minimal OpenXR pawn.
"""

import json
import os
import unreal

LEVEL_PATH = "/Game/Maps/LV_InterVerse_SanGerman"
ENV_PROFILE = os.path.join(unreal.Paths.project_dir(), "Config", "InterVerseQuestEnvironment.json")


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


def _safe_set(obj, name, value):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception as exc:
        unreal.log_warning("InterVerseSG environment property {} could not be set: {}".format(name, exc))
        return False


def _load_environment_profile():
    if not os.path.exists(ENV_PROFILE):
        return {}
    try:
        with open(ENV_PROFILE, "r", encoding="utf-8") as handle:
            return json.load(handle)
    except Exception as exc:
        unreal.log_warning("InterVerseSG environment profile could not be read: {}".format(exc))
        return {}


def _ensure_level():
    if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        unreal.EditorLevelLibrary.load_level(LEVEL_PATH)
    else:
        unreal.EditorLevelLibrary.new_level(LEVEL_PATH)


def _ensure_environment():
    profile = _load_environment_profile()
    sun_cfg = profile.get("sun", {})
    sky_cfg = profile.get("skylight", {})
    atmosphere_cfg = profile.get("atmosphere", {})

    sun_rotation = unreal.Rotator(
        float(sun_cfg.get("rotation_pitch", -48.0)),
        float(sun_cfg.get("rotation_yaw", -28.0)),
        float(sun_cfg.get("rotation_roll", 0.0)),
    )
    sun = _ensure_actor(
        unreal.DirectionalLight,
        "IV_DirectionalLight",
        unreal.Vector(0.0, 0.0, 500.0),
        sun_rotation,
    )
    try:
        sun.set_actor_rotation(sun_rotation, False)
    except Exception:
        pass
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    if light:
        _safe_set(light, "intensity", float(sun_cfg.get("intensity", 6.0)))
        rgb = sun_cfg.get("color_rgb", [255, 244, 222])
        _safe_set(light, "light_color", unreal.Color(int(rgb[0]), int(rgb[1]), int(rgb[2]), 255))
        _safe_set(light, "cast_shadows", bool(sun_cfg.get("cast_shadows", True)))

    sky = _ensure_actor(unreal.SkyLight, "IV_SkyLight", unreal.Vector(0.0, 0.0, 300.0))
    sky_light = sky.get_component_by_class(unreal.SkyLightComponent)
    if sky_light:
        _safe_set(sky_light, "intensity_scale", float(sky_cfg.get("intensity_scale", 0.85)))
        _safe_set(sky_light, "real_time_capture", bool(sky_cfg.get("real_time_capture", False)))

    if atmosphere_cfg.get("enabled", True):
        _ensure_actor(unreal.SkyAtmosphere, "IV_SkyAtmosphere", unreal.Vector(0.0, 0.0, 0.0))

    fog = _ensure_actor(unreal.ExponentialHeightFog, "IV_HeightFog", unreal.Vector(0.0, 0.0, 0.0))
    fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    if fog_component:
        _safe_set(fog_component, "fog_density", float(atmosphere_cfg.get("height_fog_density", 0.0)))

    unreal.log("InterVerseSG Quest tropical daylight environment profile applied.")


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
        unreal.log_warning("InterVerseSG terrain grid is not built yet. Run python Scripts/build_campus_terrain.py outside Unreal first.")
        return
    actor_class = getattr(unreal, "InterVerseTerrainActor", None)
    if actor_class is None:
        unreal.log_warning("InterVerseTerrainActor C++ class is unavailable. Compile the project, then rerun the bootstrap.")
        return
    actor = _ensure_actor(actor_class, "IV_CampusTerrain", unreal.Vector(0.0, 0.0, 0.0))
    try:
        actor.rebuild_terrain()
    except Exception as exc:
        unreal.log_warning("InterVerseSG terrain rebuild warning: {}".format(exc))


def _ensure_geometry_actor():
    if not _config_exists("InterVerseCampusGeometry.local.json"):
        unreal.log_warning("InterVerseSG geometry config is not built yet. Run python Scripts/build_campus_geometry.py outside Unreal first.")
        return
    actor_class = getattr(unreal, "InterVerseCampusGeometryActor", None)
    if actor_class is None:
        unreal.log_warning("InterVerseCampusGeometryActor C++ class is unavailable. Compile the project, then rerun the bootstrap.")
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
        unreal.log_warning("InterVerseBuildingExtrusionActor C++ class is unavailable. Ensure ProceduralMeshComponent is enabled, compile the project, then rerun the bootstrap.")
        return
    actor = _ensure_actor(actor_class, "IV_CampusBuildings", unreal.Vector(0.0, 0.0, 0.0))
    try:
        actor.rebuild_buildings()
    except Exception as exc:
        unreal.log_warning("InterVerseSG building extrusion warning: {}".format(exc))


def _ensure_surface_actor():
    if not _config_exists("InterVerseCampusSurfaces.local.json"):
        unreal.log_warning("InterVerseSG circulation/surface config is not built yet. Run python Scripts/build_campus_geometry.py outside Unreal first.")
        return
    actor_class = getattr(unreal, "InterVerseCampusSurfaceActor", None)
    if actor_class is None:
        unreal.log_warning("InterVerseCampusSurfaceActor C++ class is unavailable. Compile the project, then rerun the bootstrap.")
        return
    actor = _ensure_actor(actor_class, "IV_CampusSurfaces", unreal.Vector(0.0, 0.0, 0.0))
    try:
        actor.rebuild_surfaces()
    except Exception as exc:
        unreal.log_warning("InterVerseSG campus surface rebuild warning: {}".format(exc))


def _ensure_foliage_actor():
    if not _config_exists("InterVerseFoliage.local.json"):
        unreal.log_warning("InterVerseSG foliage data not built yet. Optional: run python Scripts/fetch_osm_foliage.py outside Unreal.")
        return
    actor_class = getattr(unreal, "InterVerseFoliageActor", None)
    if actor_class is None:
        unreal.log_warning("InterVerseFoliageActor C++ class is unavailable. Compile the project, then rerun the bootstrap.")
        return
    actor = _ensure_actor(actor_class, "IV_CampusFoliage", unreal.Vector(0.0, 0.0, 0.0))
    try:
        actor.rebuild_foliage()
    except Exception as exc:
        unreal.log_warning("InterVerseSG foliage rebuild warning: {}".format(exc))


def _ensure_props_actor():
    if not _config_exists("InterVerseCampusProps.local.json"):
        unreal.log_warning("InterVerseSG mapped campus props not built yet. Optional: run python Scripts/fetch_osm_campus_props.py outside Unreal.")
        return
    actor_class = getattr(unreal, "InterVerseCampusPropsActor", None)
    if actor_class is None:
        unreal.log_warning("InterVerseCampusPropsActor C++ class is unavailable. Compile the project, then rerun the bootstrap.")
        return
    actor = _ensure_actor(actor_class, "IV_CampusProps", unreal.Vector(0.0, 0.0, 0.0))
    try:
        actor.rebuild_props()
    except Exception as exc:
        unreal.log_warning("InterVerseSG campus props rebuild warning: {}".format(exc))


def _find_anchor_location(anchor_name):
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if anchor_name in [str(tag) for tag in actor.tags]:
            return actor.get_actor_location()
    return unreal.Vector(0.0, 0.0, 100.0)


def _ensure_xr_pawn():
    actor_class = getattr(unreal, "InterVerseXRPawn", None)
    if actor_class is None:
        unreal.log_warning("InterVerseXRPawn C++ class is unavailable. Compile the project, then rerun the bootstrap.")
        return
    start = _find_anchor_location("NAV_MarquisScienceHall")
    start.z += 10.0
    pawn = _ensure_actor(actor_class, "IV_XRPawn", start)
    unreal.log("InterVerseSG XR Pawn ready at {}".format(pawn.get_actor_location()))


def bootstrap():
    _ensure_level()
    _ensure_environment()
    _ensure_terrain_actor()
    _run_anchor_generator()
    _ensure_geometry_actor()
    _ensure_building_extrusion_actor()
    _ensure_surface_actor()
    _ensure_foliage_actor()
    _ensure_props_actor()
    _ensure_xr_pawn()
    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("InterVerseSG bootstrap complete: {}".format(LEVEL_PATH))


if __name__ == "__main__":
    bootstrap()
