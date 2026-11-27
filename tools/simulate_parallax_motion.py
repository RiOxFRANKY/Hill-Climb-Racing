#!/usr/bin/env python3
"""
simulate_parallax_motion.py
Simulates in-engine parallax rendering at 0m and 80m for all 4 themes (Beach, Swamp, Snow, Moon)
to verify that there is ZERO horizontal shearing, ZERO cut lines, and smooth multi-depth parallax.
"""

import os
import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS = os.path.join(ROOT, "assets")
OUT_DIR = os.path.join(ROOT, "temp", "verify_motion")
os.makedirs(OUT_DIR, exist_ok=True)

THEMES = ["swamp", "beach", "snow", "moon"]

def simulate_theme(theme):
    theme_dir = os.path.join(ASSETS, theme)
    sky_p = os.path.join(theme_dir, "bg_sky.png")
    mtn_p = os.path.join(theme_dir, "bg_mountains.png")
    hill_p = os.path.join(theme_dir, "bg_hills.png")

    if not (os.path.exists(sky_p) and os.path.exists(mtn_p) and os.path.exists(hill_p)):
        print(f"Missing layers for {theme}")
        return

    sky_strip = np.array(Image.open(sky_p).convert("RGBA"))
    mtn_strip = np.array(Image.open(mtn_p).convert("RGBA"))
    hill_strip = np.array(Image.open(hill_p).convert("RGBA"))

    h, strip_w = sky_strip.shape[:2]
    w = 1920

    for cam_m in [0, 80]:
        cam_x = cam_m * 28.0 # camera coordinate in pixels
        s_shift = int((cam_x * 0.012) % strip_w)
        m_shift = int((cam_x * 0.038) % strip_w)
        h_shift = int((cam_x * 0.095) % strip_w)

        # 1. Sky
        sky_sub = np.hstack((sky_strip, sky_strip))[:, s_shift:s_shift + w]
        comp = Image.fromarray(sky_sub, "RGBA")

        # 2. Midground Mountains
        mtn_sub = np.hstack((mtn_strip, mtn_strip))[:, m_shift:m_shift + w]
        mtn_img = Image.fromarray(mtn_sub, "RGBA")
        comp.paste(mtn_img, (0, 0), Image.fromarray(mtn_sub[:, :, 3], "L"))

        # 3. Foreground Hills & Trees
        hill_sub = np.hstack((hill_strip, hill_strip))[:, h_shift:h_shift + w]
        hill_img = Image.fromarray(hill_sub, "RGBA")
        comp.paste(hill_img, (0, 0), Image.fromarray(hill_sub[:, :, 3], "L"))

        out_path = os.path.join(OUT_DIR, f"{theme}_{cam_m}m.png")
        comp.save(out_path, format="PNG")
        print(f"  [+] Simulated {theme} at {cam_m}m -> {out_path}")

if __name__ == "__main__":
    for t in THEMES:
        simulate_theme(t)
    print("Motion simulation completed successfully!")
