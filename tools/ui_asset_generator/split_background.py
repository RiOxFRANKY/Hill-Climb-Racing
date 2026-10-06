#!/usr/bin/env python3
"""
Split background.png into 3 parallax layers:
1. bg_sky.png (Sky + Clouds)
2. bg_mountains.png (Distant blue/teal mountain range)
3. bg_hills.png (Closer green rolling hills and trees)
Also ensures seamless horizontal wrapping by creating a 3840px mirrored strip.
"""

import os
from collections import deque
import numpy as np
from PIL import Image

ASSETS_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "assets"))
BG_PATH = os.path.join(ASSETS_DIR, "background.png")

def split_background():
    img = Image.open(BG_PATH).convert("RGBA")
    w, h = img.size
    arr = np.array(img)

    # 1. Connected Component Flood-Fill from (0, 0) to find the exact contiguous Sky region.
    # Sky contains blue gradients and clouds, while mountain pixels stop the flood fill.
    visited = np.zeros((h, w), dtype=bool)
    queue = deque([(0, 0)])
    visited[0, 0] = True

    def is_sky_or_cloud(y, x):
        r, g, b, _ = [int(v) for v in arr[y, x]]
        # Open sky tones:
        if b >= 235 and g >= 165 and r <= 95:
            return True
        # Cloud whites / pale blues:
        if r >= 100 and g >= 140 and b >= 190:
            return True
        # Lighter sky blue tones:
        if b >= 240 and g >= 164:
            return True
        return False

    while queue:
        cy, cx = queue.popleft()
        for dy, dx in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
            ny, nx = cy + dy, cx + dx
            if 0 <= ny < h and 0 <= nx < w and not visited[ny, nx]:
                if is_sky_or_cloud(ny, nx):
                    visited[ny, nx] = True
                    queue.append((ny, nx))

    # Mountain horizon is the bottom-most sky pixel + 1 in each column
    mountain_horizon = np.zeros(w, dtype=int)
    for x in range(w):
        col = np.where(visited[:, x])[0]
        mountain_horizon[x] = col.max() + 1 if len(col) > 0 else 450

    # 2. Detect Front Hills boundary (where bright green rolling hills & tree rows start)
    # Front rolling hills have either bright lime grass (G >= 165, R >= 85, G > B + 40)
    # or dark green ridge trees (G >= 60, B <= 70, G > B + 20).
    hills_horizon = np.zeros(w, dtype=int)
    for x in range(w):
        found = False
        for y in range(480, h):
            r, g, b, _ = [int(v) for v in arr[y, x]]
            is_front_grass = (g >= 165 and r >= 85 and g > b + 40)
            is_front_tree = (g >= 60 and b <= 70 and g > b + 20)
            if is_front_grass or is_front_tree:
                hills_horizon[x] = y
                found = True
                break
        if not found:
            hills_horizon[x] = 650

    # 3. Create Layer 1: Sky
    # Full sky and clouds, extending natural sky gradient down to row 1080
    sky_arr = np.zeros((h, w, 4), dtype=np.uint8)
    for y in range(h):
        for x in range(w):
            if visited[y, x]:
                sky_arr[y, x] = arr[y, x]
            else:
                # Natural sky gradient continuation below mountains
                t = max(0.0, min(1.0, (y - 380) / 400.0))
                r = int(np.clip(70 + (210 - 70) * t, 0, 255))
                g = int(np.clip(174 + (238 - 174) * t, 0, 255))
                b = int(np.clip(252 + (230 - 252) * t, 0, 255))
                sky_arr[y, x] = [r, g, b, 255]

    # 4. Create Layer 2: Mountains
    # Transparent above mountain_horizon[x].
    # Below hills_horizon[x], extend mountain tones so parallax motion never exposes holes.
    mountains_arr = np.zeros((h, w, 4), dtype=np.uint8)
    for x in range(w):
        top_y = mountain_horizon[x]
        hill_y = max(top_y, hills_horizon[x])
        for y in range(top_y, h):
            if y < hill_y:
                mountains_arr[y, x] = arr[y, x]
            else:
                # Smoothly extend mountain tone downwards
                sample_y = max(top_y, hill_y - 1 - (y - hill_y) % 20)
                mountains_arr[y, x] = arr[sample_y, x]

    # 5. Create Layer 3: Hills
    # Transparent above hills_horizon[x], original pixels below
    hills_arr = np.zeros((h, w, 4), dtype=np.uint8)
    for x in range(w):
        hill_y = hills_horizon[x]
        for y in range(hill_y, h):
            hills_arr[y, x] = arr[y, x]

    # Helper to create mirrored 2x strip for perfect seamless wrapping
    def make_seamless_strip(layer_arr):
        left = layer_arr
        right = np.fliplr(layer_arr)
        strip = np.hstack((left, right))
        return Image.fromarray(strip, mode="RGBA")

    img_sky = make_seamless_strip(sky_arr)
    img_mountains = make_seamless_strip(mountains_arr)
    img_hills = make_seamless_strip(hills_arr)

    sky_path = os.path.join(ASSETS_DIR, "bg_sky.png")
    mountains_path = os.path.join(ASSETS_DIR, "bg_mountains.png")
    hills_path = os.path.join(ASSETS_DIR, "bg_hills.png")

    img_sky.save(sky_path, optimize=True)
    img_mountains.save(mountains_path, optimize=True)
    img_hills.save(hills_path, optimize=True)

    print(f"Generated parallax layers in {ASSETS_DIR}:")
    print(f"  [+] bg_sky.png ({img_sky.size})")
    print(f"  [+] bg_mountains.png ({img_mountains.size})")
    print(f"  [+] bg_hills.png ({img_hills.size})")

if __name__ == "__main__":
    split_background()
