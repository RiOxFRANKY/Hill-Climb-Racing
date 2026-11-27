#!/usr/bin/env python3
"""
process_advanced_assets.py
Processes:
1. 5 High-Definition Pixel-Art Map Badges (Grassland, Beach, Swamp, Snow, Moon)
   - Flood-fill removes white background to 100% transparent alpha
   - Tight-crops to badge border
   - Saves to assets/ui/map_icon_<theme>.png (128x128 and 256x256)
   - Saves to assets/<theme>/terrain.png and assets/terrain_<theme>.png
2. 4 Master Pixel-Art Backgrounds (Beach, Swamp, Snow, Moon)
   - Resizes each generated masterpiece to exact 1920x1080 (Lanczos)
   - Deploys full composite background.png
   - Precision splits into 3 parallax layers:
     * bg_sky.png (3840x1080 seamless)
     * bg_mountains.png (3840x1080 seamless, transparent above mountain skyline)
     * bg_hills.png (3840x1080 seamless, transparent above foreground hills/trees)
"""

import os
from collections import deque
import numpy as np
from PIL import Image, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS = os.path.join(ROOT, "assets")
BRAIN = r"C:\Users\USER\.gemini\antigravity\brain\41be61e2-179e-4a72-ac0d-a9841ff894c5"

ICON_FILES = {
    "grassland": "map_icon_grassland_1795729519826.jpg",
    "beach": "map_icon_beach_1795729528638.jpg",
    "swamp": "map_icon_swamp_1795729538499.jpg",
    "snow": "map_icon_snow_1795729546331.jpg",
    "moon": "map_icon_moon_1795729554391.jpg",
}

BG_FILES = {
    "beach": "beach_bg_generated_1795729496647.jpg",
    "swamp": "swamp_bg_generated_1795729498724.jpg",
    "snow": "snow_bg_generated_1795729482738.jpg",
    "moon": "moon_bg_generated_1795729497804.jpg",
}

def process_badges():
    print("Processing 5 Master Map Badges...")
    for theme, fn in ICON_FILES.items():
        src_p = os.path.join(BRAIN, fn)
        if not os.path.exists(src_p):
            print(f"  [!] Missing {src_p}")
            continue

        im = Image.open(src_p).convert("RGBA")
        arr = np.array(im)
        h, w = arr.shape[:2]

        # Flood fill transparent exterior from 4 corners
        visited = np.zeros((h, w), dtype=bool)
        queue = deque([(0, 0), (0, w - 1), (h - 1, 0), (h - 1, w - 1)])
        for y, x in queue:
            visited[y, x] = True

        while queue:
            cy, cx = queue.popleft()
            arr[cy, cx, 3] = 0
            for dy, dx in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                ny, nx = cy + dy, cx + dx
                if 0 <= ny < h and 0 <= nx < w and not visited[ny, nx]:
                    # Near-white exterior canvas outside the black outer pixel frame
                    if arr[ny, nx, 0] > 230 and arr[ny, nx, 1] > 230 and arr[ny, nx, 2] > 230:
                        visited[ny, nx] = True
                        queue.append((ny, nx))

        img_trans = Image.fromarray(arr, "RGBA")
        bbox = img_trans.getbbox()
        if bbox:
            # Crop tightly to outer badge border with 4px margin
            x0 = max(0, bbox[0] - 4)
            y0 = max(0, bbox[1] - 4)
            x1 = min(w, bbox[2] + 4)
            y1 = min(h, bbox[3] + 4)
            cropped = img_trans.crop((x0, y0, x1, y1))
        else:
            cropped = img_trans

        # High resolution 256x256
        icon_256 = cropped.resize((256, 256), Image.Resampling.LANCZOS)
        # Compact 128x128
        icon_128 = cropped.resize((128, 128), Image.Resampling.LANCZOS)

        # Deploy
        icon_256.save(os.path.join(ASSETS, "ui", f"map_icon_{theme}_256.png"), optimize=True)
        icon_128.save(os.path.join(ASSETS, "ui", f"map_icon_{theme}.png"), optimize=True)
        icon_256.save(os.path.join(ASSETS, theme, "terrain.png"), optimize=True)
        icon_256.save(os.path.join(ASSETS, f"terrain_{theme}.png"), optimize=True)
        print(f"  [+] Saved {theme} badge icon (256x256 & 128x128)")

def make_seamless_strip(layer_img):
    """Mirror 1920 horizontally to create a 3840 seamless looping strip."""
    arr = np.array(layer_img)
    arr_flip = np.fliplr(arr)
    strip = np.hstack((arr, arr_flip))
    return Image.fromarray(strip, mode=layer_img.mode)

def process_backgrounds():
    print("Processing and segmenting 4 Master Backgrounds...")
    
    # Layer boundaries for each theme (in 1920x1080 space):
    # mountain_top_y: where mountains start
    # hill_top_y: where foreground hills/trees start
    params = {
        "beach": {
            "mtn_y": 420,
            "mtn_fade": 40,
            "hill_y": 640,
            "hill_fade": 30,
            "sky_extend_col": (45, 145, 245),
        },
        "swamp": {
            "mtn_y": 380,
            "mtn_fade": 45,
            "hill_y": 600,
            "hill_fade": 35,
            "sky_extend_col": (40, 55, 38),
        },
        "snow": {
            "mtn_y": 370,
            "mtn_fade": 45,
            "hill_y": 620,
            "hill_fade": 35,
            "sky_extend_col": (15, 28, 65),
        },
        "moon": {
            "mtn_y": 370,
            "mtn_fade": 40,
            "hill_y": 640,
            "hill_fade": 30,
            "sky_extend_col": (6, 8, 16),
        },
    }

    for theme, fn in BG_FILES.items():
        src_p = os.path.join(BRAIN, fn)
        if not os.path.exists(src_p):
            print(f"  [!] Missing {src_p}")
            continue

        raw = Image.open(src_p).convert("RGBA")
        base = raw.resize((1920, 1080), Image.Resampling.LANCZOS)
        out_dir = os.path.join(ASSETS, theme)
        os.makedirs(out_dir, exist_ok=True)

        # 1. Save full composite 1920x1080 background.png
        base.convert("RGB").save(os.path.join(out_dir, "background.png"), optimize=True)
        print(f"  [+] Saved {theme}/background.png (1920x1080)")

        arr = np.array(base)
        h, w = 1080, 1920
        cfg = params[theme]

        # 2. Extract Layer 1: Sky (bg_sky.png)
        # Keeps rows 0 to mtn_y + fade, and extends natural sky down to row 1080
        sky_arr = np.zeros((h, w, 4), dtype=np.uint8)
        mtn_y = cfg["mtn_y"]
        mtn_fade = cfg["mtn_fade"]
        for y in range(h):
            if y < mtn_y:
                sky_arr[y, :, :] = arr[y, :, :]
            elif y < mtn_y + mtn_fade:
                t = (y - mtn_y) / float(mtn_fade)
                # Blend lower sky with sky_extend_col
                for x in range(w):
                    r = int(arr[y, x, 0] * (1 - t) + cfg["sky_extend_col"][0] * t)
                    g = int(arr[y, x, 1] * (1 - t) + cfg["sky_extend_col"][1] * t)
                    b = int(arr[y, x, 2] * (1 - t) + cfg["sky_extend_col"][2] * t)
                    sky_arr[y, x] = [r, g, b, 255]
            else:
                sky_arr[y, :, 0] = cfg["sky_extend_col"][0]
                sky_arr[y, :, 1] = cfg["sky_extend_col"][1]
                sky_arr[y, :, 2] = cfg["sky_extend_col"][2]
                sky_arr[y, :, 3] = 255

        img_sky = make_seamless_strip(Image.fromarray(sky_arr, "RGBA"))
        img_sky.save(os.path.join(out_dir, "bg_sky.png"), optimize=True)
        print(f"  [+] Saved {theme}/bg_sky.png (3840x1080)")

        # 3. Extract Layer 2: Distant Mountains (bg_mountains.png)
        # Transparent above mtn_y - mtn_fade, then solid down to row 1080
        mtn_arr = np.zeros((h, w, 4), dtype=np.uint8)
        top_mtn = mtn_y - int(mtn_fade * 0.7)
        for y in range(h):
            if y < top_mtn:
                mtn_arr[y, :, 3] = 0
            elif y < top_mtn + mtn_fade:
                t = (y - top_mtn) / float(mtn_fade)
                alpha = int(255 * (t ** 2))
                mtn_arr[y, :, :3] = arr[y, :, :3]
                mtn_arr[y, :, 3] = alpha
            else:
                mtn_arr[y, :, :] = arr[y, :, :]

        img_mtn = make_seamless_strip(Image.fromarray(mtn_arr, "RGBA"))
        img_mtn.save(os.path.join(out_dir, "bg_mountains.png"), optimize=True)
        print(f"  [+] Saved {theme}/bg_mountains.png (3840x1080)")

        # 4. Extract Layer 3: Foreground Hills & Trees (bg_hills.png)
        # Transparent above hill_y - hill_fade, then solid down to row 1080
        hill_arr = np.zeros((h, w, 4), dtype=np.uint8)
        hill_y = cfg["hill_y"]
        hill_fade = cfg["hill_fade"]
        top_hill = hill_y - int(hill_fade * 0.7)
        for y in range(h):
            if y < top_hill:
                hill_arr[y, :, 3] = 0
            elif y < top_hill + hill_fade:
                t = (y - top_hill) / float(hill_fade)
                alpha = int(255 * (t ** 2))
                hill_arr[y, :, :3] = arr[y, :, :3]
                hill_arr[y, :, 3] = alpha
            else:
                hill_arr[y, :, :] = arr[y, :, :]

        img_hills = make_seamless_strip(Image.fromarray(hill_arr, "RGBA"))
        img_hills.save(os.path.join(out_dir, "bg_hills.png"), optimize=True)
        print(f"  [+] Saved {theme}/bg_hills.png (3840x1080)")

if __name__ == "__main__":
    process_badges()
    process_backgrounds()
    print("Processing completed successfully!")
