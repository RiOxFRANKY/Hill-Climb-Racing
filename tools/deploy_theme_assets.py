#!/usr/bin/env python3
"""
deploy_theme_assets.py
Generates and deploys:
1. 3-layer parallax backgrounds (bg_sky, bg_mountains, bg_hills, background.png) for Beach, Swamp, Snow, Moon into assets/
2. Themed vehicle sprites (car_body.png, wheel.png) for Beach, Swamp, Snow, Moon into assets/
3. Authentic pixel-art terrain icons (terrain_grassland.png, terrain_beach.png, terrain_swamp.png, terrain_snow.png, terrain_moon.png) into assets/ui/ and assets/
4. Re-usable terrain top ribbons and fill tiles into assets/terrain/
"""

import os
import shutil
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS = os.path.join(ROOT, "assets")
TEMP = os.path.join(ROOT, "temp")

THEMES = ["grassland", "beach", "swamp", "snow", "moon"]

def ensure_dirs():
    for theme in ["beach", "swamp", "snow", "moon", "grassland"]:
        os.makedirs(os.path.join(ASSETS, theme), exist_ok=True)
    os.makedirs(os.path.join(ASSETS, "ui"), exist_ok=True)
    os.makedirs(os.path.join(ASSETS, "terrain"), exist_ok=True)

def deploy_parallax_and_vehicles():
    print("Deploying parallax backgrounds and vehicles to assets/...")
    
    # Grassland is already in root assets, also put copy in assets/grassland for consistency
    for f in ["bg_sky.png", "bg_mountains.png", "bg_hills.png", "background.png", "car_body.png", "wheel.png"]:
        src = os.path.join(ASSETS, f)
        dst = os.path.join(ASSETS, "grassland", f)
        if os.path.exists(src):
            shutil.copy2(src, dst)

    for theme in ["beach", "swamp", "snow", "moon"]:
        t_src_parallax = os.path.join(TEMP, theme, "parallax")
        t_dst = os.path.join(ASSETS, theme)
        
        # Copy 3 parallax layers
        for layer in ["bg_sky.png", "bg_mountains.png", "bg_hills.png"]:
            src_f = os.path.join(t_src_parallax, layer)
            dst_f = os.path.join(t_dst, layer)
            if os.path.exists(src_f):
                shutil.copy2(src_f, dst_f)
                print(f"  [+] Copied {theme}/{layer}")

        # Create composite 1920x1080 background.png from the first 1920 columns of the 3 layers
        sky = Image.open(os.path.join(t_dst, "bg_sky.png")).crop((0, 0, 1920, 1080)).convert("RGBA")
        mtn = Image.open(os.path.join(t_dst, "bg_mountains.png")).crop((0, 0, 1920, 1080)).convert("RGBA")
        hills = Image.open(os.path.join(t_dst, "bg_hills.png")).crop((0, 0, 1920, 1080)).convert("RGBA")
        
        comp = Image.alpha_composite(sky, mtn)
        comp = Image.alpha_composite(comp, hills)
        comp.convert("RGB").save(os.path.join(t_dst, "background.png"), optimize=True)
        print(f"  [+] Created {theme}/background.png (1920x1080)")

        # Copy themed car_body and wheel
        t_src_veh = os.path.join(TEMP, theme, "vehicle")
        for v in ["car_body.png", "wheel.png"]:
            src_v = os.path.join(t_src_veh, v)
            dst_v = os.path.join(t_dst, v)
            if os.path.exists(src_v):
                shutil.copy2(src_v, dst_v)
                print(f"  [+] Copied {theme}/{v}")

def create_terrain_icons():
    print("Generating authentic terrain badges and material icons...")
    
    # Palettes for the 5 themes:
    # (outline, highlight, surface_main, surface_speck, surface_shade, surface_edge, subsoil_main, subsoil_dark, subsoil_light, bg_sky)
    palettes = {
        "grassland": {
            "name": "Grassland",
            "outline": (31, 58, 20),
            "highlight": (178, 226, 60),
            "surface_main": (122, 194, 33),
            "surface_speck": (96, 165, 27),
            "surface_shade": (84, 150, 26),
            "surface_edge": (46, 98, 22),
            "subsoil_main": (156, 90, 38),
            "subsoil_dark": (132, 74, 30),
            "subsoil_light": (172, 104, 48),
            "decor": "pebbles",
            "bg": (145, 210, 255),
        },
        "beach": {
            "name": "Beach",
            "outline": (140, 110, 50),
            "highlight": (255, 245, 180),
            "surface_main": (235, 195, 115),
            "surface_speck": (245, 215, 140),
            "surface_shade": (215, 175, 95),
            "surface_edge": (185, 145, 80),
            "subsoil_main": (195, 150, 95),
            "subsoil_dark": (165, 120, 75),
            "subsoil_light": (215, 170, 115),
            "decor": "shells",
            "bg": (120, 205, 255),
        },
        "swamp": {
            "name": "Swamp",
            "outline": (25, 40, 20),
            "highlight": (135, 170, 60),
            "surface_main": (85, 115, 45),
            "surface_speck": (105, 135, 55),
            "surface_shade": (65, 90, 35),
            "surface_edge": (45, 60, 25),
            "subsoil_main": (60, 48, 36),
            "subsoil_dark": (42, 32, 24),
            "subsoil_light": (78, 62, 46),
            "decor": "roots",
            "bg": (95, 115, 80),
        },
        "snow": {
            "name": "Arctic",
            "outline": (110, 150, 185),
            "highlight": (255, 255, 255),
            "surface_main": (230, 242, 255),
            "surface_speck": (245, 250, 255),
            "surface_shade": (200, 222, 245),
            "surface_edge": (150, 185, 220),
            "subsoil_main": (95, 120, 150),
            "subsoil_dark": (70, 90, 120),
            "subsoil_light": (120, 145, 175),
            "decor": "crystals",
            "bg": (45, 75, 130),
        },
        "moon": {
            "name": "Moon",
            "outline": (55, 60, 70),
            "highlight": (225, 230, 240),
            "surface_main": (175, 180, 190),
            "surface_speck": (200, 205, 215),
            "surface_shade": (140, 145, 155),
            "surface_edge": (110, 115, 125),
            "subsoil_main": (80, 85, 95),
            "subsoil_dark": (55, 60, 70),
            "subsoil_light": (105, 110, 120),
            "decor": "craters",
            "bg": (12, 14, 25),
        },
    }

    size = 128
    for key, p in palettes.items():
        icon = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        draw = ImageDraw.Draw(icon)
        
        # Outer border / badge backing: rounded pixel rectangle
        # Border
        draw.rounded_rectangle([2, 2, size - 3, size - 3], radius=16, fill=(18, 22, 28, 255), outline=(50, 60, 75, 255), width=3)
        # Inner sky background
        draw.rounded_rectangle([6, 6, size - 7, size - 7], radius=13, fill=(*p["bg"], 255))
        
        # Sun / Moon / Star in the icon sky
        if key == "grassland":
            draw.ellipse([88, 14, 112, 38], fill=(255, 230, 80, 255))
            # Cloud
            draw.ellipse([20, 22, 45, 36], fill=(255, 255, 255, 220))
            draw.ellipse([35, 18, 65, 38], fill=(255, 255, 255, 240))
        elif key == "beach":
            # Bright tropical sun with corona
            draw.ellipse([82, 10, 116, 44], fill=(255, 250, 160, 120))
            draw.ellipse([88, 16, 110, 38], fill=(255, 255, 220, 255))
            # Distant seagulls
            draw.arc([30, 22, 42, 30], 180, 360, fill=(60, 80, 110, 255), width=2)
            draw.arc([42, 22, 54, 30], 180, 360, fill=(60, 80, 110, 255), width=2)
        elif key == "swamp":
            # Eerie pale moon
            draw.ellipse([85, 14, 110, 39], fill=(210, 225, 170, 220))
            # Fog bands
            draw.rectangle([10, 34, 118, 42], fill=(80, 105, 75, 120))
        elif key == "snow":
            # Aurora ribbon
            draw.polygon([(10, 10), (45, 8), (118, 25), (118, 38), (55, 22), (10, 28)], fill=(80, 240, 180, 140))
            draw.ellipse([92, 14, 112, 34], fill=(235, 245, 255, 240))
            draw.ellipse([98, 12, 118, 32], fill=(45, 75, 130, 255)) # crescent cutout
        elif key == "moon":
            # Deep space with stars & tiny Earth
            draw.ellipse([90, 14, 110, 34], fill=(30, 120, 220, 255)) # Earth
            draw.arc([92, 18, 108, 30], 40, 160, fill=(255, 255, 255, 200), width=3)
            # Stars
            for sx, sy in [(18, 14), (35, 28), (55, 12), (75, 32), (25, 42)]:
                draw.rectangle([sx, sy, sx+1, sy+1], fill=(255, 255, 255, 255))

        # Terrain cross section curve
        # Curve points across x=6 to x=121
        pts = []
        for x in range(6, size - 6):
            # Hill sine contour
            nx = (x - 6) / float(size - 12)
            h_y = 58 - 14 * np.sin(nx * np.pi * 1.5 + 0.3)
            pts.append((x, int(h_y)))

        # Fill subterranean soil
        poly_soil = [(6, size - 7)] + pts + [(size - 7, size - 7)]
        draw.polygon(poly_soil, fill=p["subsoil_main"])

        # Subsoil strata and speckles
        np.random.seed(hash(key) % 10000)
        for y in range(65, size - 8):
            for x in range(8, size - 8):
                # Only if below terrain curve
                nx = (x - 6) / float(size - 12)
                cur_hy = 58 - 14 * np.sin(nx * np.pi * 1.5 + 0.3)
                if y > cur_hy + 16:
                    rn = np.random.randint(0, 16)
                    if rn == 0:
                        draw.point((x, y), fill=p["subsoil_dark"])
                    elif rn == 1:
                        draw.point((x, y), fill=p["subsoil_light"])

        # Upper surface strip (e.g. grass, sand, moss, snow, lunar dust)
        for i, (x, y) in enumerate(pts):
            # Outline pixel at surface
            draw.line([(x, y), (x, y + 2)], fill=p["outline"])
            # Highlight pixel
            draw.line([(x, y + 2), (x, y + 4)], fill=p["highlight"])
            # Surface layer
            surf_depth = int(12 + 2 * np.sin(i * 0.4))
            for sy in range(y + 4, y + surf_depth):
                rn = np.random.randint(0, 5)
                col = p["surface_main"] if rn < 3 else (p["surface_speck"] if rn == 3 else p["surface_shade"])
                draw.point((x, sy), fill=col)
            # Edge transition
            draw.line([(x, y + surf_depth), (x, y + surf_depth + 2)], fill=p["surface_edge"])

        # Decor in subsoil / surface
        if p["decor"] == "pebbles":
            # Small stones in soil
            draw.rectangle([34, 85, 38, 88], fill=(66, 62, 70))
            draw.rectangle([78, 92, 84, 96], fill=(110, 60, 24))
            draw.rectangle([79, 93, 83, 95], fill=(222, 158, 96))
        elif p["decor"] == "shells":
            # Sea shells / coral bits
            draw.polygon([(40, 88), (44, 82), (48, 88)], fill=(255, 230, 220))
            draw.polygon([(78, 95), (83, 90), (88, 95)], fill=(245, 190, 180))
        elif p["decor"] == "roots":
            # Tangled tree roots
            draw.line([(45, 78), (52, 90), (62, 96)], fill=(32, 24, 18), width=3)
            draw.line([(52, 90), (50, 102)], fill=(42, 32, 24), width=2)
        elif p["decor"] == "crystals":
            # Ice crystals / glacial frost
            draw.polygon([(42, 85), (46, 78), (50, 85), (46, 92)], fill=(180, 225, 255))
            draw.polygon([(78, 92), (81, 86), (84, 92), (81, 98)], fill=(210, 240, 255))
        elif p["decor"] == "craters":
            # Lunar impact craters & basalt chunks
            draw.ellipse([38, 84, 52, 92], fill=(45, 48, 55))
            draw.arc([38, 84, 52, 92], 0, 180, fill=(185, 190, 200), width=2)
            draw.ellipse([76, 94, 86, 100], fill=(50, 52, 60))

        # Re-enforce clean outer frame
        draw.rounded_rectangle([2, 2, size - 3, size - 3], radius=16, outline=(255, 210, 50, 255), width=2)
        
        # Save to assets/ui/ and assets/
        ui_path = os.path.join(ASSETS, "ui", f"map_icon_{key}.png")
        root_path = os.path.join(ASSETS, f"terrain_{key}.png")
        theme_path = os.path.join(ASSETS, key, "terrain.png")
        
        icon.save(ui_path, optimize=True)
        icon.save(root_path, optimize=True)
        icon.save(theme_path, optimize=True)
        print(f"  [+] Saved terrain icon: {ui_path}")

        # Also create a 256x256 high-res version for large displays
        icon_256 = icon.resize((256, 256), Image.Resampling.NEAREST)
        icon_256.save(os.path.join(ASSETS, "ui", f"map_icon_{key}_256.png"), optimize=True)

    # Generate tileable terrain textures for assets/terrain/
    for key, p in palettes.items():
        # Top ribbon tile (64 x 32)
        top_tile = Image.new("RGBA", (64, 32), (0, 0, 0, 0))
        d_top = ImageDraw.Draw(top_tile)
        d_top.rectangle([0, 0, 63, 1], fill=p["outline"])
        d_top.rectangle([0, 2, 63, 4], fill=p["highlight"])
        for y in range(5, 26):
            for x in range(64):
                rn = np.random.randint(0, 5)
                c = p["surface_main"] if rn < 3 else (p["surface_speck"] if rn == 3 else p["surface_shade"])
                d_top.point((x, y), fill=c)
        d_top.rectangle([0, 27, 63, 29], fill=p["surface_edge"])
        top_tile.save(os.path.join(ASSETS, "terrain", f"{key}_top.png"), optimize=True)

        # Fill tile (64 x 64)
        fill_tile = Image.new("RGBA", (64, 64), p["subsoil_main"])
        d_fill = ImageDraw.Draw(fill_tile)
        for y in range(64):
            for x in range(64):
                rn = np.random.randint(0, 18)
                if rn == 0:
                    d_fill.point((x, y), fill=p["subsoil_dark"])
                elif rn == 1:
                    d_fill.point((x, y), fill=p["subsoil_light"])
        fill_tile.save(os.path.join(ASSETS, "terrain", f"{key}_fill.png"), optimize=True)

    # Keep compatibility with old grass_top.png and dirt_fill.png
    shutil.copy2(os.path.join(ASSETS, "terrain", "grassland_top.png"), os.path.join(ASSETS, "terrain", "grass_top.png"))
    shutil.copy2(os.path.join(ASSETS, "terrain", "grassland_fill.png"), os.path.join(ASSETS, "terrain", "dirt_fill.png"))
    print("  [+] Saved terrain texture tiles in assets/terrain/")

if __name__ == "__main__":
    ensure_dirs()
    deploy_parallax_and_vehicles()
    create_terrain_icons()
    print("All theme assets successfully generated and deployed!")
