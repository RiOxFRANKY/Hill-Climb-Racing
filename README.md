# Hill Climb Racing

A small, asset-free hill-climb driving prototype written in C++17 and Qt 6 Widgets.
The game uses a fixed **1920 x 1080** render resolution, including on Windows
desktops that use display scaling.

## Controls

- `D` or `Right Arrow`: accelerate / rotate clockwise in the air
- `A` or `Left Arrow`: brake, reverse / rotate counter-clockwise in the air
- `Space`: boost while driving (uses more fuel)
- `P`: pause or resume
- `R`: restart
- `Esc`: quit

Collect coins for points and red fuel cans to refill the tank.

## Build on this machine

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
