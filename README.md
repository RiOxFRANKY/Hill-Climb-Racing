# Hill Climb Racing 2D (Qt 6 & C++17)

A fully functional, desktop 2D retro physics-based **Hill Climb Racing** game engineered with **C++17** and **Qt 6 Widgets** (`QWidget` + `QPainter`). Built for high-performance desktop execution, crisp pixel art presentation, and clean architectural separation of concerns.

Compatible with **Qt 6 (specifically Qt 6.11.1)** using **MinGW 64-bit (GCC)** or **MSVC** on Windows, configurable out-of-the-box via both **CMake** and **qmake**.

---

## Table of Contents
- [Key Features](#key-features)
- [Controls](#controls)
- [Quick Start & Building](#quick-start--building)
  - [Method 1: Qt Creator (qmake)](#method-1-qt-creator-qmake---recommended)
  - [Method 2: Qt Creator (CMake)](#method-2-qt-creator-cmake)
  - [Method 3: PowerShell / Command Line](#method-3-powershell--command-line)
- [System Architecture](#system-architecture)
  - [Module A: Physics Simulation (`src/physics/`)](#module-a-physics-simulation-srcphysics)
  - [Module B: Procedural Terrain (`src/terrain/`)](#module-b-procedural-terrain-srcterrain)
  - [Module C: Vehicle Entity (`src/vehicle/`)](#module-c-vehicle-entity-srcvehicle)
  - [Module D: Game Orchestration (`src/core/`)](#module-d-game-orchestration-srccore)
  - [Module E: Qt Renderer & HUD (`src/render/`)](#module-e-qt-renderer--hud-srcrender)
- [Pixel Art Assets & Dimensions](#pixel-art-assets--dimensions)
- [Tweakable Parameters & Balancing](#tweakable-parameters--balancing)

---

## Key Features

- **Custom 2D Vehicle Suspension Solver**:
  - Independent front & rear spring-damper struts utilizing Hooke's Law ($F_s = -k\Delta x - c v$) with rigid lateral locking (eliminating horizontal wheel drifting or crossing).
  - Circle-to-segment collision detection with normal reaction forces and anti-tunneling chassis bumper probes.
  - Realistic normal-load traction coupling ($F_{\max} = \mu \cdot F_N$) with progressive throttle and dedicated gentle reverse gearing.
  - Zero-drift standstill static friction deadband when coasting to a complete stop.
  - In-air pitch stabilization (nose up with Gas, nose down with Brake).
- **Procedural Infinite Terrain & Gapless Tiling**:
  - Multi-octave harmonic sine wave profile with safe starting runway and progressive difficulty scaling.
  - Gapless along-slope grass tile chain ($L \cdot \cos\theta$) ensuring continuous end-to-end connections without cracks on steep inclines.
  - Parallax sky backdrop with $C^\infty$ mathematically seamless horizontal looping.
- **Retro Pixel Art Visuals & Uniform Density**:
  - Rendered to an internal **960 x 540** virtual canvas and scaled to Full HD (**1920 x 1080**) with `Qt::FastTransformation` for razor-sharp pixel edges.
  - All sprites match the native canvas 1:1 without arbitrary in-game downscaling.
  - **Graceful Procedural Fallback**: If any PNG asset is missing or fails to load, the renderer automatically draws crisp vector shapes, keeping the game 100% playable.
- **Interactive Retro HUD**:
  - Analog speedometer dial gauge with rotating needle pointer.
  - Color-shifting fuel gauge bar (Green $\to$ Amber $\to$ Flashing Red Warning).
  - Track distance progress bar, coin counters, and on-screen interactive pedal indicators.

---

## Controls

| Action | Primary Key | Secondary Key | Mouse / Touch |
| :--- | :--- | :--- | :--- |
| **Accelerate / Gas** | `D` | `Right Arrow` | Click on-screen **Green Pedal** (bottom-right) |
| **Brake / Reverse** | `A` | `Left Arrow` | Click on-screen **Red Pedal** (bottom-left) |
| **In-Air Tilt Nose Up** | `D` | `Right Arrow` | — |
| **In-Air Tilt Nose Down** | `A` | `Left Arrow` | — |
| **Pause / Resume** | `Esc` | `P` | — |
| **Restart Course** | `R` | — | Click banner prompt |

---

## Quick Start & Building

### Prerequisites
- **Framework**: Qt 6.5+ (Tested on **Qt 6.11.1**) with `Qt6Widgets`
- **Compiler**: MinGW 64-bit (GCC 11+) or MSVC 2019/2022
- **Build Tools**: CMake (>= 3.16) or qmake

---

### Method 1: Qt Creator (qmake) - Recommended
1. Launch **Qt Creator**.
2. Go to **File > Open File or Project...** (`Ctrl + O`).
3. Select `HillClimbGame.pro`.
4. Select your **Desktop Qt 6.11.1 MinGW 64-bit** kit and click **Configure Project**.
5. Press **Run** (`Ctrl + R`).

---

### Method 2: Qt Creator (CMake)
1. In Qt Creator, go to **File > Open File or Project...**.
2. Select `CMakeLists.txt`.
3. Select your **Desktop Qt 6.11.1 MinGW 64-bit** kit and click **Configure Project**.
4. Press **Run** (`Ctrl + R`).

---

### Method 3: PowerShell / Command Line

#### Building with CMake & MinGW:
```powershell
# Set path to Qt and MinGW (adjust paths to your local Qt installation if needed)
$env:Path = "C:\Qt\6.11.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;$env:Path"

# Configure and build
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Launch the game
./build/HillClimbGame.exe
```

#### Building with qmake & MinGW:
```powershell
$env:Path = "C:\Qt\6.11.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;$env:Path"

New-Item -ItemType Directory -Force build-qmake | Out-Null
Set-Location build-qmake
qmake.exe ..\HillClimbGame.pro
mingw32-make.exe -j8

# Launch the game
./release/HillClimbGame.exe
```

---

## System Architecture

The codebase strictly adheres to clean architectural separation across 5 distinct modules:

```
HillClimbRacing/
├── CMakeLists.txt                 # CMake configuration (Qt 6, C++17)
├── HillClimbGame.pro              # qmake configuration
├── resources.qrc                  # Qt Resource bundle embedding all starter PNGs
├── assets/                        # Structured pixel-art starter PNGs
│   ├── vehicle/                   # Chassis, wheel, driver head
│   ├── terrain/                   # Repeating grass top and tiled dirt fill
│   ├── collectibles/              # Coin, fuel can
│   ├── ui/                        # Pedals, meter dial, indicator needle
│   └── backgrounds/               # Seamless 16:9 parallax sky backdrop
└── src/
    ├── main.cpp                   # Application entry point (QApplication & GameWidget)
    ├── physics/                   # Module A: Physics Abstraction & Solver
    ├── terrain/                   # Module B: Procedural Profile & Tile Geometry
    ├── vehicle/                   # Module C: Vehicle Entity & Fuel Logic
    ├── core/                      # Module D: Loop Orchestration & Input Handler
    └── render/                    # Module E: QPainter Canvas & Sprite Management
```

### Module A: Physics Simulation (`src/physics/`)
- **`PhysicsTypes.h`**: Exposes strictly pure C++ mathematical structs (`Vec2`, `Transform2D`, `VehiclePhysicsState`, `TerrainSegment`) with zero external/Qt dependencies.
- **`IPhysicsWorld.h`**: Abstract pure virtual facade interface defining physics lifecycle (`step`, `setThrottle`, `addTerrainSegment`, `getVehicleState`, `reset`).
- **`PhysicsWorld.h` / `PhysicsWorld.cpp`**: 
  - Complete 2D spring-damper suspension solver running with sub-stepping (16 sub-steps per frame) for rock-solid numerical stability.
  - Rigid strut constraint locks wheel lateral motion to suspension axis ($\Delta x_{\text{lateral}} = 0$).
  - Dynamic wheel ground collision, traction coupling, and neck-snap head collision detection.

### Module B: Procedural Terrain (`src/terrain/`)
- **`TerrainTypes.h`**: Node definitions (`TerrainNode`), surface enums (`SurfaceType`), and tile connection descriptors (`GrassTileSegment`).
- **`ITerrainGenerator.h`**: Interface for mathematical elevation generation.
- **`SineTerrainGenerator.h` / `.cpp`**: Multi-octave trigonometric hill generator ($h(x) = \text{baseY} - \sum A_k \sin(\omega_k x)$) with smoothstep flat runway blending and progressive difficulty scaling.
- **`TerrainManager.h` / `.cpp`**:
  - Manages full course node array with $O(1)$ height queries: `index = floor(x / dx)`.
  - Feeds local collision segments within a radius of the vehicle to `PhysicsWorld`.
  - Precomputes gapless along-slope grass tile chain ($L = \sqrt{\Delta x^2 + \Delta y^2} \equiv 32\text{ px}$) so tiles never gap or swim.

### Module C: Vehicle Entity (`src/vehicle/`)
- **`Car.h` / `Car.cpp`**:
  - Encapsulates fuel level ($0.0 \to 100.0$), distance traveled, forward speed in km/h, and air-time metrics.
  - Disables drive power when fuel reaches zero.
  - Outputs lightweight `CarRenderData` descriptor consumed by the renderer.

### Module D: Game Orchestration (`src/core/`)
- **`GameTypes.h`**: `GameState` enum (`Playing`, `Paused`, `GameOver_OutOfFuel`, `GameOver_HeadCrash`, `LevelWon`) and `Collectible` definitions.
- **`InputHandler.h` / `.cpp`**: Thread-safe key state tracker filtering OS autorepeat events and syncing keyboard and on-screen mouse pedal touch states.
- **`GameEngine.h` / `.cpp`**:
  - Master simulation loop running on a fixed 60 Hz tick rate.
  - Spawns collectible coins along hill crests and red fuel cans at regular intervals.
  - Evaluates win condition ($1000\text{m}$ finish line) and loss conditions (neck crash or fuel exhaustion).
  - Smooth lookahead camera tracking with velocity-biased horizontal lead.

### Module E: Qt Renderer & HUD (`src/render/`)
- **`SpriteManager.h` / `SpriteManager.cpp`**:
  - Manages asset loading from compiled Qt binary resources (`:/assets/...`) with fallback search paths on the local filesystem.
  - Caches `QPixmap` assets.
- **`GameWidget.h` / `GameWidget.cpp`**:
  - Subclasses `QWidget` and hosts the 60 FPS `QTimer`.
  - Renders to an offscreen `QImage(960, 540)` virtual buffer, blitted to the window with `Qt::FastTransformation` for integer-scaled pixel art.
  - Performs viewport culling on grass tiles, parallax sky backdrop scrolling, terrain dirt fill, vehicle components, and HUD overlay.

---

## Pixel Art Assets & Dimensions

All assets are located under `assets/` and compiled into `resources.qrc`. In accordance with the **Uniform Pixel Density Rule**, each sprite must strictly maintain its exact native resolution:

| Category | File Path | Resolution | Aspect Ratio | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Vehicle** | `assets/vehicle/chassis.png` | 96 x 48 px | 2:1 | Retro red off-roader chassis with roll cage & cabin |
| **Vehicle** | `assets/vehicle/wheel.png` | 32 x 32 px | 1:1 | Deep-tread tire with silver alloy rim & lug nuts |
| **Vehicle** | `assets/vehicle/driver_head.png` | 24 x 24 px | 1:1 | Helmeted driver head with tinted goggles |
| **Terrain** | `assets/terrain/grass_top.png` | 32 x 16 px | 2:1 | Seamless horizontal repeating top-edge grass trim |
| **Terrain** | `assets/terrain/dirt_fill.png` | 32 x 32 px | 1:1 | Seamless 2D tiling subterranean dirt texture |
| **Collectibles** | `assets/collectibles/coin.png` | 24 x 24 px | 1:1 | Golden coin with star stamp and sparkle shine |
| **Collectibles** | `assets/collectibles/fuel_can.png` | 24 x 30 px | 4:5 | Classic red military Jerrycan with carry handle |
| **UI** | `assets/ui/pedal_gas_released.png` | 36 x 60 px | 3:5 | Green metallic gas pedal (unpressed) |
| **UI** | `assets/ui/pedal_gas_pressed.png` | 36 x 60 px | 3:5 | Green metallic gas pedal (depressed) |
| **UI** | `assets/ui/pedal_brake_released.png` | 36 x 60 px | 3:5 | Red ribbed brake pedal (unpressed) |
| **UI** | `assets/ui/pedal_brake_pressed.png` | 36 x 60 px | 3:5 | Red ribbed brake pedal (depressed) |
| **UI** | `assets/ui/meter_dial.png` | 64 x 64 px | 1:1 | Round speedometer dial gauge |
| **UI** | `assets/ui/dial_needle.png` | 8 x 32 px | 1:4 | Pivot-centered needle pointer (pivot at x=4, y=28) |
| **Background** | `assets/backgrounds/sky_bg.png` | 960 x 540 px | 16:9 | Harmonic seamless parallax sky with mountain ridge |

> **Asset Customization Note**: Any replacement PNG images placed in `assets/` must match the exact dimensions listed above to prevent texture warping, collision offset errors, or alignment seams.

---

## Tweakable Parameters & Balancing

Every class exposes its physical constants and visual tunables inside a `struct Config` located at the very top of its header file:

- **Physics Tuning** ([`src/physics/PhysicsWorld.h`](src/physics/PhysicsWorld.h)):
  - `gravity`: $980.0\text{ px/s}^2$
  - `chassisMass`: $90.0\text{ kg}$, `chassisInertia`: $65,000.0$ (prevents unwanted toppling)
  - `springK`: $18,000.0\text{ N/m}$, `dampingC`: $1,400.0$ (stiff, responsive suspension)
  - `restLength`: $14.0\text{ px}$, `minLength`: $8.0\text{ px}$, `maxLength`: $18.0\text{ px}$ (strict travel limits)
  - `driveTorque`: $950.0\text{ N}\cdot\text{m}$ (gradual forward acceleration)
  - `reverseTorque`: $550.0\text{ N}\cdot\text{m}$ (gentle, steady reverse control)
  - `brakeTorque`: $1,600.0\text{ N}\cdot\text{m}$ (responsive forward stopping)
  - `tireFriction`: $1.75$ (high-traction grip)
  - `groundStabilizer`: $18.0$ (natural wheel-plane slope alignment)
- **Terrain Tuning** ([`src/terrain/SineTerrainGenerator.h`](src/terrain/SineTerrainGenerator.h)):
  - `baseY`: $380.0\text{ px}$
  - `startFlatLength`: $300.0\text{ px}$
  - Amplitudes and harmonic frequencies for long rolling hills, steep crests, and ripples.
- **Vehicle Fuel & Gameplay** ([`src/vehicle/Car.h`](src/vehicle/Car.h)):
  - `maxFuel`: $100.0$
  - `idleFuelBurnRate`: $0.45\%/\text{s}$
  - `gasFuelBurnRate`: $2.20\%/\text{s}$
  - `targetDistanceMeters`: $1,000.0\text{ m}$
