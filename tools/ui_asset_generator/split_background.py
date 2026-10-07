#!/usr/bin/env python3
"""
Split background.png into 3 parallax layers:
1. bg_sky.png (Sky + Clouds)
2. bg_mountains.png (Distant blue/teal mountain range naturally extended all the way down to row 1080)
3. bg_hills.png (Rolling green hills with 100% full, unchunked trees on crests and valleys)
Also ensures seamless horizontal wrapping by creating a 3840px mirrored strip for each.
"""

import os
from collections import deque
import numpy as np
from PIL import Image

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ASSETS_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, "..", "..", "assets"))
BG_PATH = os.path.join(ASSETS_DIR, "background.png")
EXTENSION_PATH = os.path.join(SCRIPT_DIR, "clean_initial_mountains.jpg")

def split_background():
    img = Image.open(BG_PATH).convert("RGBA")
    w, h = img.size
    arr = np.array(img)

    # 1. Connected Component Flood-Fill from (0, 0) to find the exact contiguous Sky region.
    visited = np.zeros((h, w), dtype=bool)
    queue = deque([(0, 0)])
    visited[0, 0] = True

    def is_sky_or_cloud(y, x):
        r, g, b, _ = [int(v) for v in arr[y, x]]
        if b >= 235 and g >= 165 and r <= 95:
            return True
        if r >= 100 and g >= 140 and b >= 190:
            return True
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

    # 2. Exact Tree and Hill Top Boundary
    # Ensures every single tree crown, trunk, branch, and leaf on the hill ridge
    # is 100% captured in the front hills layer with ZERO chunking.
    def is_hill_or_tree(r, g, b):
        # Foliage (deep green, olive, lime)
        if g > b + 14 and b < 90:
            return True
        if g > 135 and b < 115:
            return True
        # Tree trunks and branches (brown)
        if r > 45 and g > 35 and b < 65 and r > b + 8:
            return True
        # Dark foliage shadow
        if g > 38 and b < 55 and r < 55 and g >= b + 8:
            return True
        return False

    tree_top = np.zeros(w, dtype=int)
    for x in range(w):
        found = False
        for y in range(400, h):
            r, g, b, _ = [int(v) for v in arr[y, x]]
            if is_hill_or_tree(r, g, b):
                tree_top[x] = y
                found = True
                break
        if not found:
            tree_top[x] = 700

    # 3. Create Layer 1: Sky
    # Full sky and clouds, extending natural sky gradient down to row 1080
    sky_arr = np.zeros((h, w, 4), dtype=np.uint8)
    for y in range(h):
        for x in range(w):
            if visited[y, x]:
                sky_arr[y, x] = arr[y, x]
            else:
                t = max(0.0, min(1.0, (y - 380) / 400.0))
                r = int(np.clip(70 + (210 - 70) * t, 0, 255))
                g = int(np.clip(174 + (238 - 174) * t, 0, 255))
                b = int(np.clip(252 + (230 - 252) * t, 0, 255))
                sky_arr[y, x] = [r, g, b, 255]

    # 4. Create Layer 2: Middle Mountains (Extended Downwards)
    # Uses clean_initial_mountains.jpg which strictly extends ONLY the authentic
    # rolling teal-green and soft blue pixel-art mountains down to row 1080.
    # Completely eliminates alien mountain types, grey crags, and barcode streaks.
    # Sky region above mountain ridge is 100% transparent.
    ai_ext = Image.open(EXTENSION_PATH).resize((w, h), Image.Resampling.LANCZOS)
    arr_ext = np.array(ai_ext.convert("RGB"))

    mountains_arr = np.zeros((h, w, 4), dtype=np.uint8)

    # Detect the top edge of the mountain in each column (mountain pixels have R <= 110)
    mtn_edge = np.zeros(w, dtype=int)
    for x in range(w):
        for y in range(h):
            if arr_ext[y, x, 0] <= 110 and (arr_ext[y, x, 1] > 100 or arr_ext[y, x, 2] > 100):
                mtn_edge[x] = y
                break

    for x in range(w):
        top_y = mtn_edge[x]
        for y in range(top_y, h):
            mountains_arr[y, x] = [arr_ext[y, x, 0], arr_ext[y, x, 1], arr_ext[y, x, 2], 255]

    # 5. Create Layer 3: Rolling Hills (Full tree lines fully intact, not chunked)
    # Transparent above tree crowns; solid 100% of the hill terrain and trees below.
    hills_arr = np.zeros((h, w, 4), dtype=np.uint8)
    for x in range(w):
        cut_y = tree_top[x]
        for y in range(cut_y, h):
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
