# Hill Climb Racing

A hill-climb driving game written in C++17 and Qt Quick, for desktop and
Android. The game lays out a fixed **1920 x 1080** scene and scales it to fit
the window or screen, keeping its 16:9 shape.

The simulation (`src/core`, `src/physics`, `src/vehicle`, `src/terrain`) is
plain C++. `GameController` (`src/quick`) exposes it to the QML scene in
`qml/`, and `TerrainItem` draws the pixel-art ground as a GPU texture.

## Course

The track is a 10 km course built by `src/terrain/procedural.cpp`. The ground
is a function of distance made from three layers of domain-warped sine waves:
long swells, rolling hills and short bumps. Ten zones (Green Valley through
Home Stretch) each set their own amplitudes, wavelengths and gradient, and
neighbouring zones blend over 120 m. The course gets rougher the further you
drive. On top of the waves, each zone places its own mix of set pieces:
big hills (often steeper on one side), flat-topped mesas, deep valleys, runs
of rollers, staircases of ledges and kicker ramps. Nine ravines have a flat
run-up, a takeoff lip and a downhill landing. The ground is sampled every
0.25 m into the same chains the renderer and physics already use. A seed
shifts every wave and picks every set piece, so each seed is a different
course and the same seed is always the same course.

```powershell
.\build\HillClimbQt.exe --seed 42
```

To try a later zone straight away, start part-way along the course:

```powershell
.\build\HillClimbQt.exe --start-at 6900
```

The hand-made "Grassland Gauntlet" course is still available:

```powershell
.\build\HillClimbQt.exe --course :/assets/terrain_10km.json
```

After replacing `data/terrain_10km.npz`, regenerate its JSON with
`python tools/export_terrain.py`.

## Controls

The game opens on a start menu. Use `Up`/`Down` (or `W`/`S`) and `Enter`, or
the mouse, to pick **Play**, **Garage**, **Controls** or **Quit**. **Play**
leads to a mode (**Normal**, **Rescue**, **Race**) and then, for Normal, a map:
**Grazing the Grass**, **Beachday**, **Swampy Vibe**, **Arctic Elegance** or
**The Real Excursion**. All five maps can be played in Normal mode; Garage,
Rescue and Race are marked *SOON*. `Esc` steps back a page.

Each map is harder than the one above it. They share the same artwork, but
each has its own zone names and course, and the ground gets bigger and
bumpier, set pieces get larger, steeper and closer together (with more ledges
and kicker ramps), the climbs get longer, and ravines become more frequent
and wider: 9 jumps up to 11 m on the first two maps, 17 on the next two, and
25 up to about 14 m on The Real Excursion. To start straight on a map, pass
its number (1 to 5):

```powershell
.\build\HillClimbQt.exe --map 5
```

- `D` or `Right Arrow`: accelerate / rotate clockwise in the air
- `A` or `Left Arrow`: brake, reverse / rotate counter-clockwise in the air
- `Space`: boost while driving (uses more fuel)
- `P`: pause or resume
- `R`: restart
- `F11` or `Alt+Enter`: toggle fullscreen
- `Esc`: leave fullscreen; otherwise return to the start menu (steps back a
  page in the menu, and quits from its first page)

On a touch screen, hold the on-screen **Brake**, **Boost** and **Gas** pedals
(several at once is fine), and tap **P Pause**, **R Restart** or **Esc Menu**
in the bar at the top. Android's Back button works like `Esc`.

The engine burns fuel the whole time you race: slowly when idling, faster on
the gas and fastest while boosting. A full tank lasts about 42 s on the gas,
roughly 580 m of flat ground at the car's usual speed. Red fuel cans refill
it. Their spacing follows the terrain: a can comes sooner after climbs and
bumpy stretches and later after downhills, so on the gas you reach each one
with about 10-20% of the tank left. Cans and coins are always in the same
places on a given course.

Coins come in groups of 5 to 8, worth 5, 25, 100 or 500 each, in a row that
follows the ground. Bigger values turn up further along the course, and the
larger coins show their value. The
coin counter is the total value collected, and it adds to your score along
with 2.5 points per metre. Reach the finish line at 10,000 m.

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
