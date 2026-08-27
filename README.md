# InterVerse Unreal

Unreal Engine / Meta Quest client for the InterVerseSG immersive campus platform.

## Architecture

The Quest client never stores an OpenAI key. Runtime flow:

1. Unreal sends natural-language intent to `interverse-api`.
2. The AI service returns a structured command.
3. Unreal sends that command to `interverse-builder` for validation.
4. Only accepted commands are executed against an allowlisted Unreal Blueprint class.

## Target

- Unreal Engine 5.7 baseline
- Meta Quest 3 / Quest 3S
- OpenXR + Meta XR
- Android standalone build
- Blueprint-first integration with a small C++ runtime plugin

## Runtime configuration

Copy `Config/InterVerseRuntime.example.ini` to the project configuration and replace the service URLs with the deployed Render URLs.

Do not put API secrets in the Unreal project, APK, or repository.

## First vertical slice

`Ask -> Validate -> Execute`:

- "Llévame al Salón 101" -> navigation target
- "Coloca 10 sillas en el Salón 101" -> `BP_FurnitureChair`
- destructive actions -> confirmation required
