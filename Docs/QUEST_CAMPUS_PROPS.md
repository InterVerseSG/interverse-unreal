# InterVerseSG — Perfil de mobiliario para Meta Quest

## Objetivo

Añadir contexto visual al Recinto de San Germán sin convertir cada banco,
luminaria o señal en un Actor separado. Los objetos repetitivos se renderizan
mediante `UHierarchicalInstancedStaticMeshComponent` (HISM).

## Fuente de datos

`Scripts/fetch_osm_campus_props.py` consulta únicamente objetos cartografiados
en OpenStreetMap:

- `amenity=bench`
- `highway=street_lamp`
- `tourism=information`
- `information=guidepost|map|board`

No se generan objetos aleatorios ni se infieren ubicaciones no presentes en OSM.

## Prioridad espacial

Los objetos situados hasta 90 m de estas anclas reciben prioridad visual:

- `NAV_EscuelaGraduada`
- `NAV_CAI`
- `NAV_CentroEstudiantes`

Esto no altera navegación ni coordenadas. Solo modifica la distancia de culling.

## Distancias iniciales

| Grupo | Start Cull | End Cull |
| --- | ---: | ---: |
| Estándar | 70 m | 180 m |
| Prioridad | 120 m | 300 m |

Ajustar después de profiling real en Quest; no aumentar distancias por estética
sin comprobar GPU/CPU y draw calls.

## Mesh recomendado

### Banco

- 300–800 triángulos para LOD0.
- Un solo material opaco.
- Sin colisión en el HISM visual.
- Si se necesita impedir que el usuario atraviese el banco, usar una colisión
  simple separada solo en áreas de interacción cercanas.

### Luminaria

- 300–700 triángulos.
- No colocar una `PointLight` dinámica por poste.
- Simular luminancia con material/emissive muy moderado si se necesita.
- Para una escena nocturna futura, usar pocas luces compartidas o iluminación
  baked, no una luz dinámica por instancia.

### Señal

- 100–400 triángulos.
- Un material atlas para múltiples diseños.
- Texto de alta resolución solo en señales que el usuario pueda leer de cerca.

## Sombras

Por defecto todos estos HISM usan `bCastShadows=false`. La silueta del campus y
los edificios tienen prioridad sobre sombras de mobiliario en Quest.

## Draw calls

No crear un Actor por objeto. La arquitectura actual agrupa por:

- categoría: banco / luminaria / señal;
- prioridad: estándar / prioridad.

Esto produce un máximo inicial de seis grupos HISM, independientemente del
número de objetos cartografiados.

## Integración

Cuando exista `Config/InterVerseCampusProps.local.json`, el bootstrap crea:

`IV_CampusProps`

Después deben asignarse en el Editor tres meshes móviles compartidos:

- `BenchMesh`
- `LampMesh`
- `SignMesh`

Hasta que esos assets existan, el actor puede estar presente sin generar
instancias visibles.
