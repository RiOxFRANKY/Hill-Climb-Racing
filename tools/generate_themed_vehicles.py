#!/usr/bin/env python3
"""
Generate themed vehicle sprites for Snow, Beach, Moon, and Swamp.
Outputs modified car_body.png (1670x942) and wheel.png (1254x1254) into:
  temp/snow/vehicle/
  temp/beach/vehicle/
  temp/moon/vehicle/
  temp/swamp/vehicle/
"""

import os
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS_DIR = os.path.join(ROOT, "assets")
TEMP_DIR = os.path.join(ROOT, "temp")

def load_base_sprites():
    car = Image.open(os.path.join(ASSETS_DIR, "car_body.png")).convert("RGBA")
    wheel = Image.open(os.path.join(ASSETS_DIR, "wheel.png")).convert("RGBA")
    return car, wheel

def create_snow_vehicle(base_car, base_wheel):
    car = base_car.copy()
    cw, ch = car.size
    c_arr = np.array(car)
    alpha = c_arr[:, :, 3]

    # Find top edge for snow accumulation
    top_y = np.full(cw, ch)
    for x in range(cw):
        ys = np.where(alpha[:, x] > 50)[0]
        if len(ys) > 0:
            top_y[x] = ys.min()

    # Create Snow Layer for Car
    snow_overlay = Image.new("RGBA", (cw, ch), (0, 0, 0, 0))
    s_draw = ImageDraw.Draw(snow_overlay)

    # 1. Snow caps on top surfaces
    # Find contiguous top segments
    for x in range(15, cw - 15):
        ty = top_y[x]
        if ty >= ch - 50:
            continue
        
        # Calculate local slope
        x_left = max(15, x - 10)
        x_right = min(cw - 15, x + 10)
        slope = abs(float(top_y[x_right]) - float(top_y[x_left])) / (x_right - x_left)

        # Snow sticks best to flatter surfaces (hood, roof, spoiler, bumper)
        if slope < 0.8:
            depth = int(14 + 10 * math.sin(x * 0.08) + 8 * math.cos(x * 0.03))
            if x < 400: # rear spoiler / deck
                depth = int(depth * 1.3)
            elif 550 < x < 950: # roof / roll cage
                depth = int(depth * 1.2)
            elif x > 1200: # hood & front bumper
                depth = int(depth * 1.1)

            # Draw snow cap column
            for dy in range(-depth, 16):
                py = ty + dy
                if 0 <= py < ch:
                    if dy < 0:
                        # Snow crest above edge
                        t = (dy + depth) / max(1.0, depth)
                        r = int(245 + 10 * t)
                        g = int(248 + 7 * t)
                        b = 255
                        a = int(255 * min(1.0, t * 1.5))
                        snow_overlay.putpixel((x, py), (r, g, b, a))
                    else:
                        # Snow dusting on surface edge
                        t = dy / 16.0
                        if alpha[py, x] > 50:
                            r = int(220 + 30 * (1 - t))
                            g = int(235 + 20 * (1 - t))
                            b = 255
                            a = int(220 * (1.0 - t * 0.8))
                            snow_overlay.putpixel((x, py), (r, g, b, a))

    # 2. Icicles hanging from front bumper and undercarriage
    icicle_spots = [320, 360, 410, 1320, 1370, 1420, 1480, 1540]
    for spot in icicle_spots:
        by = ch - 1
        ys = np.where(alpha[:, spot] > 100)[0]
        if len(ys) > 0:
            by = ys.max()
            length = int(25 + 15 * math.sin(spot))
            for iy in range(length):
                py = by + iy
                if py < ch:
                    w_ice = max(1, int(4 * (1.0 - iy / length)))
                    for dx in range(-w_ice, w_ice + 1):
                        px = spot + dx
                        if 0 <= px < cw:
                            snow_overlay.putpixel((px, py), (210, 235, 255, int(200 * (1 - iy / length))))

    # 3. Frost glaze on windshield (approx x=950..1200, y=320..500)
    for y in range(320, 500):
        for x in range(950, 1200):
            if alpha[y, x] > 50:
                # Check if it's the windshield (bluish or dark glass)
                r, g, b = c_arr[y, x, :3]
                if abs(int(r) - int(g)) < 50 and b > 40:
                    # Wiper arc
                    dist_to_wiper_pivot = math.sqrt((x - 1180)**2 + (y - 500)**2)
                    wiper_angle = math.atan2(500 - y, 1180 - x)
                    in_wiper_arc = (120 < dist_to_wiper_pivot < 220) and (0.2 < wiper_angle < 1.4)
                    if not in_wiper_arc:
                        # Frost crystals
                        noise = (math.sin(x * 0.3) * math.cos(y * 0.3) + 1) * 0.5
                        if noise > 0.4:
                            snow_overlay.putpixel((x, y), (230, 242, 255, int(160 * noise)))

    car_composite = Image.alpha_composite(car, snow_overlay)

    # Modify Wheel for Snow
    wheel = base_wheel.copy()
    ww, wh = wheel.size
    w_arr = np.array(wheel)
    w_alpha = w_arr[:, :, 3]
    cx, cy = ww // 2, wh // 2

    wheel_snow = Image.new("RGBA", (ww, wh), (0, 0, 0, 0))
    y_coords, x_coords = np.ogrid[:wh, :ww]
    dist_from_center = np.sqrt((x_coords - cx)**2 + (y_coords - cy)**2)
    angle = np.arctan2(y_coords - cy, x_coords - cx)

    # Snow packed in outer tread grooves (r between 480 and 610)
    tread_snow_mask = (dist_from_center >= 470) & (dist_from_center <= 605) & (w_alpha > 50)
    # Lug pattern
    lugs = np.sin(angle * 18) > 0.1
    tread_snow_mask = tread_snow_mask & lugs

    # Frost on wheel rim
    rim_snow_mask = (dist_from_center >= 250) & (dist_from_center <= 300) & (np.sin(angle * 6) > 0.5)

    ws_arr = np.zeros((wh, ww, 4), dtype=np.uint8)
    ws_arr[tread_snow_mask] = [240, 248, 255, 230]
    ws_arr[rim_snow_mask] = [220, 240, 255, 180]

    # Soften edges
    ws_img = Image.fromarray(ws_arr, "RGBA").filter(ImageFilter.GaussianBlur(1.0))
    wheel_composite = Image.alpha_composite(wheel, ws_img)

    return car_composite, wheel_composite


def create_beach_vehicle(base_car, base_wheel):
    car = base_car.copy()
    cw, ch = car.size
    c_arr = np.array(car)
    alpha = c_arr[:, :, 3]

    sand_overlay = Image.new("RGBA", (cw, ch), (0, 0, 0, 0))
    sand_arr = np.zeros((ch, cw, 4), dtype=np.uint8)

    # Sand coats bottom areas, wheel arches, and lower panels
    for y in range(450, ch):
        depth_factor = (y - 450) / float(ch - 450)
        for x in range(cw):
            if alpha[y, x] > 50:
                # Noise pattern for sand grains
                n = (math.sin(x * 0.4 + y * 0.2) + math.cos(x * 0.15 - y * 0.35) + 2) / 4.0
                intensity = depth_factor * (0.6 + 0.4 * n)
                if intensity > 0.25:
                    r = int(225 + 20 * n)
                    g = int(195 + 15 * n)
                    b = int(135 + 10 * n)
                    a = int(min(220, intensity * 240))
                    sand_arr[y, x] = [r, g, b, a]

    # Sand spray along the sides
    np.random.seed(42)
    for _ in range(8000):
        sx = np.random.randint(50, cw - 50)
        sy = np.random.randint(500, ch - 50)
        if alpha[sy, sx] > 50:
            sand_arr[sy, sx] = [230, 205, 145, np.random.randint(140, 240)]

    s_img = Image.fromarray(sand_arr, "RGBA").filter(ImageFilter.GaussianBlur(0.8))
    car_composite = Image.alpha_composite(car, s_img)

    # Wheel for Beach
    wheel = base_wheel.copy()
    ww, wh = wheel.size
    w_arr = np.array(wheel)
    cx, cy = ww // 2, wh // 2

    y_coords, x_coords = np.ogrid[:wh, :ww]
    dist_from_center = np.sqrt((x_coords - cx)**2 + (y_coords - cy)**2)
    angle = np.arctan2(y_coords - cy, x_coords - cx)

    wheel_sand = np.zeros((wh, ww, 4), dtype=np.uint8)
    # Sand embedded in treads
    sand_mask = (dist_from_center >= 460) & (dist_from_center <= 605) & (w_arr[:, :, 3] > 50)
    tread_pat = np.sin(angle * 24) > 0.0
    sand_mask = sand_mask & tread_pat
    wheel_sand[sand_mask] = [220, 190, 130, 210]

    # Fine sand in rim crevice
    rim_sand = (dist_from_center >= 260) & (dist_from_center <= 330) & (w_arr[:, :, 3] > 50) & (np.sin(angle * 8) > 0.2)
    wheel_sand[rim_sand] = [210, 180, 120, 160]

    ws_img = Image.fromarray(wheel_sand, "RGBA").filter(ImageFilter.GaussianBlur(1.0))
    wheel_composite = Image.alpha_composite(wheel, ws_img)

    return car_composite, wheel_composite


def create_swamp_vehicle(base_car, base_wheel):
    car = base_car.copy()
    cw, ch = car.size
    c_arr = np.array(car)
    alpha = c_arr[:, :, 3]

    mud_overlay = Image.new("RGBA", (cw, ch), (0, 0, 0, 0))
    mud_arr = np.zeros((ch, cw, 4), dtype=np.uint8)

    # Heavy viscous dark mud along bottom and wheel wells
    for y in range(400, ch):
        mud_factor = ((y - 400) / float(ch - 400)) ** 1.4
        for x in range(cw):
            if alpha[y, x] > 50:
                n = (math.sin(x * 0.12) * math.cos(y * 0.08) + math.sin(x * 0.05 + y * 0.15) + 2) / 4.0
                thresh = 0.8 - mud_factor * 0.7
                if n > thresh:
                    # Dark wet peat mud
                    darkness = np.random.uniform(0.7, 1.1)
                    r = int(np.clip(62 * darkness, 30, 90))
                    g = int(np.clip(45 * darkness, 20, 70))
                    b = int(np.clip(26 * darkness, 10, 45))
                    a = int(min(250, mud_factor * 255 * (n / thresh)))
                    mud_arr[y, x] = [r, g, b, a]

    # Mud splatters flinging upward
    np.random.seed(101)
    for _ in range(12000):
        mx = np.random.randint(30, cw - 30)
        my = np.random.randint(350, ch - 50)
        if alpha[my, mx] > 50:
            blob_size = np.random.randint(2, 6)
            for dy in range(-blob_size, blob_size + 1):
                for dx in range(-blob_size, blob_size + 1):
                    if dx*dx + dy*dy <= blob_size*blob_size:
                        px, py = mx + dx, my + dy
                        if 0 <= px < cw and 0 <= py < ch and alpha[py, px] > 50:
                            mud_arr[py, px] = [55, 38, 20, np.random.randint(200, 255)]

    # Mud drips
    drip_xs = [380, 520, 750, 920, 1250, 1400, 1520]
    for dx in drip_xs:
        drip_len = np.random.randint(30, 80)
        drip_start = np.random.randint(600, 720)
        for dy in range(drip_len):
            py = drip_start + dy
            if 0 <= py < ch and alpha[py, dx] > 50:
                w_d = max(1, int(3 * (1.0 - dy / drip_len)))
                for ox in range(-w_d, w_d + 1):
                    mud_arr[py, dx + ox] = [50, 34, 18, 240]

    # Wiper arc clearing mud on windshield
    for y in range(320, 500):
        for x in range(950, 1200):
            dist_to_pivot = math.sqrt((x - 1180)**2 + (y - 500)**2)
            angle_w = math.atan2(500 - y, 1180 - x)
            if (120 < dist_to_pivot < 220) and (0.2 < angle_w < 1.4):
                mud_arr[y, x] = [0, 0, 0, 0] # cleared by wiper!

    m_img = Image.fromarray(mud_arr, "RGBA").filter(ImageFilter.GaussianBlur(0.7))
    car_composite = Image.alpha_composite(car, m_img)

    # Wheel for Swamp
    wheel = base_wheel.copy()
    ww, wh = wheel.size
    w_arr = np.array(wheel)
    cx, cy = ww // 2, wh // 2

    y_coords, x_coords = np.ogrid[:wh, :ww]
    dist_from_center = np.sqrt((x_coords - cx)**2 + (y_coords - cy)**2)
    angle = np.arctan2(y_coords - cy, x_coords - cx)

    wheel_mud = np.zeros((wh, ww, 4), dtype=np.uint8)
    # Heavy mud filling treads
    mud_tread = (dist_from_center >= 430) & (dist_from_center <= 610) & (w_arr[:, :, 3] > 50)
    wheel_mud[mud_tread] = [58, 42, 24, 235]

    # Mud splattered on rim spokes
    rim_splat = (dist_from_center < 430) & (w_arr[:, :, 3] > 100) & (np.sin(angle * 7) > 0.4)
    wheel_mud[rim_splat] = [52, 38, 22, 190]

    wm_img = Image.fromarray(wheel_mud, "RGBA").filter(ImageFilter.GaussianBlur(1.2))
    wheel_composite = Image.alpha_composite(wheel, wm_img)

    return car_composite, wheel_composite


def create_moon_vehicle(base_car, base_wheel):
    car = base_car.copy()
    cw, ch = car.size
    c_arr = np.array(car)
    alpha = c_arr[:, :, 3]

    moon_overlay = Image.new("RGBA", (cw, ch), (0, 0, 0, 0))
    m_draw = ImageDraw.Draw(moon_overlay)
    m_arr = np.zeros((ch, cw, 4), dtype=np.uint8)

    # 1. Silvery-gray lunar regolith dust on lower chassis and wheels
    for y in range(480, ch):
        factor = ((y - 480) / float(ch - 480))
        for x in range(cw):
            if alpha[y, x] > 50:
                n = (math.sin(x * 0.3) * math.cos(y * 0.2) + 1) * 0.5
                if n > 0.35:
                    gray = int(185 + 35 * n)
                    m_arr[y, x] = [gray, gray, int(gray * 1.05), int(factor * 210)]

    # 2. Lunar Rover Gold Thermal Foil Patch on door panel (x=680..860, y=520..640)
    for y in range(530, 630):
        for x in range(700, 840):
            if alpha[y, x] > 100:
                # Gold reflective foil texture
                foil_tex = (math.sin(x * 0.8) * math.cos(y * 0.8) + 1) * 0.5
                r = int(220 + 35 * foil_tex)
                g = int(175 + 30 * foil_tex)
                b = int(35 + 20 * foil_tex)
                m_arr[y, x] = [r, g, b, 240]

    # 3. Space agency mission badge & lunar antenna
    # High-gain antenna dish at rear (x=380, y=240..310)
    m_draw.line([(380, 310), (380, 240)], fill=(200, 205, 215, 255), width=4)
    m_draw.arc([(360, 220), (400, 250)], start=180, end=360, fill=(240, 240, 250, 255), width=4)
    m_draw.ellipse([(376, 216), (384, 224)], fill=(255, 50, 50, 255)) # beacon light

    # Combine array and drawn elements
    dust_img = Image.fromarray(m_arr, "RGBA").filter(ImageFilter.GaussianBlur(0.8))
    full_overlay = Image.alpha_composite(dust_img, moon_overlay)
    car_composite = Image.alpha_composite(car, full_overlay)

    # Wheel for Moon
    wheel = base_wheel.copy()
    ww, wh = wheel.size
    w_arr = np.array(wheel)
    cx, cy = ww // 2, wh // 2

    y_coords, x_coords = np.ogrid[:wh, :ww]
    dist_from_center = np.sqrt((x_coords - cx)**2 + (y_coords - cy)**2)
    angle = np.arctan2(y_coords - cy, x_coords - cx)

    wheel_moon = np.zeros((wh, ww, 4), dtype=np.uint8)
    # Fine gray regolith dust in treads
    tread_mask = (dist_from_center >= 460) & (dist_from_center <= 605) & (w_arr[:, :, 3] > 50)
    wheel_moon[tread_mask] = [190, 195, 205, 200]

    # Metallic rim gleam
    rim_mask = (dist_from_center >= 260) & (dist_from_center <= 320) & (w_arr[:, :, 3] > 50) & (np.sin(angle * 4) > 0.3)
    wheel_moon[rim_mask] = [210, 220, 235, 170]

    wm_img = Image.fromarray(wheel_moon, "RGBA").filter(ImageFilter.GaussianBlur(1.0))
    wheel_composite = Image.alpha_composite(wheel, wm_img)

    return car_composite, wheel_composite


def main():
    base_car, base_wheel = load_base_sprites()
    print("Loaded base car:", base_car.size, "and wheel:", base_wheel.size)

    themes = {
        "snow": create_snow_vehicle,
        "beach": create_beach_vehicle,
        "swamp": create_swamp_vehicle,
        "moon": create_moon_vehicle,
    }

    for name, func in themes.items():
        print(f"Generating {name} vehicle sprites...")
        car, wheel = func(base_car, base_wheel)
        out_dir = os.path.join(TEMP_DIR, name, "vehicle")
        os.makedirs(out_dir, exist_ok=True)
        car_path = os.path.join(out_dir, "car_body.png")
        wheel_path = os.path.join(out_dir, "wheel.png")
        car.save(car_path, optimize=True)
        wheel.save(wheel_path, optimize=True)
        print(f"  [+] Saved {car_path} ({car.size})")
        print(f"  [+] Saved {wheel_path} ({wheel.size})")

    print("\nAll themed vehicle sprites generated successfully!")

if __name__ == "__main__":
    main()
