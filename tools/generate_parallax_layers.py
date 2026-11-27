#!/usr/bin/env python3
"""
Generate 3-layer seamless parallax backgrounds for Snow, Beach, Moon, and Swamp.
Each theme gets 3 layers (3840x1080 each):
  1. bg_sky.png
  2. bg_mountains.png (transparent above skyline)
  3. bg_hills.png (transparent above foreground ridge)
Outputs saved to:
  temp/snow/parallax/
  temp/beach/parallax/
  temp/moon/parallax/
  temp/swamp/parallax/
"""

import os
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TEMP_DIR = os.path.join(ROOT, "temp")
W, H = 1920, 1080
STRIP_W = W * 2 # 3840

def make_seamless_strip(img):
    """Mirror 1920 horizontally to create a 3840 seamless looping strip."""
    arr = np.array(img)
    arr_flip = np.fliplr(arr)
    strip = np.hstack((arr, arr_flip))
    return Image.fromarray(strip, mode=img.mode)

# ==========================================
# 1. SNOW PARALLAX
# ==========================================
def generate_snow_parallax():
    print("Generating Snow parallax layers...")
    
    # 1.1 Sky (Aurora Borealis, cold polar night gradient, moon, stars)
    sky = Image.new("RGBA", (W, H))
    s_arr = np.zeros((H, W, 4), dtype=np.uint8)
    for y in range(H):
        t = y / float(H)
        # Deep polar indigo to cold frosty cyan
        r = int(12 + (85 - 12) * (t ** 1.3))
        g = int(22 + (155 - 22) * (t ** 1.3))
        b = int(55 + (225 - 55) * (t ** 1.3))
        s_arr[y, :, :] = [r, g, b, 255]
    
    # Aurora Borealis curtains
    np.random.seed(42)
    for y in range(80, 520):
        ty = (y - 80) / 440.0
        wave1 = math.sin(ty * math.pi) * 0.9
        for x in range(W):
            aurora_w = math.sin(x * 0.008 + ty * 4.0) * math.cos(x * 0.003 - ty * 2.0)
            glow = max(0.0, (aurora_w + 0.6) * wave1)
            if glow > 0.15:
                # Green/cyan neon core, magenta edges
                curtain = (math.sin(x * 0.04 + y * 0.08) + 1) * 0.5
                ar = int(np.clip(s_arr[y, x, 0] + 50 * glow * curtain, 0, 255))
                ag = int(np.clip(s_arr[y, x, 1] + 180 * glow, 0, 255))
                ab = int(np.clip(s_arr[y, x, 2] + 120 * glow * (1 - curtain * 0.5), 0, 255))
                s_arr[y, x] = [ar, ag, ab, 255]

    # Stars
    for _ in range(400):
        sx = np.random.randint(0, W)
        sy = np.random.randint(0, 480)
        bright = np.random.randint(180, 255)
        s_arr[sy, sx] = [bright, bright, 255, 255]

    sky_img = Image.fromarray(s_arr, "RGBA")
    s_draw = ImageDraw.Draw(sky_img)
    # Crescent cold moon
    mx, my, mr = 1550, 160, 45
    s_draw.ellipse([mx-mr-15, my-mr-15, mx+mr+15, my+mr+15], fill=(210, 235, 255, 30))
    s_draw.ellipse([mx-mr, my-mr, mx+mr, my+mr], fill=(245, 250, 255, 240))
    s_draw.ellipse([mx-mr+18, my-mr-10, mx+mr+18, my+mr-10], fill=(25, 45, 95, 255)) # cutout

    # 1.2 Distant Glaciated Mountains
    mtn = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    m_arr = np.zeros((H, W, 4), dtype=np.uint8)
    
    # Mountain ridge profile
    mtn_y = np.zeros(W, dtype=int)
    for x in range(W):
        h_val = 420 - 130 * math.sin(x * 0.005) - 90 * math.cos(x * 0.012 + 1.2) - 45 * math.sin(x * 0.025)
        mtn_y[x] = int(np.clip(h_val, 220, 600))
    
    for x in range(W):
        top = mtn_y[x]
        for y in range(top, H):
            depth = (y - top) / float(H - top)
            # Sharp glacial rock & snow shading
            facet = math.sin((x + y * 0.6) * 0.03)
            if facet > 0:
                # Sunlit snow face (crisp white/pale blue)
                r = int(np.clip(210 + 35 * (1 - depth * 0.6), 0, 255))
                g = int(np.clip(225 + 25 * (1 - depth * 0.6), 0, 255))
                b = int(np.clip(245 + 10 * (1 - depth * 0.5), 0, 255))
            else:
                # Deep glacial blue shadow
                r = int(np.clip(65 + 60 * depth, 0, 255))
                g = int(np.clip(95 + 70 * depth, 0, 255))
                b = int(np.clip(150 + 65 * depth, 0, 255))
            m_arr[y, x] = [r, g, b, 255]
    mtn_img = Image.fromarray(m_arr, "RGBA")

    # 1.3 Closer Snowy Foothills & Pine Trees
    hills = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    h_arr = np.zeros((H, W, 4), dtype=np.uint8)
    hill_y = np.zeros(W, dtype=int)
    for x in range(W):
        h_val = 680 - 80 * math.sin(x * 0.007) - 50 * math.cos(x * 0.018)
        hill_y[x] = int(np.clip(h_val, 550, 800))
    
    for x in range(W):
        top = hill_y[x]
        for y in range(top, H):
            t = (y - top) / float(H - top)
            r = int(185 + (130 - 185) * t)
            g = int(210 + (160 - 210) * t)
            b = int(240 + (195 - 240) * t)
            h_arr[y, x] = [r, g, b, 255]
    
    hills_img = Image.fromarray(h_arr, "RGBA")
    h_draw = ImageDraw.Draw(hills_img)

    # Add snowy pine trees along the hill crest
    for tx in range(20, W - 20, 32):
        ty = hill_y[tx]
        th = int(55 + 25 * math.sin(tx * 0.2))
        # Draw layered evergreen triangles
        for layer in range(3):
            ly = ty - th + layer * 16
            lw = int((layer + 1) * 14)
            # Dark green pine foliage with white snow cap
            h_draw.polygon([(tx, ly - 8), (tx - lw, ly + 14), (tx + lw, ly + 14)], fill=(28, 55, 38, 255))
            h_draw.polygon([(tx, ly - 8), (tx - lw + 3, ly + 6), (tx + lw - 3, ly + 6)], fill=(240, 248, 255, 255))
        # Trunk
        h_draw.rectangle([tx - 2, ty - 6, tx + 2, ty + 8], fill=(60, 42, 28, 255))

    out_dir = os.path.join(TEMP_DIR, "snow", "parallax")
    make_seamless_strip(sky_img).save(os.path.join(out_dir, "bg_sky.png"), optimize=True)
    make_seamless_strip(mtn_img).save(os.path.join(out_dir, "bg_mountains.png"), optimize=True)
    make_seamless_strip(hills_img).save(os.path.join(out_dir, "bg_hills.png"), optimize=True)
    print("  [+] Snow parallax layers saved (3840x1080).")

# ==========================================
# 2. BEACH PARALLAX
# ==========================================
def generate_beach_parallax():
    print("Generating Beach parallax layers...")
    
    # 2.1 Sky (Tropical azure, bright sun, wispy clouds)
    sky = Image.new("RGBA", (W, H))
    s_arr = np.zeros((H, W, 4), dtype=np.uint8)
    for y in range(H):
        t = y / float(H)
        r = int(45 + (185 - 45) * t)
        g = int(145 + (225 - 145) * t)
        b = int(245 + (255 - 245) * t)
        s_arr[y, :, :] = [r, g, b, 255]
    
    sky_img = Image.fromarray(s_arr, "RGBA")
    s_draw = ImageDraw.Draw(sky_img)
    # Tropical Sun & Corona
    sun_x, sun_y = 1350, 220
    s_draw.ellipse([sun_x - 120, sun_y - 120, sun_x + 120, sun_y + 120], fill=(255, 250, 210, 40))
    s_draw.ellipse([sun_x - 65, sun_y - 65, sun_x + 65, sun_y + 65], fill=(255, 255, 235, 120))
    s_draw.ellipse([sun_x - 38, sun_y - 38, sun_x + 38, sun_y + 38], fill=(255, 255, 250, 255))

    # Fluffy cumulus clouds
    def draw_cloud(cx, cy, scale):
        for ox, oy, r in [(0, 0, 45), (-35, 10, 35), (35, 12, 35), (-60, 20, 25), (60, 20, 25)]:
            s_draw.ellipse([cx + ox*scale - r*scale, cy + oy*scale - r*scale,
                            cx + ox*scale + r*scale, cy + oy*scale + r*scale],
                           fill=(255, 255, 255, 210))
    draw_cloud(350, 260, 1.2)
    draw_cloud(880, 180, 0.9)
    draw_cloud(1650, 320, 1.1)

    # Distant seagulls
    for gx, gy in [(620, 240), (660, 230), (710, 250), (1450, 360), (1490, 350)]:
        s_draw.arc([gx - 12, gy, gx, gy + 10], 180, 360, fill=(70, 90, 120, 220), width=2)
        s_draw.arc([gx, gy, gx + 12, gy + 10], 180, 360, fill=(70, 90, 120, 220), width=2)

    # 2.2 Distant Coastal Sea Cliffs & Ocean Horizon
    mtn = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    m_arr = np.zeros((H, W, 4), dtype=np.uint8)
    
    mtn_y = np.zeros(W, dtype=int)
    for x in range(W):
        # Volcanic island silhouettes and sea stacks
        isl1 = 120 * math.exp(-((x - 450) / 180.0) ** 2)
        isl2 = 160 * math.exp(-((x - 1250) / 240.0) ** 2)
        stack = 70 * math.exp(-((x - 850) / 35.0) ** 2)
        horizon = 620
        top = int(horizon - (isl1 + isl2 + stack))
        mtn_y[x] = top

    for x in range(W):
        top = mtn_y[x]
        for y in range(top, H):
            if y < 620:
                # Distant island cliffs (tropical blue-green haze)
                t = (y - top) / float(620 - top + 1)
                r = int(60 + 40 * t)
                g = int(115 + 35 * t)
                b = int(145 + 25 * t)
            else:
                # Sparkling ocean surface down to row 1080
                t = (y - 620) / float(H - 620)
                r = int(24 + 30 * t)
                g = int(135 + 40 * t)
                b = int(195 + 20 * t)
            m_arr[y, x] = [r, g, b, 255]
    mtn_img = Image.fromarray(m_arr, "RGBA")

    # 2.3 Rolling Golden Sand Dunes & Swaying Palm Trees
    hills = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    h_arr = np.zeros((H, W, 4), dtype=np.uint8)
    hill_y = np.zeros(W, dtype=int)
    for x in range(W):
        h_val = 720 - 70 * math.sin(x * 0.006) - 45 * math.cos(x * 0.015)
        hill_y[x] = int(np.clip(h_val, 620, 820))

    for x in range(W):
        top = hill_y[x]
        for y in range(top, H):
            t = (y - top) / float(H - top)
            # Warm golden sand dune gradient
            r = int(240 - 55 * t)
            g = int(205 - 65 * t)
            b = int(140 - 60 * t)
            h_arr[y, x] = [r, g, b, 255]

    hills_img = Image.fromarray(h_arr, "RGBA")
    h_draw = ImageDraw.Draw(hills_img)

    # Draw coconut palm trees
    palm_xs = [160, 320, 680, 840, 1140, 1420, 1750]
    for px in palm_xs:
        py = hill_y[px]
        # Curved trunk
        trunk_pts = []
        lean = 25 * math.sin(px)
        for step in range(8):
            st = step / 7.0
            tx = px + lean * (st ** 1.5)
            ty = py - 110 * st
            trunk_pts.append((tx, ty))
        for i in range(len(trunk_pts) - 1):
            h_draw.line([trunk_pts[i], trunk_pts[i+1]], fill=(120, 88, 55, 255), width=max(4, int(10 - i)))
        
        # Palm fronds
        top_tx, top_ty = trunk_pts[-1]
        for fa in np.linspace(0, 2 * math.pi, 8, endpoint=False):
            fx = top_tx + 55 * math.cos(fa)
            fy = top_ty + 35 * math.sin(fa) + 12 # droop
            h_draw.line([(top_tx, top_ty), (fx, fy)], fill=(38, 145, 52, 255), width=4)
            # Little leaflets
            for fstep in range(4):
                fst = fstep / 3.0
                lx = top_tx + (fx - top_tx) * fst
                ly = top_ty + (fy - top_ty) * fst
                h_draw.line([(lx, ly), (lx + 8 * math.sin(fa), ly + 10)], fill=(50, 168, 65, 255), width=2)

    out_dir = os.path.join(TEMP_DIR, "beach", "parallax")
    make_seamless_strip(sky_img).save(os.path.join(out_dir, "bg_sky.png"), optimize=True)
    make_seamless_strip(mtn_img).save(os.path.join(out_dir, "bg_mountains.png"), optimize=True)
    make_seamless_strip(hills_img).save(os.path.join(out_dir, "bg_hills.png"), optimize=True)
    print("  [+] Beach parallax layers saved (3840x1080).")

# ==========================================
# 3. MOON PARALLAX
# ==========================================
def generate_moon_parallax():
    print("Generating Moon parallax layers...")
    
    # 3.1 Deep Cosmic Space (Twinkling stars, colorful nebulae, glowing Earth)
    sky = Image.new("RGBA", (W, H))
    s_arr = np.zeros((H, W, 4), dtype=np.uint8)
    for y in range(H):
        t = y / float(H)
        r = int(4 + 8 * t)
        g = int(6 + 10 * t)
        b = int(14 + 18 * t)
        s_arr[y, :, :] = [r, g, b, 255]

    # Cosmic Nebulae (magenta and cyan dust clouds)
    np.random.seed(99)
    for y in range(50, 550):
        for x in range(W):
            n1 = math.sin(x * 0.006 + y * 0.008) * math.cos(x * 0.004 - y * 0.005)
            n2 = math.sin((x + 500) * 0.005) * math.sin(y * 0.01)
            if n1 > 0.4:
                # Magenta nebula
                intensity = (n1 - 0.4) * 1.5
                s_arr[y, x, 0] = int(np.clip(s_arr[y, x, 0] + 80 * intensity, 0, 255))
                s_arr[y, x, 1] = int(np.clip(s_arr[y, x, 1] + 20 * intensity, 0, 255))
                s_arr[y, x, 2] = int(np.clip(s_arr[y, x, 2] + 95 * intensity, 0, 255))
            elif n2 > 0.45:
                # Cyan nebula
                intensity = (n2 - 0.45) * 1.6
                s_arr[y, x, 0] = int(np.clip(s_arr[y, x, 0] + 15 * intensity, 0, 255))
                s_arr[y, x, 1] = int(np.clip(s_arr[y, x, 1] + 75 * intensity, 0, 255))
                s_arr[y, x, 2] = int(np.clip(s_arr[y, x, 2] + 110 * intensity, 0, 255))

    # Dense Starfield
    for _ in range(800):
        sx = np.random.randint(0, W)
        sy = np.random.randint(0, 700)
        c = np.random.randint(180, 255)
        s_arr[sy, sx] = [c, c, np.random.randint(220, 255), 255]

    sky_img = Image.fromarray(s_arr, "RGBA")
    s_draw = ImageDraw.Draw(sky_img)

    # Planet Earth floating in space
    ex, ey, er = 1420, 230, 80
    # Atmosphere glow
    s_draw.ellipse([ex - er - 14, ey - er - 14, ex + er + 14, ey + er + 14], fill=(70, 160, 255, 60))
    s_draw.ellipse([ex - er - 6, ey - er - 6, ex + er + 6, ey + er + 6], fill=(90, 190, 255, 120))
    # Ocean blue marble
    s_draw.ellipse([ex - er, ey - er, ex + er, ey + er], fill=(25, 95, 190, 255))
    # Continents (green/tan)
    s_draw.ellipse([ex - 45, ey - 50, ex + 20, ey + 10], fill=(55, 145, 75, 230))
    s_draw.ellipse([ex + 5, ey - 10, ex + 60, ey + 45], fill=(70, 155, 80, 230))
    # Swirling white clouds
    s_draw.arc([ex - er + 15, ey - 35, ex + er - 10, ey + 25], 30, 170, fill=(255, 255, 255, 220), width=12)
    s_draw.arc([ex - 50, ey + 15, ex + 55, ey + 65], 190, 340, fill=(255, 255, 255, 210), width=10)

    # 3.2 Distant Lunar Highlands & Sharp Crater Rims
    mtn = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    m_arr = np.zeros((H, W, 4), dtype=np.uint8)
    
    mtn_y = np.zeros(W, dtype=int)
    for x in range(W):
        h_val = 460 - 110 * math.sin(x * 0.007) - 75 * math.cos(x * 0.016 + 0.8) - 35 * math.sin(x * 0.035)
        mtn_y[x] = int(np.clip(h_val, 280, 620))

    for x in range(W):
        top = mtn_y[x]
        for y in range(top, H):
            depth = (y - top) / float(H - top)
            # High-contrast vacuum lighting (silvery sunlight vs pitch shadow)
            facet = math.sin((x + y * 0.4) * 0.04)
            if facet > -0.1:
                # Sunlit regolith rock
                val = int(np.clip(185 - 60 * depth, 80, 230))
                m_arr[y, x] = [val, val, int(val * 1.05), 255]
            else:
                # Harsh shadow
                val = int(np.clip(28 + 25 * depth, 20, 65))
                m_arr[y, x] = [val, val, int(val * 1.1), 255]
    mtn_img = Image.fromarray(m_arr, "RGBA")

    # 3.3 Closer Lunar Crater Ridges & Surface Outposts
    hills = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    h_arr = np.zeros((H, W, 4), dtype=np.uint8)
    hill_y = np.zeros(W, dtype=int)
    for x in range(W):
        h_val = 690 - 70 * math.sin(x * 0.008) - 45 * math.cos(x * 0.02)
        hill_y[x] = int(np.clip(h_val, 560, 800))

    for x in range(W):
        top = hill_y[x]
        for y in range(top, H):
            t = (y - top) / float(H - top)
            # Lunar soil strata
            val = int(125 - 45 * t)
            h_arr[y, x] = [val, val, int(val * 1.04), 255]

    hills_img = Image.fromarray(h_arr, "RGBA")
    h_draw = ImageDraw.Draw(hills_img)

    # Draw Lunar Outpost features: Apollo/Artemis lander, antenna, boulders
    # Lunar Lander at x=780
    lx, ly = 780, hill_y[780]
    # Gold foil descent stage
    h_draw.polygon([(lx - 35, ly), (lx - 25, ly - 35), (lx + 25, ly - 35), (lx + 35, ly)], fill=(215, 175, 45, 255))
    # White ascent cabin
    h_draw.ellipse([lx - 20, ly - 58, lx + 20, ly - 26], fill=(235, 240, 245, 255))
    # Lander struts
    h_draw.line([(lx - 25, ly - 10), (lx - 50, ly + 8)], fill=(180, 185, 195, 255), width=4)
    h_draw.line([(lx + 25, ly - 10), (lx + 50, ly + 8)], fill=(180, 185, 195, 255), width=4)
    # Lander flag
    h_draw.line([(lx + 65, ly + 5), (lx + 65, ly - 35)], fill=(200, 200, 210, 255), width=3)
    h_draw.rectangle([lx + 65, ly - 35, lx + 90, ly - 20], fill=(240, 60, 60, 255))

    # Basalt boulders scattered along the ridge
    for bx in [220, 480, 1120, 1380, 1680]:
        by = hill_y[bx]
        bw, bh = 24, 18
        h_draw.ellipse([bx - bw, by - bh, bx + bw, by + 4], fill=(160, 165, 175, 255))
        # Shadow side
        h_draw.pieslice([bx - bw, by - bh, bx + bw, by + 4], 90, 270, fill=(45, 48, 55, 255))

    out_dir = os.path.join(TEMP_DIR, "moon", "parallax")
    make_seamless_strip(sky_img).save(os.path.join(out_dir, "bg_sky.png"), optimize=True)
    make_seamless_strip(mtn_img).save(os.path.join(out_dir, "bg_mountains.png"), optimize=True)
    make_seamless_strip(hills_img).save(os.path.join(out_dir, "bg_hills.png"), optimize=True)
    print("  [+] Moon parallax layers saved (3840x1080).")

# ==========================================
# 4. SWAMP PARALLAX
# ==========================================
def generate_swamp_parallax():
    print("Generating Swamp parallax layers...")
    
    # 4.1 Eerie Murky Dusk Twilight Sky & Pale Moon
    sky = Image.new("RGBA", (W, H))
    s_arr = np.zeros((H, W, 4), dtype=np.uint8)
    for y in range(H):
        t = y / float(H)
        # Sickly olive green to murky amber sunset glow
        r = int(35 + (130 - 35) * t)
        g = int(50 + (115 - 50) * t)
        b = int(35 + (60 - 35) * t)
        s_arr[y, :, :] = [r, g, b, 255]

    # Thick swamp fog banks illuminated by sunset
    for y in range(250, 700):
        ty = (y - 250) / 450.0
        for x in range(W):
            fog = (math.sin(x * 0.005 + ty * 3.0) + math.cos(x * 0.012 - ty * 2.0) + 2) / 4.0
            if fog > 0.4:
                glow = (fog - 0.4) * 0.7
                s_arr[y, x, 0] = int(np.clip(s_arr[y, x, 0] + 60 * glow, 0, 255))
                s_arr[y, x, 1] = int(np.clip(s_arr[y, x, 1] + 70 * glow, 0, 255))
                s_arr[y, x, 2] = int(np.clip(s_arr[y, x, 2] + 45 * glow, 0, 255))

    sky_img = Image.fromarray(s_arr, "RGBA")
    s_draw = ImageDraw.Draw(sky_img)
    # Pale eerie moon shrouded in mist
    mx, my, mr = 420, 240, 65
    s_draw.ellipse([mx - mr - 20, my - mr - 20, mx + mr + 20, my + mr + 20], fill=(175, 185, 140, 45))
    s_draw.ellipse([mx - mr, my - mr, mx + mr, my + mr], fill=(215, 225, 180, 180))

    # 4.2 Distant Swamp Forest Canopy & Rolling Fog
    mtn = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    m_arr = np.zeros((H, W, 4), dtype=np.uint8)
    
    mtn_y = np.zeros(W, dtype=int)
    for x in range(W):
        # Spooky cypress tree crown silhouettes in the distance
        canopy = 480 - 65 * math.sin(x * 0.01) - 45 * math.sin(x * 0.035) - 25 * math.cos(x * 0.08)
        mtn_y[x] = int(np.clip(canopy, 350, 620))

    for x in range(W):
        top = mtn_y[x]
        for y in range(top, H):
            t = (y - top) / float(H - top)
            # Murky olive-dark tree silhouette with fog blending
            r = int(32 + 35 * t)
            g = int(48 + 30 * t)
            b = int(32 + 20 * t)
            m_arr[y, x] = [r, g, b, 255]
    mtn_img = Image.fromarray(m_arr, "RGBA")

    # 4.3 Closer Gnarled Cypress Trees with Hanging Spanish Moss & Fireflies
    hills = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    h_arr = np.zeros((H, W, 4), dtype=np.uint8)
    hill_y = np.zeros(W, dtype=int)
    for x in range(W):
        h_val = 710 - 55 * math.sin(x * 0.007) - 35 * math.cos(x * 0.016)
        hill_y[x] = int(np.clip(h_val, 620, 800))

    for x in range(W):
        top = hill_y[x]
        for y in range(top, H):
            t = (y - top) / float(H - top)
            # Dark swamp mud and stagnant water
            r = int(42 - 15 * t)
            g = int(48 - 18 * t)
            b = int(28 - 12 * t)
            h_arr[y, x] = [r, g, b, 255]

    hills_img = Image.fromarray(h_arr, "RGBA")
    h_draw = ImageDraw.Draw(hills_img)

    # Ancient weeping cypress trees with Spanish moss
    tree_xs = [140, 360, 620, 890, 1180, 1460, 1740]
    for tx in tree_xs:
        ty = hill_y[tx]
        # Massive trunk with flared cypress knees
        h_draw.polygon([(tx - 28, ty), (tx - 14, ty - 130), (tx + 14, ty - 130), (tx + 28, ty)], fill=(45, 36, 25, 255))
        # Cypress knees
        h_draw.rectangle([tx - 40, ty - 22, tx - 30, ty], fill=(52, 42, 28, 255))
        h_draw.rectangle([tx + 30, ty - 25, tx + 40, ty], fill=(52, 42, 28, 255))
        
        # Twisted branches
        crown_y = ty - 130
        for b_dir, b_reach in [(-1, 80), (1, 80), (-0.5, 55), (0.5, 55)]:
            bx_end = tx + int(b_dir * b_reach)
            by_end = crown_y - np.random.randint(20, 50)
            h_draw.line([(tx, crown_y), (bx_end, by_end)], fill=(40, 32, 22, 255), width=6)
            
            # Hanging Spanish moss drapes
            for m_step in range(4):
                mx_pos = tx + int(b_dir * b_reach * (m_step / 3.0))
                my_pos = crown_y - 20
                moss_len = np.random.randint(35, 75)
                h_draw.line([(mx_pos, my_pos), (mx_pos + np.random.randint(-4, 4), my_pos + moss_len)],
                            fill=(92, 108, 72, 210), width=3)

    # Glowing Swamp Fireflies / Will-o'-the-wisps
    np.random.seed(333)
    for _ in range(80):
        fx = np.random.randint(40, W - 40)
        fy = np.random.randint(450, 800)
        h_draw.ellipse([fx - 6, fy - 6, fx + 6, fy + 6], fill=(185, 245, 55, 60))
        h_draw.ellipse([fx - 2, fy - 2, fx + 2, fy + 2], fill=(225, 255, 120, 240))

    out_dir = os.path.join(TEMP_DIR, "swamp", "parallax")
    make_seamless_strip(sky_img).save(os.path.join(out_dir, "bg_sky.png"), optimize=True)
    make_seamless_strip(mtn_img).save(os.path.join(out_dir, "bg_mountains.png"), optimize=True)
    make_seamless_strip(hills_img).save(os.path.join(out_dir, "bg_hills.png"), optimize=True)
    print("  [+] Swamp parallax layers saved (3840x1080).")

def main():
    generate_snow_parallax()
    generate_beach_parallax()
    generate_moon_parallax()
    generate_swamp_parallax()
    print("\nAll parallax layers generated successfully across 4 themes!")

if __name__ == "__main__":
    main()
