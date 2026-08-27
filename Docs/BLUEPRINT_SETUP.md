# Blueprint setup — first vertical slice

## BP_InterVerseController

Create an Actor Blueprint named `BP_InterVerseController` and add the `InterVerseCloudClient` component.

Bind these events:

### OnAssistantCommand
1. Display `Command.Response` in `WBP_InterGuidePanel`.
2. Call `ValidateCommand(Command, false)`.

### OnCommandValidated
1. If `Accepted == false`, display `Reason` and stop.
2. If `RequiresConfirmation == true`, show `WBP_ConfirmAction` and do not execute yet.
3. If `Action == navigate`, resolve `Target` against a navigation registry.
4. If `Action == create_object`, resolve `BlueprintClass` against the local allowlist and spawn it only in the requested approved zone.
5. Never load an arbitrary Unreal class path received from the network.

## Local Blueprint allowlist

The cloud Builder returns symbolic names such as `BP_FurnitureChair`. Unreal must map those symbolic names locally to class references. Do not call `LoadClass` using untrusted network strings.

Recommended Data Asset: `DA_InterVerseAssetRegistry`.

Initial mappings:

- `BP_WallBasic`
- `BP_FloorBasic`
- `BP_DoorInteractive`
- `BP_WindowBasic`
- `BP_FurnitureChair`
- `BP_FurnitureDesk`
- `BP_InteractiveScreen`
- `BP_SignInteractive`
- `BP_TreeOptimized`
- `BP_LightOptimized`
- `BP_NavigationPoint`

## Navigation

Create tagged navigation actors:

- `NAV_Entrance`
- `NAV_Reception`
- `NAV_NorthHallway`
- `NAV_Classroom101`

The first test phrase is: `Llévame al Salón 101`.

## Quest safety

All network calls use HTTPS. OpenAI credentials remain in `interverse-api`; no model key belongs in Unreal or the Android package.
