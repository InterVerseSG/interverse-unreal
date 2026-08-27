# InterVerseSG — Meta Quest / Android: primer APK

## Estado

El proyecto ya contiene:

- OpenXR habilitado.
- Enhanced Input habilitado.
- Pawn XR (`AInterVerseXRPawn`).
- locomoción suave, snap turn y teleport parabólico.
- arco/indicador visual de teleport.
- navegación por `NAV_*` y guía con distancia.
- llamadas HTTPS a InterVerse API y Builder.

## Toolchain verificado para Unreal Engine 5.8

Usar el flujo de SDK Management / Turnkey de UE 5.8 siempre que sea posible.

- Android target SDK: 35
- Android minimum install SDK: 26
- NDK: r27c
- Build-tools: 35.0.1
- Java: OpenJDK 21.0.3
- arquitectura: arm64

El repositorio configura `TargetSDKVersion=35` y `MinSDKVersion=26`.

## 1. Instalar Unreal Engine 5.8

Instalar UE 5.8 desde Epic Games Launcher y abrirlo al menos una vez.

## 2. Instalar Android mediante Turnkey

En Unreal:

`Platforms > SDK Management > Android > Install SDK`

Seguir el instalador y reiniciar sesión de Windows cuando termine para que las variables de entorno queden actualizadas.

Después regresar a:

`Platforms > SDK Management > Android`

El SDK debe mostrarse válido.

## 3. Instalar Meta XR Plugin

Meta documenta el Meta XR Plugin como parte del entorno requerido para desarrollar para Meta Horizon OS / Quest.

Instalar una versión del Meta XR Plugin compatible con la versión de Unreal utilizada. No añadir manualmente `OculusXR`/`MetaXR` al `.uproject` antes de que el plugin exista en la instalación, ya que Unreal puede rechazar la apertura del proyecto si falta un plugin requerido.

Después de instalarlo:

1. Abrir `InterVerseSG.uproject`.
2. `Edit > Plugins`.
3. Buscar Meta XR / OculusXR.
4. Habilitar el plugin recomendado por Meta para la versión instalada.
5. Reiniciar Unreal.
6. Mantener OpenXR habilitado.

## 4. Preparar Meta Quest

En el visor/cuenta Meta:

1. Tener una organización de desarrollador configurada.
2. Activar Developer Mode para el visor.
3. Conectar el Quest por USB-C.
4. Aceptar el diálogo de depuración USB dentro del visor.
5. Confirmar que Unreal/ADB reconoce el dispositivo.

## 5. Construir datos del campus

Desde la raíz del repositorio:

```powershell
python Scripts/build_campus_terrain.py
python Scripts/build_campus_geometry.py
```

Esto genera terreno, alturas de NAV, footprints, edificios, caminos y superficies locales.

## 6. Generar el nivel

Abrir Unreal y ejecutar desde la consola Python:

```python
exec(open(unreal.Paths.project_dir() + "Scripts/bootstrap_interverse_level.py", encoding="utf-8").read())
```

Debe existir `LV_InterVerse_SanGerman` con:

- `IV_CampusTerrain`
- `IV_CampusBuildings`
- `IV_CampusSurfaces`
- `IV_XRPawn`
- `NAV_*`

## 7. Pruebas antes del APK

En Play In Editor verificar:

1. WASD: movimiento.
2. Q/E: snap turn.
3. T: teleport de prueba.
4. arco visible durante teleport.
5. `Navigation->StartGuidanceToAnchor("NAV_EscuelaGraduada")`: flecha + distancia.
6. `Navigation->NavigateToAnchor("NAV_EscuelaGraduada")`: movimiento directo.

Después validar Touch controllers en Quest:

- stick izquierdo: movimiento
- stick derecho X: snap turn
- trigger derecho: apuntar/confirmar teleport

## 8. Empaquetar Development APK

En Unreal:

`Platforms > Android`

Verificar que Android SDK aparezca válido y luego empaquetar inicialmente como Development para pruebas internas.

El package name configurado es:

`com.interversesg.interverse`

## 9. Prueba de IA

Una vez instalada la app:

1. confirmar conectividad HTTPS;
2. ejecutar `AskAssistant()` con "Llévame a la Escuela Graduada";
3. la cadena esperada es:

`Quest -> interverse-api -> Gemini -> interverse-builder -> NAV_EscuelaGraduada -> Unreal`.

## Restricciones actuales

- Las fachadas todavía no son modelos arquitectónicos finales.
- Varias alturas usan datos OSM cuando existen y placeholders cuando faltan.
- El Meta XR Plugin debe instalarse y compilarse localmente antes del primer build Quest.
- La primera compilación local en UE 5.8 es necesaria para descubrir cualquier cambio de API específico de la versión.
