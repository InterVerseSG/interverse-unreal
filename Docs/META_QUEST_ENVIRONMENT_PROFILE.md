# InterVerseSG — Perfil de entorno para Meta Quest

Este perfil define la base de rendimiento para el gemelo digital del Recinto de San Germán en Meta Quest. Se aplica primero a Quest 3 / Quest 3S y mantiene compatibilidad razonable con Quest 2 / Quest Pro mientras se valida el primer build real.

## Renderer base

- Unreal Engine 5.8.
- Android ARM64.
- Vulkan móvil; no Vulkan SM5.
- Mobile Forward (`r.Mobile.ShadingPath=0`).
- Mobile HDR desactivado.
- MSAA 4x.
- Instanced Stereo activado.
- Mobile Multi-View activado.
- Bloom, motion blur, ambient occlusion, SSR, depth of field y lens flare desactivados en el perfil base.

## Principio de diseño

InterVerseSG debe privilegiar frame time estable, claridad visual y confort VR sobre efectos de render costosos. La arquitectura real del campus debe representarse primero con formas, escala, señalización, materiales simples y navegación correcta. Los detalles decorativos se añaden después de medir en dispositivo.

## Geometría

- Usar footprints reales como base.
- Evitar subdivisión innecesaria de fachadas.
- Crear LOD para edificios que reciban modelos detallados.
- Agrupar geometría que comparta material cuando sea posible para reducir draw calls.
- Evitar colisión compleja por triángulo para edificios decorativos.
- Mantener colisión solo en pisos, escaleras, rampas, bordes necesarios y elementos interactivos.
- El actor procedural de edificios mantiene colisión desactivada por defecto hasta que se definan meshes finales de navegación.

## Materiales

- Preferir materiales opacos y simples.
- Evitar translucencia extensa en cristales; usar aproximaciones opacas o masked cuando sea viable.
- Mantener pocas variantes de materiales institucionales compartidos.
- Evitar materiales con múltiples muestras de textura, parallax, displacement o lógica procedural pesada.
- Preparar atlas para señalización y elementos repetitivos cuando se incorporen fachadas finales.

## Texturas

- No usar texturas 4K como valor predeterminado.
- 2048 px solo para elementos que realmente lo justifiquen a corta distancia.
- 1024 px o menos para la mayoría de superficies del campus.
- Usar mipmaps y texture streaming.
- Revisar ASTC al preparar el APK de Quest.

## Iluminación

- Favorecer iluminación estática/baked cuando los assets finales dispongan de UV de lightmap.
- Reducir luces dinámicas con sombras.
- Evitar múltiples point/spot lights con shadow casting.
- El prototipo procedural debe funcionar correctamente incluso con iluminación simple sin sombras dinámicas costosas.

## Vegetación

- Instanced Static Mesh / Hierarchical Instanced Static Mesh para árboles y objetos repetidos.
- Foliage density inicial moderada.
- LOD agresivo para vegetación distante.
- Evitar transparencias complejas y overdraw excesivo en hojas.

## Campus y visibilidad

El recinto debe dividirse lógicamente por sectores para que el dispositivo no procese todo con el mismo nivel de detalle. Las áreas prioritarias son:

1. Entrada / Marquis Science Hall.
2. CAI.
3. Centro de Estudiantes.
4. Escuela de Estudios Graduados e Investigación (EEGEI).
5. Edificios académicos principales.

Los edificios lejanos pueden usar proxies/LOD simplificados hasta que el usuario se acerque.

## Locomoción y confort

- Teleport disponible en todo momento.
- Snap turn como opción principal.
- Smooth locomotion opcional.
- Nada de aceleraciones bruscas de cámara.
- La navegación de IA debe orientar o teletransportar sin modificar la orientación de cabeza del HMD de forma artificial.
- Las rutas deben utilizar `NAV_*` y la red peatonal validada del campus.

## Foveated rendering

Meta Quest soporta Fixed Foveated Rendering (FFR), pero su configuración específica se integrará cuando Meta XR Plugin esté instalado y comprobado con la versión exacta de Unreal 5.8. No se añade una dependencia Meta XR al `.uproject` hasta verificar que el plugin existe en el entorno de compilación.

## Perfil inicial de escalabilidad

`DefaultScalability.ini` contiene un perfil conservador para:

- distancia de visión;
- sombras;
- efectos;
- foliage;
- texturas.

Estos valores son punto de partida. La decisión final se tomará mediante profiling en Quest, no mediante calidad visual observada únicamente en PC.

## Regla para aceptar una nueva función visual

Antes de añadir una función costosa al entorno final debe responder afirmativamente:

1. ¿Aporta información, orientación o inmersión perceptible en el visor?
2. ¿Existe una alternativa más barata visualmente equivalente?
3. ¿Se puede desactivar mediante calidad/LOD?
4. ¿Se ha medido en Quest?

Si no se ha medido todavía, la función debe permanecer opcional.

## Próxima validación física

Cuando exista acceso a una estación con Unreal Engine 5.8 y Android/Quest configurado:

1. Compilar Development Editor Win64.
2. Verificar shaders Mobile Forward.
3. Probar VR Preview/OpenXR.
4. Generar Android ASTC Development.
5. Ejecutar en Quest 3/3S.
6. Medir frame time CPU/GPU y draw calls.
7. Activar Meta XR Plugin compatible.
8. Evaluar FFR y, solo si es necesario, Application SpaceWarp.
