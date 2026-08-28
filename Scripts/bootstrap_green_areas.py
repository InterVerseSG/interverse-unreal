"""Create/update IV_CampusGreenAreas inside the current Unreal level."""
import unreal


def main():
    actor_class = getattr(unreal, "InterVerseGreenAreaActor", None)
    if actor_class is None:
        unreal.log_warning("InterVerseGreenAreaActor is unavailable. Compile the C++ project first.")
        return
    actor = None
    for item in unreal.EditorLevelLibrary.get_all_level_actors():
        if item.get_actor_label() == "IV_CampusGreenAreas":
            actor = item
            break
    if actor is None:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(actor_class, unreal.Vector(0.0, 0.0, 0.0))
        actor.set_actor_label("IV_CampusGreenAreas")
    try:
        actor.rebuild_green_areas()
        unreal.EditorLevelLibrary.save_current_level()
        unreal.log("InterVerseSG mapped green areas ready.")
    except Exception as exc:
        unreal.log_warning("InterVerseSG green area rebuild warning: {}".format(exc))


if __name__ == "__main__":
    main()
