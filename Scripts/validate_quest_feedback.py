"""Validate Quest haptic and controller feedback architecture without Unreal."""

from __future__ import annotations

import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
H = ROOT / "Source" / "InterVerseRuntime" / "Public" / "InterVerseQuestFeedbackActor.h"
CPP = ROOT / "Source" / "InterVerseRuntime" / "Private" / "InterVerseQuestFeedbackActor.cpp"
BOOTSTRAP = ROOT / "Scripts" / "bootstrap_interverse_level.py"


def require(path: pathlib.Path, token: str) -> None:
    text = path.read_text(encoding="utf-8")
    if token not in text:
        raise AssertionError(f"Missing {token!r} in {path.relative_to(ROOT)}")


def main() -> int:
    for token in (
        "bEnableHaptics",
        "HoverAmplitude",
        "PressAmplitude",
        "TeleportAmplitude",
        "ArrivalAmplitude",
        "ArrivalDistanceCm",
        "HandleTeleportCompleted",
    ):
        require(H, token)

    for token in (
        "SetHapticsByValue",
        "EControllerHand::Right",
        "OnTeleportCompleted.AddDynamic",
        "IsPointerHoveringWidget",
        "IsPointerPressed",
        "GetGuidanceDistanceCm",
        "ArrivalDistanceCm",
        "RightControllerVisual->SetRelativeScale3D",
        "bEnableControllerScaleFeedback",
    ):
        require(CPP, token)

    require(BOOTSTRAP, "_ensure_quest_feedback")
    require(BOOTSTRAP, "InterVerseQuestFeedbackActor")
    require(BOOTSTRAP, "IV_QuestFeedback")
    require(BOOTSTRAP, "_ensure_quest_feedback()")

    print("PASS: Quest hover/press haptics")
    print("PASS: Quest teleport completion haptic")
    print("PASS: Quest destination arrival haptic")
    print("PASS: right-controller hover/press scale feedback")
    print("PASS: IV_QuestFeedback bootstrap integration")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}")
        raise SystemExit(1)
