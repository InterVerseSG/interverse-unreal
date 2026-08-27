# InterVerseSG VR locomotion and guidance

This phase adds locomotion logic to `AInterVerseXRPawn` without making the Meta Quest controller bindings the source of truth. The movement code is independent from the final OpenXR/Enhanced Input mapping so it can be tested first on desktop.

## Implemented

- Smooth movement relative to the HMD yaw.
- Snap turning around the current HMD position to avoid artificial lateral jumps.
- Parabolic teleport aiming from the right motion controller.
- Teleport destination validation that rejects steep wall-like surfaces.
- Teleport that preserves the user's room-scale HMD XY offset.
- Guided navigation to any verified `NAV_*` anchor, providing direction and distance without moving the user.
- AI navigation remains available through `InterVerseCloudClient -> Builder -> NavigationAnchor -> InterVerseNavigationComponent`.

## Desktop fallback controls

The current `Config/DefaultInput.ini` provides development bindings:

| Function | Desktop | Generic controller |
| --- | --- | --- |
| Move | W/A/S/D | Left stick |
| Snap turn | Q / E | Right stick X |
| Teleport aim + commit | Hold/release T | Right trigger |

These are development fallbacks. Final Quest Touch bindings should be created through OpenXR + Enhanced Input before packaging the production APK.

## Test 1 — smooth movement

1. Build terrain and campus geometry.
2. Run `bootstrap_interverse_level.py`.
3. Press Play in Unreal.
4. Use W/A/S/D.
5. Movement should follow the camera/HMD horizontal facing direction.

## Test 2 — snap turn

Press Q or E. Each press should rotate approximately 30 degrees around the current camera position rather than around an arbitrary world origin.

## Test 3 — teleport

1. Aim the right controller at a walkable surface (or use the component functions from Blueprint during desktop-only testing).
2. Begin teleport aim.
3. The component computes a parabolic arc using gravity and traces it against `Visibility` collision.
4. Surfaces with an impact normal Z below 0.65 are rejected.
5. Release/commit to teleport.

A visual arc/reticle is intentionally not yet generated; the component publishes `OnTeleportAimChanged` so the next UI pass can draw it without changing locomotion logic.

## Test 4 — guided navigation

From Blueprint or Python-accessible gameplay logic, call on `IV_XRPawn.Navigation`:

`StartGuidanceToAnchor("NAV_EscuelaGraduada")`

The component updates:

- `GetGuidanceDirection()`
- `GetGuidanceDistanceCm()`
- `OnGuidanceUpdated`

No teleport occurs in guidance mode.

## Test 5 — AI navigation

Use `CloudClient.AskAssistant(...)` with a request equivalent to:

`Llévame a la Escuela Graduada`

The configured automatic flow is:

1. Gemini interprets the request.
2. InterVerse Builder validates the location.
3. Builder returns `NAV_EscuelaGraduada`.
4. `InterVerseNavigationComponent` resolves the runtime actor tag.
5. The XR Pawn teleports to the verified target.

## Safety and comfort defaults

- Smooth movement can be disabled with `bSmoothMovementEnabled`.
- Snap turn defaults to 30 degrees.
- Teleport rejects steep surfaces.
- Navigation anchors use runtime Actor Tags rather than editor-only actor labels.
- Manual teleport and AI navigation remain separate mechanisms.

## Next phase

Create final OpenXR/Enhanced Input Touch mappings and add visual comfort/navigation feedback:

- teleport spline/arc,
- destination reticle,
- guidance arrow,
- distance label,
- optional vignette for smooth movement,
- Quest performance settings and Android packaging profile.
