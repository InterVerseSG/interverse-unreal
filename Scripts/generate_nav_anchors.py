"""Generate InterVerse campus navigation anchors in the current Unreal level.

Requirements:
- Enable the Unreal Engine Python Editor Script Plugin.
- Run this script from the Unreal Python console or an Editor Utility workflow.
- Verified anchors are created/updated as TargetPoint actors.
- Pending-coordinate POIs are reported but never placed at guessed positions.
"""

import json
import os
import unreal


ANCHOR_CONFIG = os.path.normpath(
    os.path.join(
        unreal.Paths.project_dir(),
        "Config",
        "InterVerseCampusAnchors.json",
    )
)

TAG_MANAGED = "InterVerseNavAnchor"
TAG_VERIFIED = "CoordinateVerified"


def _load_config():
    with open(ANCHOR_CONFIG, "r", encoding="utf-8") as handle:
        return json.load(handle)


def _find_managed_anchor(label):
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label() == label and TAG_MANAGED in [str(tag) for tag in actor.tags]:
            return actor
    return None


def _create_or_update_anchor(entry):
    label = entry["navigation_anchor"]
    location = unreal.Vector(
        float(entry["unreal_x_cm"]),
        float(entry["unreal_y_cm"]),
        0.0,
    )

    actor = _find_managed_anchor(label)
    if actor is None:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.TargetPoint,
            location,
            unreal.Rotator(0.0, 0.0, 0.0),
        )
        actor.set_actor_label(label)
    else:
        actor.set_actor_location(location, False, False)

    actor.tags = [
        unreal.Name(TAG_MANAGED),
        unreal.Name(TAG_VERIFIED),
        unreal.Name(entry["id"]),
    ]
    return actor


def generate():
    config = _load_config()
    anchors = config.get("anchors", [])
    pending = config.get("pending_anchors", [])

    created_or_updated = 0
    for entry in anchors:
        if entry.get("coordinate_status") != "verified":
            continue
        _create_or_update_anchor(entry)
        created_or_updated += 1

    unreal.log(
        "InterVerseSG: generated/updated {} verified NAV anchors.".format(
            created_or_updated
        )
    )

    for entry in pending:
        unreal.log_warning(
            "InterVerseSG pending coordinate: {} ({}) - not placed.".format(
                entry.get("display_name", entry.get("id")),
                entry.get("navigation_anchor"),
            )
        )

    unreal.EditorLevelLibrary.save_current_level()


if __name__ == "__main__":
    generate()
