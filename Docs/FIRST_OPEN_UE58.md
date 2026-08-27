# InterVerseSG — First open in Unreal Engine 5.8

## Prerequisites

- Unreal Engine 5.8 installed through Epic Games Launcher.
- Visual Studio 2022 with Desktop development with C++ and Game development with C++ workloads.
- Git installed.
- Optional for Quest testing later: Meta Quest Developer Mode and Android tooling.

## 1. Clone the project

```powershell
git clone https://github.com/InterVerseSG/interverse-unreal.git
cd interverse-unreal
```

## 2. Generate project files

Right-click `InterVerseSG.uproject` and choose **Generate Visual Studio project files**.

If Windows does not show that option, open the `.uproject` once with Unreal Engine 5.8; Unreal will prompt to build the C++ module.

## 3. Build the editor target

Open the generated solution and build:

`InterVerseSGEditor / Development Editor / Win64`

Then open `InterVerseSG.uproject`.

## 4. Confirm plugins

In **Edit > Plugins**, confirm:

- OpenXR: Enabled
- Python Editor Script Plugin: Enabled

Restart the editor if Unreal requests it.

## 5. Bootstrap the campus map

Open **Window > Output Log** and switch the command input to Python. Run:

```python
exec(open(unreal.Paths.project_dir() + "Scripts/bootstrap_interverse_level.py", encoding="utf-8").read())
```

The script will:

1. create/open `/Game/Maps/LV_InterVerse_SanGerman`;
2. add basic lighting;
3. read `Config/InterVerseCampusAnchors.json`;
4. create/update every verified `NAV_...` TargetPoint;
5. skip POIs with pending coordinates, including EEGEI until its position is verified;
6. save the level.

## 6. What you should see

In the World Outliner, search for `NAV_`. You should see the verified campus anchors, including CAI, Centro de Estudiantes, Carlos J. Torres, Marquis Science Hall, Pista Sambolín and others.

At this stage these are navigation reference points, not finished buildings. The next layer is terrain, roads, building footprints and Quest locomotion.

## Safety rule

Do not manually reposition generated actors and then rerun the generator expecting those edits to persist. Verified coordinates are authoritative and the script will update managed anchors from configuration.
