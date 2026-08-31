# InterVerseSG — Quest controller interaction

## Goal
Provide a lightweight controller-first interaction layer for Meta Quest without depending on expensive per-frame debug rendering.

## Current XR foundation
`AInterVerseXRPawn` already owns left/right `UMotionControllerComponent`, a world-space VR menu, and a right-hand `UWidgetInteractionComponent`. The right trigger is shared contextually: when the menu is hidden it controls teleport aiming; when the menu is visible it presses/releases the widget pointer.

## Interaction phase
1. Keep controller tracking through OpenXR/Quest motion sources.
2. Render a lightweight right-hand pointer only while the VR menu is visible.
3. End the pointer at the current widget hit location when available; otherwise clamp it to the configured interaction distance.
4. Expose hover state so the HUD/controller visual can react without changing navigation behavior.
5. Keep teleport visuals disabled while the menu is open so the two interaction modes cannot compete.

## Quest performance rules
- One pointer mesh, not a chain of spawned actors.
- No collision on pointer geometry.
- Reuse the procedural mesh section each frame only while the menu is visible.
- Keep debug rendering disabled in packaged builds.
- Prefer controller interaction first; optional articulated hand meshes/hand tracking can be added later as a separate feature tier.

## Acceptance test in Unreal
1. Open `LV_InterVerse_SanGerman`.
2. Run VR Preview with Quest connected through the supported OpenXR path.
3. Press X/Menu on the left controller to show the menu.
4. Aim the right controller at a menu button.
5. Verify the pointer terminates on the widget and the button receives hover state.
6. Press/release the right trigger and verify one click is generated.
7. Close the menu and verify the pointer disappears and teleport control returns to the right trigger.

## Next visual layer
After controller-pointer validation on hardware, add optional low-poly controller/hand representations and hover/pressed material feedback. This should remain independent from navigation, Gemini commands, and campus geometry so it can be disabled on lower performance profiles.
