#!/usr/bin/env python3
"""
Generate map-themed ZoneBanner boards (720x128 RGBA) and hanging chain/rope
links (16x32 RGBA) matching the exact proportions and eyelet geometry of
wooden_board.png and chain_link.png:
  - Beach: lighter driftwood board with algae, small corals, and braided ropes
    routed through the four corners & hanging knots
  - Swamp: dark forged iron plate rusty around edges & rusty iron chains
  - Snow / Arctic: darker timber planks with snow cap on top edge & frosted chains
  - Moon: clean light-gray brushed titanium plate & sturdier modern links
"""

import os
import random
from PIL import Image, ImageDraw

OUTPUT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "assets", "ui"))
os.makedirs(OUTPUT_DIR, exist_ok=True)


def pixel_scale(img, factor=2):
    return img.resize((img.width * factor, img.height * factor), Image.Resampling.NEAREST)


def draw_nail(draw, cx, cy, base=(35, 28, 24, 255), hi=(105, 95, 85, 255), sh=(15, 12, 10, 255)):
    draw.rectangle([cx - 1, cy - 1, cx + 1, cy + 1], fill=base)
    draw.point((cx - 1, cy - 1), fill=hi)
    draw.point((cx + 1, cy + 1), fill=sh)


def draw_rivet(draw, cx, cy, hi=(215, 225, 235, 255), mid=(105, 118, 132, 255), dark=(42, 50, 60, 255)):
    draw.point((cx, cy), fill=hi)
    draw.point((cx - 1, cy), fill=mid)
    draw.point((cx + 1, cy), fill=dark)
    draw.point((cx, cy - 1), fill=hi)
    draw.point((cx, cy + 1), fill=dark)


def wood_planks_base(
    W,
    H,
    plank_colors,
    top_hi,
    bot_sh,
    outline,
    grain_dark,
    grain_light,
    nail_colors,
):
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    plank_defs = [
        {"x0": 8, "y0": 3, "x1": 352, "y1": 2, "th": 18},
        {"x0": 2, "y0": 20, "x1": 358, "y1": 21, "th": 21},
        {"x0": 12, "y0": 41, "x1": 349, "y1": 40, "th": 19},
    ]

    for idx, p in enumerate(plank_defs):
        x0, y0, x1, y1, th = p["x0"], p["y0"], p["x1"], p["y1"], p["th"]
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
        d.polygon(poly, fill=outline)

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
        d.polygon(inner_poly, fill=plank_colors[idx])

        d.line([(x0 + 2, y0 + 2), (x1 - 3, y1 + 1)], fill=top_hi[idx])
        d.line([(x0 + 3, y0 + th - 1), (x1 - 4, y1 + th - 1)], fill=bot_sh)

        for gx in range(x0 + 14, x1 - 24, 32):
            d.line([gx, y0 + 4, gx + 20, y0 + 4], fill=grain_dark[idx])
            d.line([gx + 5, y0 + 8, gx + 25, y0 + 8], fill=grain_light[idx])
            d.line([gx + 2, y0 + 12, gx + 16, y0 + 12], fill=grain_dark[idx])
            if (gx // 32) % 2 == 0:
                d.point((gx + 10, y0 + 6), fill=outline)

        d.point((x0 + 1, y0 + 6), fill=(0, 0, 0, 0))
        d.point((x1 - 2, y1 + 8), fill=(0, 0, 0, 0))
        d.point((x1 - 1, y1 + 9), fill=(0, 0, 0, 0))

        draw_nail(d, x0 + 8, y0 + th // 2, *nail_colors)
        draw_nail(d, x1 - 8, y1 + th // 2, *nail_colors)

    return img, plank_defs


# --------------------------------------------------------------------------
# 1. BEACH BOARD + BRAIDED ROPE (Light driftwood, algae, coral, tied ropes)
# --------------------------------------------------------------------------
def draw_rope_segment(d, x0, y0, x1, y1):
    # Thick rustic hemp rope line between two points with outline + highlight
    d.line([(x0, y0), (x1, y1)], fill=(78, 50, 24, 255), width=4)
    d.line([(x0, y0), (x1, y1)], fill=(196, 152, 92, 255), width=2)
    d.line([(x0 - 1, y0), (x1 - 1, y1)], fill=(230, 190, 128, 255), width=1)


def draw_coral_cluster(d, cx, cy, flip=False):
    s = -1 if flip else 1
    out = (120, 34, 38, 255)
    body = (242, 96, 86, 255)
    hi = (255, 166, 138, 255)
    branches = [
        [(cx, cy), (cx + 2 * s, cy - 5), (cx + 1 * s, cy - 9)],
        [(cx + 2 * s, cy - 3), (cx + 6 * s, cy - 7)],
        [(cx - 1 * s, cy - 3), (cx - 4 * s, cy - 6)],
    ]
    for br in branches:
        d.line(br, fill=out, width=3)
    for br in branches:
        d.line(br, fill=body, width=1)
    d.point((cx + 1 * s, cy - 9), fill=hi)
    d.point((cx + 6 * s, cy - 7), fill=hi)
    d.point((cx - 4 * s, cy - 6), fill=hi)


def draw_algae_drape(d, start_x, start_y, lengths):
    out = (18, 66, 36, 255)
    mid = (44, 152, 76, 255)
    hi = (104, 208, 120, 255)
    for i, length in enumerate(lengths):
        x = start_x + i * 2
        wiggle = 1 if i % 2 == 0 else 0
        for dy in range(length):
            px = x + (1 if (dy // 3 + i) % 2 == 0 else 0)
            py = start_y + dy
            d.point((px - 1, py), fill=out)
            d.point((px + 1, py), fill=out)
            d.point((px, py), fill=hi if dy < 3 or (dy + i) % 4 == 0 else mid)
        d.point((x + wiggle, start_y + length), fill=out)


def generate_beach_board():
    W, H = 360, 64
    img, _ = wood_planks_base(
        W,
        H,
        plank_colors=[
            (196, 156, 102, 255),
            (182, 142, 90, 255),
            (204, 164, 110, 255),
        ],
        top_hi=[
            (236, 202, 148, 255),
            (226, 190, 134, 255),
            (242, 210, 156, 255),
        ],
        bot_sh=(116, 82, 44, 255),
        outline=(74, 48, 24, 255),
        grain_dark=[
            (162, 122, 74, 255),
            (150, 110, 64, 255),
            (168, 128, 78, 255),
        ],
        grain_light=[
            (218, 182, 128, 255),
            (208, 170, 118, 255),
            (224, 188, 134, 255),
        ],
        nail_colors=((92, 68, 46, 255), (156, 126, 94, 255), (48, 32, 18, 255)),
    )
    d = ImageDraw.Draw(img)

    # Drilled corner rope holes (4 corners) + tied hemp ropes looping from hanging eyelets (x=54, 306)
    corner_holes = [(16, 10), (19, 51), (W - 17, 10), (W - 20, 50)]
    for hx, hy in corner_holes:
        d.ellipse([hx - 3, hy - 3, hx + 3, hy + 3], fill=(42, 26, 14, 255), outline=(110, 76, 42, 255))

    # Left & right rope harnesses wrapping cleanly through the 4 corner holes and meeting at hanging eyelets (bx = 54, 306)
    for bx, h_top, h_bot, side in [(54, (16, 10), (19, 51), -1), (306, (W - 17, 10), (W - 20, 50), 1)]:
        # Upper diagonal rope from top hanging knot to outer upper corner hole, then down to lower corner hole
        draw_rope_segment(d, bx, 3, h_top[0], h_top[1])
        draw_rope_segment(d, h_top[0], h_top[1], h_bot[0], h_bot[1])
        # Rope collar wrap around top rim at eyelet target (bx, 2) + tidy short knotted tail
        d.rectangle([bx - 4, 0, bx + 4, 7], fill=(182, 136, 78, 255), outline=(74, 46, 22, 255))
        d.line([(bx - 3, 2), (bx + 3, 2)], fill=(232, 194, 130, 255))
        d.line([(bx - 3, 5), (bx + 3, 5)], fill=(122, 84, 42, 255))
        # Corner knots over holes
        for hx, hy in (h_top, h_bot):
            d.rectangle([hx - 3, hy - 2, hx + 3, hy + 2], fill=(208, 164, 100, 255), outline=(74, 46, 22, 255))
            d.point((hx - 1, hy - 1), fill=(238, 202, 142, 255))

    # Hanging algae draped along outer edges & top rim (leaving central text box clean)
    draw_algae_drape(d, 26, 2, [9, 13, 8, 6])
    draw_algae_drape(d, 74, 2, [7, 11, 6])
    draw_algae_drape(d, 278, 2, [6, 10, 13, 7])
    draw_algae_drape(d, 330, 2, [8, 12, 9])
    draw_algae_drape(d, 7, 36, [11, 15, 9])
    draw_algae_drape(d, 344, 34, [10, 14, 8])

    # Small coastal coral sprigs on outer corners / edges
    draw_coral_cluster(d, 10, 28, flip=False)
    draw_coral_cluster(d, 350, 28, flip=True)
    draw_coral_cluster(d, 34, 58, flip=False)
    draw_coral_cluster(d, 324, 58, flip=True)

    return pixel_scale(img, 2)


def generate_beach_rope_link():
    # 8x16 @2x -> 16x32. Period 13px in 1x (26px in 2x) so Chain.qml overlaps seamlessly!
    W, H = 8, 16
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    for y in range(H):
        phase = y % 13
        # Braided rope 2.5px wide centered around x=3..4 with gentle diagonal strand weave
        d.line([(2, y), (5, y)], fill=(78, 48, 22, 255))
        d.line([(3, y), (4, y)], fill=(192, 146, 86, 255))
        if phase % 4 == 0:
            d.point((3, y), fill=(235, 198, 134, 255))
            d.point((4, y), fill=(138, 96, 48, 255))
        elif phase % 4 == 1:
            d.point((4, y), fill=(235, 198, 134, 255))
            d.point((3, y), fill=(166, 122, 68, 255))
        elif phase % 4 == 2:
            d.point((3, y), fill=(138, 96, 48, 255))
            d.point((4, y), fill=(218, 174, 110, 255))
        else:
            d.point((3, y), fill=(212, 168, 104, 255))
    return pixel_scale(img, 2)


# --------------------------------------------------------------------------
# 2. SWAMP BOARD + RUSTY CHAIN (Dark forged iron plate, rusty edges)
# --------------------------------------------------------------------------
def generate_swamp_board():
    W, H = 360, 64
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    # Tiered heavy iron plate silhouette matching 360x64 aspect & bounds
    outer = [(10, 3), (350, 3), (358, 11), (358, 52), (350, 60), (10, 60), (2, 52), (2, 11)]
    d.polygon(outer, fill=(18, 22, 22, 255))
    body = [(11, 5), (349, 5), (356, 12), (356, 51), (349, 58), (11, 58), (4, 51), (4, 12)]
    d.polygon(body, fill=(48, 54, 56, 255))

    # Top highlight + bottom shadow bevel
    d.line([(12, 5), (348, 5)], fill=(96, 108, 110, 255))
    d.line([(5, 12), (5, 50)], fill=(78, 88, 90, 255))
    d.line([(11, 58), (349, 58)], fill=(24, 28, 30, 255))
    d.line([(355, 12), (355, 51)], fill=(26, 30, 32, 255))

    # Recessed center armor frame
    inner = [(18, 11), (342, 11), (346, 15), (346, 48), (342, 52), (18, 52), (14, 48), (14, 15)]
    d.polygon(inner, fill=(40, 46, 48, 255), outline=(26, 30, 32, 255))
    d.line([(16, 48), (16, 15), (343, 15)], fill=(30, 35, 36, 255))
    d.line([(18, 51), (342, 51)], fill=(64, 72, 74, 255))

    # Deterministic mottled rust patches around outer rim, corners & seams
    rng = random.Random(137)
    rust_cols = [
        (104, 46, 20, 255),
        (142, 68, 28, 255),
        (172, 88, 36, 255),
        (76, 36, 18, 255),
    ]
    px = img.load()
    for y in range(4, 60):
        for x in range(3, 357):
            if px[x, y][3] == 0 or px[x, y][:3] == (18, 22, 22):
                continue
            edge_dist = min(x - 3, 356 - x, y - 4, 59 - y)
            corner_boost = 5 if (x < 34 or x > 326) else 0
            if edge_dist <= 6 + corner_boost:
                prob = max(0.0, 0.78 - edge_dist * 0.085)
                if rng.random() < prob:
                    c = rust_cols[rng.randrange(len(rust_cols))]
                    px[x, y] = c
                    if rng.random() < 0.55 and x + 1 < 356 and px[x + 1, y][3] > 0:
                        px[x + 1, y] = rust_cols[rng.randrange(len(rust_cols))]

    # Corroded iron rivets along edges + rusty hanging shackles at bx = 54, 306
    for rx, ry in [(12, 10), (12, 53), (348, 10), (348, 53), (26, 8), (334, 8), (26, 55), (334, 55)]:
        draw_rivet(d, rx, ry, hi=(184, 106, 52, 255), mid=(122, 62, 28, 255), dark=(44, 24, 14, 255))

    for bx in [54, 306]:
        d.rectangle([bx - 6, 0, bx + 6, 13], fill=(56, 42, 34, 255), outline=(18, 16, 16, 255))
        d.line([bx - 5, 1, bx + 5, 1], fill=(156, 84, 38, 255))
        d.rectangle([bx - 3, 0, bx + 3, 3], fill=(24, 22, 22, 255))
        draw_rivet(d, bx, 8, hi=(188, 108, 52, 255), mid=(126, 62, 28, 255), dark=(40, 22, 12, 255))

    return pixel_scale(img, 2)


def generate_swamp_chain_link():
    W, H = 8, 16
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    d.ellipse([0, 0, W - 1, H - 1], fill=(48, 42, 40, 255), outline=(16, 14, 14, 255))
    d.ellipse([2, 3, W - 3, H - 4], fill=(0, 0, 0, 0), outline=(16, 14, 14, 255))
    # Rust crust highlights & corroded dark spots
    d.line([1, 2, 1, H - 3], fill=(156, 82, 38, 255))
    d.line([1, 1, W - 2, 1], fill=(182, 102, 46, 255))
    d.line([W - 2, 2, W - 2, H - 3], fill=(64, 34, 20, 255))
    d.line([1, H - 2, W - 2, H - 2], fill=(82, 42, 22, 255))
    d.point((1, 6), fill=(110, 54, 26, 255))
    d.point((6, 9), fill=(142, 74, 34, 255))

    return pixel_scale(img, 2)


# --------------------------------------------------------------------------
# 3. SNOW / ARCTIC BOARD + FROSTED CHAIN (Dark timber + snow cap on top edge)
# --------------------------------------------------------------------------
def generate_snow_board():
    W, H = 360, 64
    img, plank_defs = wood_planks_base(
        W,
        H,
        plank_colors=[
            (64, 52, 48, 255),
            (54, 44, 40, 255),
            (68, 56, 52, 255),
        ],
        top_hi=[
            (98, 86, 82, 255),
            (86, 74, 70, 255),
            (102, 90, 86, 255),
        ],
        bot_sh=(28, 22, 20, 255),
        outline=(20, 16, 16, 255),
        grain_dark=[
            (46, 36, 32, 255),
            (38, 30, 26, 255),
            (50, 40, 36, 255),
        ],
        grain_light=[
            (82, 70, 66, 255),
            (72, 60, 56, 255),
            (86, 74, 70, 255),
        ],
        nail_colors=((48, 58, 72, 255), (165, 205, 232, 255), (18, 24, 34, 255)),
    )
    d = ImageDraw.Draw(img)

    # Dark steel brackets at bx = 54, 306
    for bx in [54, 306]:
        d.rectangle([bx - 6, 0, bx + 6, 13], fill=(44, 54, 68, 255), outline=(14, 18, 26, 255))
        d.line([bx - 5, 1, bx + 5, 1], fill=(155, 198, 226, 255))
        d.point((bx, 7), fill=(210, 235, 252, 255))
        d.rectangle([bx - 3, 0, bx + 3, 3], fill=(22, 28, 38, 255))

    # Organic sculpted snowdrifts along top edge (soft billowing waves, keeping y>=11 center clear)
    import math
    for x in range(8, 353):
        drift = 1.6 * math.sin(x * 0.14) + 1.0 * math.cos(x * 0.29)
        end_boost = 2.8 if (x < 42 or x > 318) else 0.0
        bracket_dip = -2.2 if (abs(x - 54) <= 7 or abs(x - 306) <= 7) else 0.0
        depth = max(3, min(9, int(round(5.2 + drift + end_boost + bracket_dip))))
        y_top = 2 if (abs(x - 54) <= 6 or abs(x - 306) <= 6) else 1
        y_bot = y_top + depth
        d.line([(x, y_top), (x, y_bot - 2)], fill=(242, 250, 255, 255))
        d.point((x, y_bot - 1), fill=(186, 226, 248, 255))
        d.point((x, y_bot), fill=(116, 166, 210, 255))
        d.point((x, y_bot + 1), fill=(42, 68, 98, 255))

    # Small snow puffs on lower outer plank shelves & 4 corner icicles
    for ix, iy in [(4, 20), (355, 21), (13, 40), (346, 40)]:
        d.line([(ix - 2, iy), (ix + 2, iy)], fill=(235, 248, 255, 255))
        d.line([(ix - 2, iy + 1), (ix + 2, iy + 1)], fill=(176, 220, 246, 255))
    for ic_x in [14, 27, 333, 346]:
        d.line([(ic_x, 9), (ic_x, 14)], fill=(190, 234, 255, 255))
        d.point((ic_x, 15), fill=(118, 174, 216, 255))

    return pixel_scale(img, 2)


def generate_snow_chain_link():
    W, H = 8, 16
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    d.ellipse([0, 0, W - 1, H - 1], fill=(42, 54, 70, 255), outline=(14, 20, 28, 255))
    d.ellipse([2, 3, W - 3, H - 4], fill=(0, 0, 0, 0), outline=(14, 20, 28, 255))
    d.line([1, 2, 1, H - 3], fill=(190, 230, 250, 255))
    d.line([1, 1, W - 2, 1], fill=(224, 246, 255, 255))
    d.line([W - 2, 2, W - 2, H - 3], fill=(32, 44, 58, 255))
    d.line([1, H - 2, W - 2, H - 2], fill=(64, 84, 106, 255))

    return pixel_scale(img, 2)


# --------------------------------------------------------------------------
# 4. MOON BOARD + MODERN TITANIUM CHAIN (Clean aerospace hull plate)
# --------------------------------------------------------------------------
def generate_moon_board():
    W, H = 360, 64
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    # Modern octagonal chamfered titanium plate matching 360x64 layout
    outer = [(10, 3), (350, 3), (358, 11), (358, 52), (350, 60), (10, 60), (2, 52), (2, 11)]
    d.polygon(outer, fill=(44, 56, 74, 255))

    hull = [(11, 5), (349, 5), (356, 12), (356, 51), (349, 58), (11, 58), (4, 51), (4, 12)]
    d.polygon(hull, fill=(214, 222, 232, 255))

    # Brushed light-titanium horizontal bands & crisp top chamfer highlight
    d.line([(12, 5), (348, 5)], fill=(248, 252, 255, 255))
    d.line([(12, 6), (348, 6)], fill=(234, 240, 248, 255))
    d.line([(5, 12), (5, 50)], fill=(240, 246, 252, 255))
    d.line([(11, 57), (349, 57)], fill=(138, 152, 170, 255))
    d.line([(11, 58), (349, 58)], fill=(102, 118, 138, 255))
    d.line([(355, 12), (355, 51)], fill=(134, 148, 166, 255))

    # Recessed clean central instrument face
    panel = [(22, 10), (338, 10), (342, 14), (342, 49), (338, 53), (22, 53), (18, 49), (18, 14)]
    d.polygon(panel, fill=(228, 234, 242, 255), outline=(144, 158, 176, 255))
    d.line([(20, 49), (20, 14), (339, 14)], fill=(186, 198, 214, 255))
    d.line([(22, 52), (338, 52)], fill=(246, 250, 255, 255))

    # Subtle cyan/cobalt aerospace accent stripes on left & right wings
    for sx in [10, 346]:
        d.rectangle([sx, 18, sx + 3, 45], fill=(64, 175, 232, 255), outline=(44, 86, 132, 255))
        d.line([(sx + 1, 19), (sx + 1, 44)], fill=(150, 228, 255, 255))

    # Machined titanium hex bolts at 4 corners
    for rx, ry in [(13, 9), (13, 54), (347, 9), (347, 54)]:
        draw_rivet(d, rx, ry, hi=(250, 252, 255, 255), mid=(146, 160, 178, 255), dark=(64, 78, 96, 255))

    # Sturdier brushed titanium clevis brackets at bx = 54, 306
    for bx in [54, 306]:
        d.rectangle([bx - 7, 0, bx + 7, 13], fill=(168, 182, 198, 255), outline=(44, 56, 74, 255))
        d.line([(bx - 6, 1), (bx + 6, 1)], fill=(242, 248, 255, 255))
        d.rectangle([bx - 4, 0, bx + 4, 4], fill=(60, 74, 94, 255))
        draw_rivet(d, bx, 8, hi=(250, 252, 255, 255), mid=(128, 142, 160, 255), dark=(52, 66, 84, 255))

    return pixel_scale(img, 2)


def generate_moon_chain_link():
    # Sturdier squared-off heavy-duty interlocking titanium link (8x16 @2x -> 16x32)
    W, H = 8, 16
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    # Outer sturdy titanium link body (rows 1..14) + vertical interlocking titanium pin ends (y=0..2, y=13..15)
    d.rectangle([0, 1, W - 1, H - 2], fill=(142, 156, 174, 255), outline=(36, 46, 62, 255))
    d.rectangle([2, 4, W - 3, H - 5], fill=(0, 0, 0, 0), outline=(36, 46, 62, 255))
    d.line([1, 2, W - 2, 2], fill=(238, 245, 252, 255))
    d.line([1, 3, 1, H - 4], fill=(220, 232, 244, 255))
    d.line([W - 2, 3, W - 2, H - 4], fill=(82, 96, 114, 255))
    d.line([1, H - 3, W - 2, H - 3], fill=(74, 88, 106, 255))
    # Heavy vertical center titanium connector pins overlapping smoothly every 13px (26px at 2x)
    d.rectangle([3, 0, 4, 2], fill=(188, 202, 220, 255))
    d.point((3, 0), fill=(242, 248, 255, 255))
    d.rectangle([3, 13, 4, 15], fill=(108, 122, 140, 255))
    d.point((3, 14), fill=(210, 224, 240, 255))

    return pixel_scale(img, 2)


def main():
    assets = {
        "wooden_board_beach.png": generate_beach_board(),
        "chain_link_beach.png": generate_beach_rope_link(),
        "wooden_board_swamp.png": generate_swamp_board(),
        "chain_link_swamp.png": generate_swamp_chain_link(),
        "wooden_board_snow.png": generate_snow_board(),
        "chain_link_snow.png": generate_snow_chain_link(),
        "wooden_board_moon.png": generate_moon_board(),
        "chain_link_moon.png": generate_moon_chain_link(),
    }
    for name, img in assets.items():
        path = os.path.join(OUTPUT_DIR, name)
        img.save(path, optimize=True)
        print(f"  [+] Wrote {name} ({img.width}x{img.height} {img.mode})")


if __name__ == "__main__":
    main()
