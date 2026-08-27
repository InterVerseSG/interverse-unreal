# InterVerseSG — Extrusión 3D de edificios

Esta fase convierte footprints verificados de OpenStreetMap en volúmenes 3D ligeros para validar el gemelo digital del Recinto de San Germán en Unreal Engine y Meta Quest.

## 1. Construir la geometría geoespacial

Desde la raíz del repositorio:

```powershell
python Scripts/build_campus_geometry.py
```

El pipeline produce:

- `Data/campus_geometry.geojson`
- `Config/InterVerseCampusGeometry.local.json`

No se inventan footprints. Los `way` se descargan desde OpenStreetMap usando los `osm_id` registrados en `Data/campus_geometry_sources.json`.

## 2. Abrir/compilar Unreal Engine 5.8

Abrir:

`InterVerseSG.uproject`

El proyecto habilita:

- OpenXR
- Python Editor Script Plugin
- ProceduralMeshComponent

Si Unreal solicita compilar los módulos C++, aceptar la compilación.

## 3. Generar el nivel

En Unreal Editor, abrir **Window → Output Log**, cambiar la consola a Python y ejecutar:

```python
exec(open(unreal.Paths.project_dir() + "Scripts/bootstrap_interverse_level.py", encoding="utf-8").read())
```

El bootstrap crea/actualiza:

- `LV_InterVerse_SanGerman`
- anclas `NAV_...`
- `IV_CampusGeometry` para contornos
- `IV_CampusBuildings` para extrusión 3D

## 4. Política de altura

`AInterVerseBuildingExtrusionActor` usa la siguiente prioridad:

1. etiqueta OSM `height`, si existe;
2. etiqueta OSM `building:levels × 3 m`;
3. valor temporal `4 m` cuando OSM no publica altura.

El valor temporal sirve solo para validar volumen y navegación. No representa la altura arquitectónica real del edificio.

## 5. Filtro de seguridad geoespacial

Solo se extruyen features que tengan una etiqueta OSM `building` distinta de `no`.

Esto evita extruir como edificio:

- pista atlética;
- perímetro general del campus;
- carreteras;
- senderos;
- estacionamientos.

## 6. Optimización inicial para Meta Quest

- `bCreateCollision = false` por defecto para la maqueta.
- una sección de ProceduralMesh por footprint;
- sin materiales complejos ni texturas de alta resolución;
- sin interiores durante esta fase;
- no usar la extrusión procedural como arte final.

Después de validar el mapa, los edificios prioritarios deberán convertirse en Static Meshes optimizados, con LOD/HLOD y materiales móviles.

## 7. EEGEI

La Escuela de Estudios Graduados e Investigación ya posee punto georreferenciado y ancla `NAV_EscuelaGraduada`. Actualmente no se extruye como edificio porque todavía no se ha verificado un footprint OSM específico para EEGEI. Esto es intencional: no se generará una forma arquitectónica inventada.
