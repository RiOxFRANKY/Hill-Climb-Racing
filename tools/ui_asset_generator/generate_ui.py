#!/usr/bin/env python3
"""
Generate retro pixel-art UI assets for Hill Climb Qt.
This script creates authentic pixel-art pedals, gauges, banners, panels, coins, and buttons.
All outputs are saved to assets/ui/.
This tool folder can be safely deleted or kept for future asset re-generation.
"""

import os
import math
from PIL import Image, ImageDraw

OUTPUT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "assets", "ui"))
os.makedirs(OUTPUT_DIR, exist_ok=True)

def pixel_scale(img, factor=2):
    return img.resize((img.width * factor, img.height * factor), Image.Resampling.NEAREST)

def draw_beveled_box(draw, x0, y0, x1, y1, bg_color, light_color, dark_color, border_color=(12, 16, 24, 255), border_width=1):
    draw.rectangle([x0, y0, x1, y1], outline=border_color, width=border_width)
    ix0, iy0, ix1, iy1 = x0 + border_width, y0 + border_width, x1 - border_width, y1 - border_width
    draw.line([ix0, iy0, ix1 - 1, iy0], fill=light_color, width=1)
    draw.line([ix0, iy0, ix0, iy1 - 1], fill=light_color, width=1)
    draw.line([ix0, iy1, ix1, iy1], fill=dark_color, width=1)
    draw.line([ix1, iy0, ix1, iy1], fill=dark_color, width=1)
    draw.rectangle([ix0 + 1, iy0 + 1, ix1 - 1, iy1 - 1], fill=bg_color)

def draw_rivet(draw, cx, cy):
    draw.point((cx, cy), fill=(210, 220, 230, 255))
    draw.point((cx - 1, cy), fill=(100, 110, 125, 255))
    draw.point((cx + 1, cy), fill=(70, 78, 90, 255))
    draw.point((cx, cy - 1), fill=(160, 175, 190, 255))
    draw.point((cx, cy + 1), fill=(40, 48, 58, 255))

def draw_nail(draw, cx, cy):
    draw.rectangle([cx - 1, cy - 1, cx + 1, cy + 1], fill=(35, 28, 24, 255))
    draw.point((cx - 1, cy - 1), fill=(105, 95, 85, 255))
    draw.point((cx + 1, cy + 1), fill=(15, 12, 10, 255))

# --------------------------------------------------------------------------
# 1. BRAKE PEDAL (Unpressed & Pressed) - 125 x 60 scaled to 250 x 120
# --------------------------------------------------------------------------
def generate_brake_pedal(pressed=False):
    W, H = 125, 60
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    y_off = 3 if pressed else 0

    draw_beveled_box(d, 12, 6 + y_off, 112, 54 + y_off,
                    bg_color=(28, 34, 44, 230),
                    light_color=(60, 72, 90, 255),
                    dark_color=(15, 18, 24, 255),
                    border_color=(8, 10, 14, 255))

    plate_c = (55, 62, 75, 255) if not pressed else (80, 45, 45, 255)
    light_c = (110, 122, 140, 255) if not pressed else (180, 90, 90, 255)
    dark_c = (25, 30, 38, 255) if not pressed else (45, 20, 20, 255)
    draw_beveled_box(d, 16, 10 + y_off, 108, 50 + y_off,
                    bg_color=plate_c, light_color=light_c, dark_color=dark_c,
                    border_color=(10, 12, 16, 255))

    rib_color = (18, 22, 28, 255) if not pressed else (220, 50, 40, 255)
    rib_light = (90, 100, 115, 255) if not pressed else (255, 120, 100, 255)
    for x in range(24, 101, 8):
        d.line([x, 14 + y_off, x + 3, 46 + y_off], fill=rib_color, width=2)
        d.line([x + 1, 14 + y_off, x + 4, 46 + y_off], fill=rib_light, width=1)

    for rx, ry in [(20, 14 + y_off), (104, 14 + y_off), (20, 46 + y_off), (104, 46 + y_off)]:
        draw_rivet(d, rx, ry)

    led_bg = (180, 30, 30, 255) if pressed else (60, 20, 20, 255)
    led_light = (255, 90, 80, 255) if pressed else (120, 40, 40, 255)
    d.rectangle([35, 47 + y_off, 89, 49 + y_off], fill=led_bg)
    d.line([36, 47 + y_off, 88, 47 + y_off], fill=led_light)

    return pixel_scale(img, 2)

# --------------------------------------------------------------------------
# 2. GAS PEDAL (Unpressed & Pressed) - 125 x 60 scaled to 250 x 120
# --------------------------------------------------------------------------
def generate_gas_pedal(pressed=False):
    W, H = 125, 60
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    y_off = 3 if pressed else 0

    draw_beveled_box(d, 12, 6 + y_off, 112, 54 + y_off,
                    bg_color=(28, 34, 44, 230),
                    light_color=(60, 72, 90, 255),
                    dark_color=(15, 18, 24, 255),
                    border_color=(8, 10, 14, 255))

    plate_c = (55, 62, 75, 255) if not pressed else (45, 75, 50, 255)
    light_c = (110, 122, 140, 255) if not pressed else (90, 180, 100, 255)
    dark_c = (25, 30, 38, 255) if not pressed else (20, 45, 25, 255)
    draw_beveled_box(d, 16, 10 + y_off, 108, 50 + y_off,
                    bg_color=plate_c, light_color=light_c, dark_color=dark_c,
                    border_color=(10, 12, 16, 255))

    slot_c = (15, 18, 24, 255) if not pressed else (50, 220, 80, 255)
    slot_hi = (90, 100, 115, 255) if not pressed else (130, 255, 150, 255)
    for col in range(24, 102, 10):
        for row in [18 + y_off, 28 + y_off, 38 + y_off]:
            d.rectangle([col, row, col + 4, row + 5], fill=slot_c)
            d.point((col + 1, row + 1), fill=slot_hi)

    for rx, ry in [(20, 14 + y_off), (104, 14 + y_off), (20, 46 + y_off), (104, 46 + y_off)]:
        draw_rivet(d, rx, ry)

    bar_c = (60, 230, 90, 255) if pressed else (25, 70, 35, 255)
    bar_hi = (150, 255, 170, 255) if pressed else (45, 110, 60, 255)
    d.rectangle([35, 11 + y_off, 89, 13 + y_off], fill=bar_c)
    d.line([36, 11 + y_off, 88, 11 + y_off], fill=bar_hi)

    return pixel_scale(img, 2)

# --------------------------------------------------------------------------
# 3. BOOST PEDAL / BUTTON (Unpressed & Pressed) - 143 x 60 scaled to 286 x 120
# --------------------------------------------------------------------------
def generate_boost_pedal(pressed=False):
    W, H = 143, 60
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    y_off = 3 if pressed else 0

    draw_beveled_box(d, 8, 6 + y_off, 134, 54 + y_off,
                    bg_color=(25, 30, 40, 240),
                    light_color=(70, 85, 105, 255),
                    dark_color=(12, 16, 22, 255),
                    border_color=(6, 8, 12, 255))

    for i in range(3):
        lx = 12 + i * 5
        d.polygon([(lx, 10 + y_off), (lx + 3, 10 + y_off), (lx, 50 + y_off)], fill=(225, 175, 30, 255))
        rx = 118 + i * 5
        d.polygon([(rx, 10 + y_off), (rx + 3, 10 + y_off), (rx, 50 + y_off)], fill=(225, 175, 30, 255))

    plate_c = (40, 50, 65, 255) if not pressed else (20, 80, 110, 255)
    light_c = (80, 100, 130, 255) if not pressed else (60, 210, 255, 255)
    dark_c = (15, 20, 28, 255) if not pressed else (10, 40, 60, 255)
    draw_beveled_box(d, 32, 10 + y_off, 110, 50 + y_off,
                    bg_color=plate_c, light_color=light_c, dark_color=dark_c,
                    border_color=(8, 12, 18, 255))

    glow = (45, 210, 255, 255) if pressed else (30, 95, 125, 255)
    glow_hi = (180, 245, 255, 255) if pressed else (70, 160, 195, 255)

    for y in [18 + y_off, 26 + y_off, 34 + y_off, 42 + y_off]:
        d.line([38, y, 104, y], fill=(15, 20, 26, 255), width=2)
        d.line([38, y - 1, 104, y - 1], fill=(glow[0]//2, glow[1]//2, glow[2]//2, 255), width=1)

    d.rectangle([54, 21 + y_off, 88, 39 + y_off], fill=(12, 18, 26, 255), outline=(6, 10, 15, 255))
    d.rectangle([56, 23 + y_off, 86, 37 + y_off], fill=glow)
    d.line([58, 25 + y_off, 84, 25 + y_off], fill=glow_hi)

    for rx, ry in [(12, 10 + y_off), (130, 10 + y_off), (12, 50 + y_off), (130, 50 + y_off)]:
        draw_rivet(d, rx, ry)

    return pixel_scale(img, 2)

# --------------------------------------------------------------------------
# 4. SPEEDOMETER DIAL & SHORT NEEDLE (Fits inside, no clip, no center bar)
# --------------------------------------------------------------------------
def generate_speedometer_dial():
    SIZE = 76
    img = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    cx, cy = SIZE // 2, SIZE // 2
    r_outer = 36
    r_inner = 32

    for y in range(SIZE):
        for x in range(SIZE):
            dist = math.hypot(x - cx, y - cy)
            if dist <= r_outer:
                if dist > r_outer - 2:
                    d.point((x, y), fill=(8, 12, 16, 255))
                elif dist > r_outer - 4:
                    val = 170 if y < cy else 75
                    d.point((x, y), fill=(val, val + 10, val + 20, 255))
                elif dist > r_inner:
                    d.point((x, y), fill=(35, 42, 54, 255))
                else:
                    d.point((x, y), fill=(16, 20, 28, 245))

    for sx, sy in [(6, 6), (SIZE - 7, 6), (6, SIZE - 7), (SIZE - 7, SIZE - 7)]:
        draw_rivet(d, sx, sy)

    for speed in range(0, 141, 10):
        angle_deg = -135 + (speed / 140.0) * 270.0
        rad = math.radians(angle_deg - 90)
        c = math.cos(rad)
        s = math.sin(rad)

        tick_len = 5 if speed % 20 == 0 else 3
        x1 = cx + int((r_inner - 2) * c)
        y1 = cy + int((r_inner - 2) * s)
        x2 = cx + int((r_inner - 2 - tick_len) * c)
        y2 = cy + int((r_inner - 2 - tick_len) * s)

        col = (240, 50, 40, 255) if speed >= 100 else (210, 225, 240, 255)
        d.line([x1, y1, x2, y2], fill=col, width=1)

    return pixel_scale(img, 2)

def generate_speedometer_needle():
    W, H = 10, 28
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    pivot_x, pivot_y = 5, 22

    needle_poly = [(5, 3), (7, 20), (3, 20)]
    d.polygon(needle_poly, fill=(245, 60, 40, 255))
    d.line([(5, 3), (4, 20)], fill=(255, 180, 90, 255))
    d.line([(6, 20), (5, 3)], fill=(160, 30, 20, 255))

    d.ellipse([pivot_x - 3, pivot_y - 3, pivot_x + 3, pivot_y + 3], fill=(30, 35, 45, 255), outline=(10, 14, 20, 255))
    d.ellipse([pivot_x - 1, pivot_y - 1, pivot_x + 1, pivot_y + 1], fill=(210, 220, 235, 255))

    return pixel_scale(img, 2)

# --------------------------------------------------------------------------
# 5. RUSTIC WORN-DOWN HANGING WOODEN BOARD & CHAINS
# --------------------------------------------------------------------------
def generate_wooden_board():
    W, H = 360, 64
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    # 3 rustic, hand-hewn planks with uneven lengths, slight individual tilts and chipped corners
    # (x0, y0_left, x1, y1_right, height, shade, left_notch, right_notch)
    plank_defs = [
        # Top plank: slightly recessed on left, juts out right
        {"x0": 8, "y0": 3, "x1": 352, "y1": 2, "th": 18, "shade": 118, "tilt": -0.6},
        # Middle plank: juts far out on left, uneven chipped ends
        {"x0": 2, "y0": 20, "x1": 358, "y1": 21, "th": 21, "shade": 104, "tilt": 0.4},
        # Bottom plank: recessed, irregular right end
        {"x0": 12, "y0": 41, "x1": 349, "y1": 40, "th": 19, "shade": 122, "tilt": -0.3},
    ]

    for p in plank_defs:
        x0, y0, x1, y1 = p["x0"], p["y0"], p["x1"], p["y1"]
        th = p["th"]
        shade = p["shade"]
        
        # Draw plank polygon with rugged ends
        poly = [
            (x0 + 1, y0 + 1),
            (x1 - 2, y1),
            (x1, y1 + 3),
            (x1 - 1, y1 + th - 2),
            (x1 - 3, y1 + th),
            (x0 + 2, y0 + th),
            (x0, y0 + th - 3),
            (x0, y0 + 3),
        ]
        
        # Dark bark border / outline
        d.polygon(poly, fill=(38, 20, 10, 255))
        
        # Wood body
        inner_poly = [
            (x0 + 2, y0 + 2),
            (x1 - 3, y1 + 1),
            (x1 - 2, y1 + 3),
            (x1 - 2, y1 + th - 3),
            (x1 - 4, y1 + th - 1),
            (x0 + 3, y0 + th - 1),
            (x0 + 1, y0 + th - 3),
            (x0 + 1, y0 + 3),
        ]
        d.polygon(inner_poly, fill=(shade, shade - 45, shade - 75, 255))
        
        # Top bevel light edge
        d.line([(x0 + 2, y0 + 2), (x1 - 3, y1 + 1)], fill=(shade + 42, shade - 10, shade - 38, 255))
        # Bottom shadow line
        d.line([(x0 + 3, y0 + th - 1), (x1 - 4, y1 + th - 1)], fill=(45, 22, 12, 255))

        # Weathered wood grain cracks and rings
        for gx in range(x0 + 14, x1 - 24, 32):
            d.line([gx, y0 + 4, gx + 20, y0 + 4], fill=(shade - 22, shade - 62, shade - 90, 255))
            d.line([gx + 5, y0 + 8, gx + 25, y0 + 8], fill=(shade + 24, shade - 22, shade - 50, 255))
            d.line([gx + 2, y0 + 12, gx + 16, y0 + 12], fill=(shade - 20, shade - 60, shade - 88, 255))
            if (gx // 32) % 2 == 0:
                d.point((gx + 10, y0 + 6), fill=(30, 15, 8, 255))

        # Chipped notches on outer edges
        d.point((x0 + 1, y0 + 6), fill=(0, 0, 0, 0))
        d.point((x1 - 2, y1 + 8), fill=(0, 0, 0, 0))
        d.point((x1 - 1, y1 + 9), fill=(0, 0, 0, 0))

        # Forged iron nails with slight asymmetry
        draw_nail(d, x0 + 8, y0 + th // 2)
        draw_nail(d, x1 - 8, y1 + th // 2)

    # Forged iron hanging brackets with mounting rivets
    for bx in [56, W - 58]:
        d.rectangle([bx - 6, 0, bx + 6, 14], fill=(42, 48, 58, 255), outline=(16, 20, 26, 255))
        d.line([bx - 5, 1, bx + 5, 1], fill=(110, 125, 145, 255))
        d.point((bx, 7), fill=(210, 220, 230, 255))
        d.rectangle([bx - 3, 0, bx + 3, 3], fill=(26, 32, 40, 255))

    return pixel_scale(img, 2)

def generate_chain_link():
    W, H = 8, 16
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    d.ellipse([0, 0, W - 1, H - 1], fill=(45, 52, 64, 255), outline=(15, 18, 24, 255))
    d.ellipse([2, 3, W - 3, H - 4], fill=(0, 0, 0, 0), outline=(15, 18, 24, 255))
    d.line([1, 2, 1, H - 3], fill=(110, 125, 145, 255))
    d.line([1, 1, W - 2, 1], fill=(110, 125, 145, 255))
    d.line([W - 2, 2, W - 2, H - 3], fill=(25, 30, 38, 255))
    d.line([1, H - 2, W - 2, H - 2], fill=(25, 30, 38, 255))

    return pixel_scale(img, 2)

# --------------------------------------------------------------------------
# 6. SHINY 3D PIXEL COIN (Both ground pickup & HUD counter icon)
# --------------------------------------------------------------------------
def generate_coin(size=48):
    # Generates a round pixel-art gold coin with 3D bottom rim thickness and glint
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    margin = 3
    rim_offset = 3
    x0, y0 = margin, margin
    x1, y1 = size - margin, size - margin

    # 1. Dark bottom-right isometric rim for physical 3D thickness
    d.ellipse([x0, y0 + rim_offset, x1, y1 + rim_offset], fill=(155, 95, 15, 255), outline=(95, 55, 10, 255))

    # 2. Main coin face
    d.ellipse([x0, y0, x1, y1], fill=(255, 205, 30, 255), outline=(125, 75, 10, 255))

    # 3. Inner embossed circular rim
    inset = 5
    d.ellipse([x0 + inset, y0 + inset, x1 - inset, y1 - inset], outline=(220, 160, 20, 255))

    # 4. Shiny top-left glint highlight arc
    d.arc([x0 + 1, y0 + 1, x1 - 1, y1 - 1], 115, 240, fill=(255, 250, 175, 255), width=2)

    # 5. Center embossed 'C'
    cx, cy = size // 2, size // 2
    r_c = 7
    d.line([cx + 3, cy - r_c, cx - 2, cy - r_c + 2], fill=(150, 90, 10, 255), width=2)
    d.line([cx - 2, cy - r_c + 2, cx - 2, cy + r_c - 2], fill=(150, 90, 10, 255), width=2)
    d.line([cx - 2, cy + r_c - 2, cx + 3, cy + r_c], fill=(150, 90, 10, 255), width=2)
    # Inner shine on C
    d.line([cx - 1, cy - r_c + 3, cx - 1, cy + r_c - 3], fill=(255, 240, 130, 255), width=1)

    return img

# --------------------------------------------------------------------------
# 7. HUD PANEL, DIALOG BOX & BUTTON
# --------------------------------------------------------------------------
def generate_hud_panel():
    W, H = 260, 82
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    draw_beveled_box(d, 0, 0, W - 1, H - 1,
                    bg_color=(20, 26, 36, 235),
                    light_color=(60, 75, 96, 255),
                    dark_color=(10, 14, 20, 255),
                    border_color=(6, 8, 12, 255),
                    border_width=2)

    draw_beveled_box(d, 10, 8, 112, 48,
                    bg_color=(12, 16, 24, 255),
                    light_color=(24, 32, 44, 255),
                    dark_color=(45, 56, 72, 255),
                    border_color=(8, 10, 15, 255))

    draw_beveled_box(d, 118, 8, 182, 48,
                    bg_color=(12, 16, 24, 255),
                    light_color=(24, 32, 44, 255),
                    dark_color=(45, 56, 72, 255),
                    border_color=(8, 10, 15, 255))

    draw_beveled_box(d, 188, 8, 249, 48,
                    bg_color=(12, 16, 24, 255),
                    light_color=(24, 32, 44, 255),
                    dark_color=(45, 56, 72, 255),
                    border_color=(8, 10, 15, 255))

    draw_beveled_box(d, 48, 54, 249, 73,
                    bg_color=(10, 14, 20, 255),
                    light_color=(24, 32, 44, 255),
                    dark_color=(40, 50, 68, 255),
                    border_color=(6, 8, 12, 255))

    for bx, by in [(5, 5), (W - 6, 5), (5, H - 6), (W - 6, H - 6)]:
        draw_rivet(d, bx, by)

    return pixel_scale(img, 2)

def generate_dialog_box():
    W, H = 350, 195
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    draw_beveled_box(d, 0, 0, W - 1, H - 1,
                    bg_color=(16, 22, 32, 245),
                    light_color=(70, 90, 115, 255),
                    dark_color=(8, 10, 16, 255),
                    border_color=(5, 7, 10, 255),
                    border_width=3)

    draw_beveled_box(d, 12, 12, W - 13, H - 13,
                    bg_color=(10, 14, 22, 250),
                    light_color=(24, 32, 44, 255),
                    dark_color=(45, 60, 80, 255),
                    border_color=(6, 8, 12, 255),
                    border_width=2)

    for x in [7, W - 8]:
        for y in [7, H - 8]:
            draw_rivet(d, x, y)
        draw_rivet(d, x, H // 2)

    return pixel_scale(img, 2)

def generate_button(pressed=False):
    W, H = 150, 37
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    y_off = 2 if pressed else 0
    bg = (180, 45, 35, 255) if not pressed else (130, 25, 20, 255)
    light = (245, 105, 80, 255) if not pressed else (180, 60, 50, 255)
    dark = (90, 18, 15, 255) if not pressed else (60, 10, 10, 255)

    draw_beveled_box(d, 2, 2 + y_off, W - 3, H - 3 + y_off,
                    bg_color=bg, light_color=light, dark_color=dark,
                    border_color=(15, 6, 6, 255),
                    border_width=2)

    d.line([6, 5 + y_off, W - 7, 5 + y_off], fill=(255, 150, 130, 255))
    return pixel_scale(img, 2)


def main():
    print("Generating pixel art UI assets in:", OUTPUT_DIR)

    assets = {
        "pedal_brake.png": generate_brake_pedal(pressed=False),
        "pedal_brake_pressed.png": generate_brake_pedal(pressed=True),
        "pedal_gas.png": generate_gas_pedal(pressed=False),
        "pedal_gas_pressed.png": generate_gas_pedal(pressed=True),
        "pedal_boost.png": generate_boost_pedal(pressed=False),
        "pedal_boost_pressed.png": generate_boost_pedal(pressed=True),
        "speedometer_dial.png": generate_speedometer_dial(),
        "speedometer_needle.png": generate_speedometer_needle(),
        "wooden_board.png": generate_wooden_board(),
        "chain_link.png": generate_chain_link(),
        "coin.png": generate_coin(48),
        "coin_icon.png": generate_coin(28),
        "hud_panel.png": generate_hud_panel(),
        "dialog_box.png": generate_dialog_box(),
        "dialog_button.png": generate_button(pressed=False),
        "dialog_button_pressed.png": generate_button(pressed=True),
    }

    for name, img in assets.items():
        filepath = os.path.join(OUTPUT_DIR, name)
        img.save(filepath)
        print(f"  [+] Wrote {name} ({img.width}x{img.height})")

    print("All pixel art UI assets generated successfully!")

if __name__ == "__main__":
    main()
