# Implement this 10 km terrain from NumPy

Use the attached `terrain_10km.npz` as the authoritative terrain for the existing Qt hill-climb game. Read the arrays directly with Python/NumPy. Do not use OCR or recreate the surface from a picture.

This file preserves the previous course exactly: **39,490 solid points**, **10 separate ground chains**, **39,480 collision edges**, and **9 ravines**. The terrain was converted from its original JSON coordinates without regenerating or resampling it.

## Load the geometry

```python
import json
import numpy as np

with np.load("terrain_10km.npz", allow_pickle=False) as data:
    points_m = data["points_m"].copy()       # (39490, 2), x right / height up
    points_px = data["points_px"].copy()     # (39490, 2), world x right / y down
    offsets = data["chain_offsets"].copy()  # (11,), half-open slice boundaries
    edges = data["edges"].copy()            # (39480, 2), indices into points
    scale = float(data["pixels_per_metre"])
    schema = json.loads(data["schema_json"].item())
    metadata = json.loads(data["source_metadata_json"].item())

chains_m = [points_m[a:b] for a, b in zip(offsets[:-1], offsets[1:])]
chains_px = [points_px[a:b] for a, b in zip(offsets[:-1], offsets[1:])]
collision_edges_m = points_m[edges]  # (39480, 2, 2): [edge, endpoint, coordinate]
```

Only NumPy and the Python standard library are needed to load or export this file. No pickle, SciPy, Matplotlib, image-generation tool or image analysis is required.

## Coordinate contract

- Course length is **10,000 m of horizontal progress**, from x=0 to x=10000.
- Rendering scale is **80 pixels per metre on both axes**. Total width is **800,000 world pixels**.
- A 1920 x 1080 viewport covers **24 x 13.5 metres** at this scale.
- `points_m[:,0]` is x to the right. `points_m[:,1]` is terrain height upward.
- `points_px` is already converted to a drawing system with Y increasing downward. Its Y values can be negative, which is valid for world coordinates.
- To draw: `screen_points = points_px - [camera_x_px, camera_y_px]`.
- If rendering directly from metres: `world_x_px = 80*x_m`, `world_y_px = -80*height_m`.
- Suggested car length: **4.5 m / 360 px**. Suggested wheel radius: **0.5 m**, giving an **80 px diameter**.
- Samples are **0.25 m apart horizontally**, or **20 world pixels**. They have not been smoothed or resampled during conversion.
- Spawn x is **10 m** within an initial **25 m flat region**. The last **50 m** is flat, with the finish at **10000 m**.

## Collision and gap rules

1. Use `edges` as the explicit valid edge list, or create one independent collision chain for each range in `chain_offsets`.
2. **Never connect the last point of one chain to the first point of the next.** They are separated by a ravine. Concatenating all points into a single polyline would create invisible bridges.
3. Edge indices refer to `points_m` / `points_px`, not the uniform `grid_*` arrays.
4. `gap_ranges_m` stores the takeoff and landing x coordinates. There is no ground strictly between them; both lip endpoints are solid.
5. The uniform grid has **40,001 samples**. `grid_height_m` is `NaN` and `grid_solid` is false inside the gaps. Interpolate heights within one solid chain only. Do not remove NaNs and interpolate across the resulting hole.
6. Keep physics units consistent. If the physics engine uses metres, pass `points_m` and convert only for rendering. If existing custom physics uses pixels, use a consistent pixel-space conversion for all objects and forces.
7. Draw the brown ground and grassy surface from the same coordinates used for collision. Render the separate sky-and-hills background behind the terrain.
8. Keep source coordinates as floating point. Round only at rasterization if needed. For performance, draw/collide with nearby portions rather than allocating one 800,000-pixel-wide image.

## Arrays

| Array | Shape | Meaning |
|---|---|---|
| `points_m` | `(39490, 2)` | All solid ground points as [x_m, height_m], with positive height upward. No NaNs. Concatenated chains; do not join the chains. |
| `chain_offsets` | `(11,)` | Half-open slice boundaries into points_m and points_px. Chain i is points[chain_offsets[i]:chain_offsets[i+1]]. |
| `edges` | `(39480, 2)` | Valid collision edge pairs. Each row contains two indices into points_m / points_px, NOT into the dense grid arrays. No edge crosses a ravine. |
| `points_px` | `(39490, 2)` | Ready-to-draw world coordinates [80*x_m, -80*height_m]. Y already points downward. Subtract the camera position; do not flip Y again. |
| `grid_x_m` | `(40001,)` | Uniform horizontal samples from 0 through 10000 m, inclusive, every 0.25 m. |
| `grid_height_m` | `(40001,)` | Height on the uniform grid. NaN strictly inside each ravine. Finite at the takeoff and landing endpoints. |
| `grid_solid` | `(40001,)` | True where grid_height_m is finite. Do not interpolate across a False interval. |
| `gap_ranges_m` | `(9, 2)` | Each row is [takeoff_x_m, landing_x_m]; the open interval between these endpoints has no ground. |
| `section_bounds_m` | `(10, 2)` | Ten [start_x_m, end_x_m] ranges. The last endpoint is exactly 10000 m. |
| `section_names` | `(10,)` | Names corresponding to section_bounds_m, stored as fixed Unicode strings, not Python objects. |
| `pixels_per_metre` | `()` | Uniform render scale on both axes: 80. |
| `length_m` | `()` | Horizontal distance from start to finish: 10000 m, not surface arc length. |
| `sample_step_m` | `()` | Horizontal sampling interval: 0.25 m. |
| `viewport_px` | `(2,)` | Reference viewport [width,height] in pixels: [1920,1080]. |
| `spawn_x_m` | `()` | Suggested spawn horizontal position: 10 m. |
| `finish_x_m` | `()` | Finish horizontal position: 10000 m. |

`schema_json` and `source_metadata_json` are Unicode scalar arrays containing JSON strings, not object arrays. The latter retains section descriptions, obstacle definitions, lip heights, slope statistics and the original validation limitations.

## Export for C++ Qt or a coding assistant that prefers text

C++ Qt does not need to parse NPZ. Run this once in Python to export the same independent chains as readable JSON, then integrate that JSON with the existing project:

```python
import json
import numpy as np

with np.load("terrain_10km.npz", allow_pickle=False) as data:
    pts = data["points_m"]
    offsets = data["chain_offsets"]
    payload = {
        "units": "metres",
        "height_positive_up": True,
        "pixels_per_metre": float(data["pixels_per_metre"]),
        "horizontal_length_m": float(data["length_m"]),
        "sample_step_m": float(data["sample_step_m"]),
        "gap_ranges_m": data["gap_ranges_m"].tolist(),
        "polylines_m": [
            pts[a:b].tolist() for a, b in zip(offsets[:-1], offsets[1:])
        ],
    }

with open("terrain_for_qt.json", "w", encoding="utf-8") as file:
    json.dump(payload, file, separators=(",", ":"), allow_nan=False)
```

## Instruction to the coding assistant

Integrate the supplied numerical terrain into my existing Qt hill-climb game. Preserve the course coordinates, the nine gaps, the 80 px/metre scale and the 1920 x 1080 reference viewport. Use the numerical ground for both rendering and collisions, with a camera that follows the car horizontally and vertically. Keep vehicle motion, suspension and wheel rotation driven by the existing physics. Identify the actual physics implementation in the project before adapting it. Do not invent terrain by OCR, regenerate it from an image, bridge the gaps, flip `points_px` vertically again, or treat these arrays as a sprite sheet. If the C++ project cannot read NPZ directly, export the JSON with the provided Python snippet and load the independent chains from that file.

## Verification limits

The NPZ was reloaded with `allow_pickle=False` and compared to the source coordinates point for point. All chains and collision edges preserve the source, and the NaN regions match the nine gaps. The original course has slopes up to about **59.4 degrees** and gap widths from **10 to 18 m**. It has **not been driven in your game physics**; traction, torque, suspension and jump handling still need playtesting.
