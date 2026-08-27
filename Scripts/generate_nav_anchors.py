"""Generate InterVerse campus navigation anchors in the current Unreal level.

Requirements:
- Enable the Unreal Engine Python Editor Script Plugin.
- Run this script from the Unreal Python console or an Editor Utility workflow.
- Verified anchors are created/updated as TargetPoint actors.
- Pending-coordinate POIs are reported but never placed at guessed positions.

Safety:
- Only actors tagged InterVerseNavAnchor are modified.
- Duplicate navigation_anchor values in the JSON abort generation.
- Pending coordinates are never spawned at (0, 0, 0).
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
    if not os.path.exists(ANCHOR_CONFIG):
        raise RuntimeError(f"InterVerse anchor config not found: {ANCHOR_CONFIG}")

    with open(ANCHOR_CONFIG, "r", encoding="utf-8") as handle:
        return json.load(handle)


def _validate_config(config):
    seen = set()
    duplicates = []

    for entry in config.get("anchors", []):
        anchor = entry.get("navigation_anchor")
        if not anchor:
            raise RuntimeError(f"Verified anchor without navigation_anchor: {entry}")
        if anchor in seen:
            duplicates.append(anchor)
        seen.add(anchor)

        if entry.get("coordinate_status") == "verified":
            for key in ("unreal_x_cm", "unreal_y_cm"):
                if entry.get(key) is None:
                    raise RuntimeError(f"{anchor} is verified but missing {key}.")

    for entry in config.get("pending_anchors", []):
        anchor = entry.get("navigation_anchor")
        if anchor in seen:
            duplicates.append(anchor)
        if anchor:
            seen.add(anchor)

    if duplicates:
        raise RuntimeError(
            "Duplicate InterVerse navigation anchors: " + ", ".join(sorted(set(duplicates)))
        )


def _find_managed_anchor(label):
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        actor_tags = [str(tag) for tag in actor.tags]
        if actor.get_actor_label() == label and TAG_MANAGED in actor_tags:
            return actor
    return None


def _create_or_update_anchor(entry):
    label = entry["navigation_anchor"]
    location = unreal.Vector(
        float(entry["unreal_x_cm"]),
        float(entry["unreal_y_cm"]),
        float(entry.get("unreal_z_cm", 0.0)),
    )

    actor = _find_managed_anchor(label)
    created = actor is None

    if created:
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

    return actor, created


def generate():
    config = _load_config()
    _validate_config(config)

    anchors = config.get("anchors", [])
    pending = config.get("pending_anchors", [])

    created = 0
    updated = 0
    skipped = 0

    with unreal.ScopedEditorTransaction("Generate InterVerse Campus NAV Anchors"):
        for entry in anchors:
            if entry.get("coordinate_status") != "verified":
                skipped += 1
                continue

            _, was_created = _create_or_update_anchor(entry)
            if was_created:
                created += 1
            else:
                updated += 1

    unreal.log(
        "InterVerseSG NAV summary: created={}, updated={}, skipped={}, pending={}.".format(
            created,
            updated,
            skipped,
            len(pending),
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
    unreal.log("InterVerseSG: current level saved successfully.")


if __name__ == "__main__":
    generate()
