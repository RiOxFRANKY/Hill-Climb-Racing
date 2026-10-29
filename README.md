# Hill Climb Racing

A hill-climb driving game written in C++17 and Qt Quick, for desktop and
Android. The game lays out a fixed **1920 x 1080** scene and scales it to fit
the window or screen, keeping its 16:9 shape.

The simulation (`src/core`, `src/physics`, `src/vehicle`, `src/terrain`) is
plain C++. `GameController` (`src/quick`) exposes it to the QML scene in
`qml/`, and `TerrainItem` draws the pixel-art ground as a GPU texture.

## Course

The track is the 10 km "Grassland Gauntlet" from `data/terrain_10km.npz`
(10 ground chains, 9 ravines, 10 sections, 80 px per metre; see
`data/terrain_numpy_implementation.md`). The game embeds it as
`assets/terrain_10km.json`. After replacing the NPZ, regenerate the JSON with:

```powershell
python tools/export_terrain.py
```

## Controls

The game opens on a start menu. Use `Up`/`Down` (or `W`/`S`) and `Enter`, or
the mouse, to pick **Play**, **Controls** or **Quit**.

- `D` or `Right Arrow`: accelerate / rotate clockwise in the air
- `A` or `Left Arrow`: brake, reverse / rotate counter-clockwise in the air
- `Space`: boost while driving (uses more fuel)
- `P`: pause or resume
- `R`: restart
- `F11` or `Alt+Enter`: toggle fullscreen
- `Esc`: leave fullscreen; otherwise return to the start menu (quits from
  the menu)

On a touch screen, hold the on-screen **Brake**, **Boost** and **Gas** pedals
(several at once is fine), and tap **P Pause**, **R Restart** or **Esc Menu**
in the bar at the top. Android's Back button works like `Esc`.

Collect coins for points and red fuel cans to refill the tank. Reach the
finish line at 10,000 m.

## Build on this machine

The project builds with CMake and needs Qt 6.8 or newer with the Qt Quick
module. In Qt Creator, open `CMakeLists.txt`, pick a Qt 6 Desktop kit and press
**Run**.

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

Add `--play` to skip the start menu, for example to capture a gameplay frame
with `--play --screenshot preview.png`.

### Android

Install the Qt for Android kit with the Qt Maintenance Tool, and set up the
Android SDK, NDK and JDK under **Edit > Preferences > Devices > Android** in
Qt Creator. Then open `CMakeLists.txt`, choose the Android kit, and run on a
connected device or emulator. The game runs fullscreen and locks the screen to
landscape.
