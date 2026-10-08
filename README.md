# Hill Climb Racing

A hill-climb driving game written in C++17 and Qt 6 Widgets. The game renders a
fixed **1920 x 1080** scene and scales it to fit the window or screen, keeping
its 16:9 shape.

## Course

The track is the 10 km "Grassland Gauntlet" from `data/terrain_10km.npz`
(10 ground chains, 9 ravines, 10 sections, 80 px per metre; see
`data/terrain_numpy_implementation.md`). The game embeds it as
`assets/terrain_10km.json`. After replacing the NPZ, regenerate the JSON with:

```powershell
python tools/export_terrain.py
```

## Controls

- `D` or `Right Arrow`: accelerate / rotate clockwise in the air
- `A` or `Left Arrow`: brake, reverse / rotate counter-clockwise in the air
- `Space`: boost while driving (uses more fuel)
- `P`: pause or resume
- `R`: restart
- `F11` or `Alt+Enter`: toggle fullscreen
- `Esc`: leave fullscreen, or quit when windowed

Collect coins for points and red fuel cans to refill the tank. Reach the
finish line at 10,000 m.

## Build on this machine

### Qt Creator / qmake

Open `Hill-Climb-Racing.pro` in Qt Creator, select the installed Qt 6 Desktop
kit, and press **Run**.

From PowerShell, the equivalent qmake commands are:

```powershell
New-Item -ItemType Directory -Force build-qmake | Out-Null
Set-Location build-qmake
& "C:\Qt\6.11.1\mingw_64\bin\qmake.exe" ..\Hill-Climb-Racing.pro
& "C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe"
```

### CMake

From PowerShell in the project directory:

```powershell
$env:Path = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\CMake_64\bin;$env:Path"
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\mingw_64"
& "C:\Qt\Tools\CMake_64\bin\cmake.exe" --build build
```

Run:

```powershell
.\build\HillClimbQt.exe
```

To capture a preview frame without playing:

```powershell
.\build\HillClimbQt.exe --screenshot preview.png
```
