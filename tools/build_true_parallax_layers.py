#!/usr/bin/env python3
"""
build_true_parallax_layers.py
Builds true, discrete, 3-layer parallax backgrounds for Beach, Swamp, Snow, and Moon
matching the exact structure and visual standard of Grassland:
  1. bg_sky.png (3840x1080 seamless): Original sky/clouds/sun/moon/aurora/space preserved
     faithfully on top, with any foreground-tree occlusions cleanly reconstructed and the
     sky/horizon atmosphere naturally extended down to row 1080.
  2. bg_mountains.png (3840x1080 seamless): Middle parallax layer (distant mountains,
     volcanic cliffs & ocean, misty swamp forest & fog, glaciated peaks, lunar crater &
     jagged range) with 100% transparent alpha above the skyline and naturally extended
     middle-layer terrain down to row 1080.
  3. bg_hills.png (3840x1080 seamless): Foreground hills/dunes/regolith with 100% intact,
     unbroken foreground trees (palms, giant cypresses with hanging moss, snowy pines) and
     equipment/cattails, 100% transparent above the foreground crest and around props.
"""

import math
import os
import numpy as np
from PIL import Image
from scipy.ndimage import (
    binary_closing,
    binary_dilation,
    binary_fill_holes,
    gaussian_filter1d,
    label,
    median_filter,
)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS = os.path.join(ROOT, "assets")
DEBUG_DIR = os.path.join(ROOT, "temp", "debug", "v6")
os.makedirs(DEBUG_DIR, exist_ok=True)

H, W = 1080, 1920

ARTIFACT_DIR = r"C:\Users\USER\.gemini\antigravity\brain\c76b8a15-4f09-42c1-abbf-332776766427"
EXT_MTNS = {
    "beach": os.path.join(ARTIFACT_DIR, "beach_mountains_ext_1795777820909.jpg"),
    "swamp": os.path.join(ARTIFACT_DIR, "swamp_midground_isolated_v2_1795778557114.jpg"),
    "snow":  os.path.join(ARTIFACT_DIR, "snow_mountains_isolated_1795778033333.jpg"),
    "snow_lower": os.path.join(ARTIFACT_DIR, "snow_mountains_fixed_parallax_v2_1795783174159.jpg"),
    "moon":  os.path.join(ARTIFACT_DIR, "moon_mountains_ext_1795777816034.jpg"),
}


def make_seamless(rgba):
    """Mirror 1920px horizontally to create a 3840x1080 seamlessly wrapping strip."""
    arr_flip = np.fliplr(rgba)
    strip = np.hstack((rgba, arr_flip))
    return Image.fromarray(strip, mode="RGBA")


def clean_edges(rgba):
    """Clamp 2px border rows/cols to eliminate Lanczos edge artifacts at mirror/bottom seams."""
    out = rgba.copy()
    out[-2:, :] = out[-3:-2, :]
    out[:, -2:] = out[:, -3:-2]
    out[:, :2] = out[:, 2:3]
    return out


def save_checker(rgba, path):
    h, w, _ = rgba.shape
    yy, xx = np.mgrid[0:h, 0:w]
    chk = (((yy // 16) + (xx // 16)) % 2) == 0
    bg = np.where(
        chk[:, :, None],
        np.array([255, 0, 255], dtype=np.uint8),
        np.array([180, 0, 180], dtype=np.uint8),
    )
    a = rgba[:, :, 3:4].astype(float) / 255.0
    out = (rgba[:, :, :3] * a + bg * (1 - a)).astype(np.uint8)
    Image.fromarray(out).save(path)


def keep_small_holes_only(mask, max_hole_area=180):
    """Fill tiny interior pinholes up to max_hole_area while keeping true transparent gaps open."""
    inv = ~mask
    lbl, n = label(inv)
    if n == 0:
        return mask
    counts = np.bincount(lbl.ravel())
    border_lbls = (
        set(np.unique(lbl[0, :]))
        | set(np.unique(lbl[-1, :]))
        | set(np.unique(lbl[:, 0]))
        | set(np.unique(lbl[:, -1]))
    )
    res = mask.copy()
    for i in range(1, n + 1):
        if i not in border_lbls and counts[i] <= max_hole_area:
            res[lbl == i] = True
    return res


def dp_ridge(cost, y_min, y_max, max_step=2, step_penalty=2.0):
    """Find smooth minimum-cost path y[x] from x=0..W-1 within [y_min, y_max)."""
    sub = cost[y_min:y_max, :].astype(np.float64)
    ny, nx = sub.shape
    dp = np.zeros((ny, nx), dtype=np.float64)
    back = np.zeros((ny, nx), dtype=np.int32)
    dp[:, 0] = sub[:, 0]
    ys = np.arange(ny)
    for x in range(1, nx):
        prev = dp[:, x - 1]
        best_cost = np.full(ny, np.inf)
        best_idx = np.zeros(ny, dtype=np.int32)
        for dy in range(-max_step, max_step + 1):
            src_y = np.clip(ys - dy, 0, ny - 1)
            cand = prev[src_y] + step_penalty * abs(dy)
            better = cand < best_cost
            best_cost[better] = cand[better]
            best_idx[better] = src_y[better]
        dp[:, x] = sub[:, x] + best_cost
        back[:, x] = best_idx
    path = np.zeros(nx, dtype=int)
    path[-1] = int(np.argmin(dp[:, -1]))
    for x in range(nx - 1, 0, -1):
        path[x - 1] = back[path[x], x]
    return path + y_min


def extract_chroma_layer(img_path, key="green", clear_pockets_below=None):
    """Extract middle layer cleanly from a #00FF00 green or #FF00FF magenta chroma-key image."""
    arr = np.array(
        Image.open(img_path).resize((W, H), Image.Resampling.LANCZOS).convert("RGBA")
    )
    r, g, b = arr[:, :, 0].astype(float), arr[:, :, 1].astype(float), arr[:, :, 2].astype(float)
    if key == "green":
        spill = g - np.maximum(r, b)
        is_key = (g > 130) & (spill > 38)
    else:
        spill = np.minimum(r, b) - g
        is_key = (r > 130) & (b > 130) & (spill > 38)

    lbl, _ = label(is_key)
    top_lbls = set(np.unique(lbl[0, :])) - {0}
    sky = np.isin(lbl, list(top_lbls))
    if clear_pockets_below is not None:
        y_grid = np.arange(H)[:, None]
        sky = sky | (is_key & (y_grid < clear_pockets_below))

    sky_d1 = binary_dilation(sky, structure=np.ones((3, 3)))
    if key == "green":
        semi = sky_d1 & (~sky) & (g > np.maximum(r, b) + 12) & (g > 50)
    else:
        semi = sky_d1 & (~sky) & (np.minimum(r, b) > g + 12) & (np.minimum(r, b) > 50)
    sky = sky | semi
    arr[sky, 3] = 0

    # Decontaminate 2px edge against chroma spill (and clean any subtle interior magenta dots in swamp leaves)
    if key == "magenta":
        # Remove subtle magenta tint anywhere above y=680 where min(R, B) > G
        y_m, x_m = np.where((np.minimum(arr[:, :, 0].astype(float), arr[:, :, 2].astype(float)) > arr[:, :, 1].astype(float) + 2) & (arr[:, :, 3] > 0))
        if len(y_m) > 0:
            mr, mg, mb = arr[y_m, x_m, 0].astype(float), arr[y_m, x_m, 1].astype(float), arr[y_m, x_m, 2].astype(float)
            # For snow bright edge (mg >= 105 and mb >= mr), restore icy snow hue; otherwise clamp R,B to G
            bright_ice = (mg >= 105) & (mb >= mr - 5) & ("snow" in img_path)
            arr[y_m[bright_ice], x_m[bright_ice], 0] = np.clip(mg[bright_ice] + 2, 0, 255).astype(np.uint8)
            arr[y_m[bright_ice], x_m[bright_ice], 1] = np.clip(mg[bright_ice] + 18, 0, 255).astype(np.uint8)
            arr[y_m[bright_ice], x_m[bright_ice], 2] = np.clip(np.maximum(mb[bright_ice], mg[bright_ice] + 22), 0, 255).astype(np.uint8)
            other = ~bright_ice
            arr[y_m[other], x_m[other], 0] = np.clip(np.minimum(mr[other], mg[other]), 0, 255).astype(np.uint8)
            arr[y_m[other], x_m[other], 2] = np.clip(np.minimum(mb[other], mg[other] + 8), 0, 255).astype(np.uint8)
    border = binary_dilation(sky, structure=np.ones((5, 5))) & (~sky)
    ys, xs = np.where(border)
    if len(ys) > 0 and key == "green":
        arr[ys, xs, 1] = np.minimum(arr[ys, xs, 1], np.maximum(arr[ys, xs, 0], arr[ys, xs, 2]))
    return clean_edges(arr)


def extract_white_bg_layer(img_path):
    """Extract middle layer cleanly from a #FFFFFF white-sky image (Beach)."""
    arr = np.array(
        Image.open(img_path).resize((W, H), Image.Resampling.LANCZOS).convert("RGBA")
    )
    r, g, b = arr[:, :, 0].astype(int), arr[:, :, 1].astype(int), arr[:, :, 2].astype(int)
    sat_rgb = np.maximum(np.maximum(r, g), b) - np.minimum(np.minimum(r, g), b)
    is_white = (r >= 232) & (g >= 232) & (b >= 232) & (sat_rgb <= 22)
    lbl, _ = label(is_white)
    top_lbls = set(np.unique(lbl[0, :])) - {0}
    sky = np.isin(lbl, list(top_lbls))
    arr[sky, 3] = 0

    border = binary_dilation(sky, structure=np.ones((5, 5))) & (~sky)
    ys, xs = np.where(border)
    if len(ys) > 0:
        pr, pg, pb = r[ys, xs].astype(float), g[ys, xs].astype(float), b[ys, xs].astype(float)
        lum = (pr + pg + pb) / 3.0
        sat = np.maximum(np.maximum(pr, pg), pb) - np.minimum(np.minimum(pr, pg), pb)
        whiteness = np.clip((lum - 185.0) / 70.0, 0.0, 1.0) * np.clip(1.0 - sat / 45.0, 0.0, 1.0)
        new_a = np.clip(255.0 * (1.0 - whiteness * 0.85), 40, 255)
        arr[ys, xs, 3] = new_a.astype(np.uint8)
        af = new_a / 255.0
        blend = whiteness > 0.15
        arr[ys[blend], xs[blend], 0] = np.clip((pr[blend] - (1 - af[blend]) * 255) / np.maximum(af[blend], 0.25), 0, 255).astype(np.uint8)
        arr[ys[blend], xs[blend], 1] = np.clip((pg[blend] - (1 - af[blend]) * 255) / np.maximum(af[blend], 0.25), 0, 255).astype(np.uint8)
        arr[ys[blend], xs[blend], 2] = np.clip((pb[blend] - (1 - af[blend]) * 255) / np.maximum(af[blend], 0.25), 0, 255).astype(np.uint8)
    return clean_edges(arr)


def build_beach():
    print("Building Beach 3-layer parallax...")
    bg = np.array(Image.open(os.path.join(ASSETS, "beach", "background.png")).convert("RGBA"))
    y_grid, x_grid = np.arange(H)[:, None], np.arange(W)[None, :]
    br, bg_c, bb = bg[:, :, 0].astype(int), bg[:, :, 1].astype(int), bg[:, :, 2].astype(int)

    # Layer 2: Distant volcanic islands + coastal cliffs + turquoise ocean
    mtn = extract_white_bg_layer(EXT_MTNS["beach"])
    ext_top = np.argmax(mtn[:, :, 3] > 0, axis=0)

    # Layer 3: Golden sand dunes + dune grass + ALL 3 palm clusters (zero trunk notches, zero floating edge frond)
    is_pure_cyan_water_or_sky = ((bb >= bg_c - 8) & (bb >= 120)) | ((br <= 38) & (bb >= 175) & (bg_c >= 175))
    # Exclude the distant green coastal cliff: up to y=655 in mid-right (x<1580), and only y<576 above right palm crowns
    is_cliff = (x_grid > 1320) & (((x_grid < 1580) & (y_grid < 655)) | (y_grid < 576)) & (bb >= 72) & (br < 80) & (bg_c - br > 55)

    is_sand = (br >= 162) & (br - bb >= 16) & (bg_c - bb >= 10) & (y_grid >= 720) & (~is_pure_cyan_water_or_sky)
    is_trunk = (br >= 42) & (bb < 135) & (bb - br <= 25) & (~is_cliff) & (y_grid >= 460)
    is_palm_green = (bg_c > bb + 14) & (bg_c >= br) & (bb < 115) & (~is_cliff) & (~is_pure_cyan_water_or_sky) & (y_grid >= 460)
    is_palm_outline = (np.maximum(np.maximum(br, bg_c), bb) < 78) & (y_grid >= 460)
    is_grass_tip = (
        (y_grid >= 715)
        & (~is_pure_cyan_water_or_sky)
        & (bb <= 135)
        & (br > 88)
        & (bg_c > 88)
        & (bg_c - bb > 8)
    )

    # Anchor sand through lower trunk roots and dune grass tufts so crests behind roots/tufts stay solid
    sand_bridge = is_sand | is_grass_tip | (is_trunk & (y_grid >= 732)) | (is_palm_outline & (y_grid >= 732))
    lbl_s, _ = label(sand_bridge, structure=np.ones((3, 3)))
    bot_s = set(np.unique(lbl_s[H - 5:, :])) - {0}
    sand_mask = binary_fill_holes(np.isin(lbl_s, list(bot_s)))

    palm_core = is_trunk | is_palm_green | is_grass_tip
    near_palm = binary_dilation(palm_core | sand_mask, structure=np.ones((5, 5)))
    fg_raw = sand_mask | ((palm_core | (is_palm_outline & near_palm)) & (~is_pure_cyan_water_or_sky) & (~is_cliff))
    # Remove disconnected off-screen palm tip touching left border (prevents floating green diamond at wrap seam)
    fg_raw[(x_grid < 45) & (y_grid >= 538) & (y_grid <= 628)] = False

    fg_filled = keep_small_holes_only(fg_raw, max_hole_area=260)
    lbl_b, _ = label(fg_filled, structure=np.ones((3, 3)))
    anchor_b = set(np.unique(lbl_b[binary_dilation(sand_mask, structure=np.ones((7, 7)))])) - {0}
    beach_mask = np.isin(lbl_b, list(anchor_b))
    hills = np.zeros_like(bg)
    hills[beach_mask] = bg[beach_mask]
    hills = clean_edges(hills)

    # Layer 1: Sky + sun + clouds + seagulls (zero mountain leaks, smooth horizon extension)
    is_beach_sky = (bb >= 236) & (y_grid < 610) & ~((x_grid > 1350) & (bg_c > br + 70) & (bb < 235)) & (~beach_mask)
    lbl_bs, _ = label(is_beach_sky)
    top_bs = set(np.unique(lbl_bs[0, :])) - {0}
    sky_mask = np.isin(lbl_bs, list(top_bs))
    bg_sky_bot = np.zeros(W, dtype=int)
    for x in range(W):
        ys = np.where(sky_mask[:, x])[0]
        bg_sky_bot[x] = ys.max() if len(ys) > 0 else 460
    # Ensure left edge behind palm (x < 260) cuts safely above palm tips (y=452) and blends smoothly right of x=240
    clean_l = int(np.median(bg_sky_bot[270:330]))
    bg_sky_bot[:245] = 452
    bg_sky_bot[245:270] = np.linspace(452, clean_l, 25).astype(int)
    cut_sky = np.minimum(bg_sky_bot, ext_top) - 4

    sky = np.zeros_like(bg)
    clean_sky_col = np.median(bg[450:500, 800:950, :3], axis=(0, 1))
    top_samples = np.array([bg[max(0, cut_sky[x] - 6), x, :3] for x in range(W)], dtype=float)
    top_samples[:270] = top_samples[270:360].mean(axis=0)
    top_samples = gaussian_filter1d(top_samples, sigma=140, axis=0)
    for x in range(W):
        yb = max(380, cut_sky[x])
        sky[:yb, x] = bg[:yb, x]
        c_edge = bg[yb - 1, x, :3].astype(float)
        c_mid = 0.65 * top_samples[x] + 0.35 * clean_sky_col
        c_bot = np.array([175.0, 232.0, 250.0])
        for y in range(yb, H):
            t = (y - yb) / float(H - yb + 1)
            blend_h = min(1.0, (y - yb) / 30.0)
            c0 = c_edge * (1.0 - blend_h) + c_mid * blend_h
            col = (c0 * (1.0 - t * 0.6) + c_bot * (t * 0.6)).astype(np.uint8)
            sky[y, x] = [col[0], col[1], col[2], 255]
    sky = clean_edges(sky)
    return sky, mtn, hills


def build_swamp():
    print("Building Swamp 3-layer parallax...")
    bg = np.array(Image.open(os.path.join(ASSETS, "swamp", "background.png")).convert("RGBA"))
    y_grid, x_grid = np.arange(H)[:, None], np.arange(W)[None, :]
    wr, wg, wb = bg[:, :, 0].astype(int), bg[:, :, 1].astype(int), bg[:, :, 2].astype(int)

    # Layer 2: Distant misty cypress forest + fog + muted dark-teal swamp mounds
    mtn = extract_chroma_layer(EXT_MTNS["swamp"], key="magenta", clear_pockets_below=660)

    # Layer 3: Closer bright lime-green mossy hills + cattails/reeds + left & right giant cypress trees
    in_trees = (
        ((x_grid < 325) & (y_grid < 685))
        | ((x_grid < 175) & (y_grid < 775))
        | ((x_grid > 1475) & (y_grid < 695))
        | ((x_grid > 1655) & (y_grid < 785))
    )

    is_close_moss_lit = (wg - wb >= 34) & (wg >= wr + 10) & (wr >= 38) & (wb <= 50)
    is_dark_outline = np.maximum(np.maximum(wr, wg), wb) <= 28

    # Solid foreground hill crest DP: require solid moss/shadow below AND forbid mid-distance dark teal hill
    cost_hill = np.full((H, W), 30.0)
    for y in range(745, 1020):
        above_not_lit = ~is_close_moss_lit[y - 4, :]
        at_moss = is_close_moss_lit[y, :] | is_dark_outline[y, :]
        below_solid = (wb[y + 4:y + 24, :] <= 52).mean(axis=0) >= 0.88
        has_warm_moss = (wg[y:y + 18, :] - wb[y:y + 18, :] >= 28).mean(axis=0) >= 0.22
        c = np.where(above_not_lit & at_moss & below_solid & has_warm_moss, -16.0, 12.0)
        in_mid_teal = (x_grid[0] >= 680) & (x_grid[0] <= 1525) & (y < 855)
        in_mid_deep = (x_grid[0] >= 680) & (x_grid[0] <= 1340) & (y < 905)
        c = np.where(in_mid_teal | in_mid_deep, 45.0, c)
        cost_hill[y, :] = c
    swamp_crest = dp_ridge(cost_hill, 745, 1020, max_step=2, step_penalty=1.5)

    fg_hill_top = swamp_crest.copy()
    for x in range(W):
        if x < 430 or x > 1535:
            for y in range(745, 925):
                if (
                    is_close_moss_lit[y, x]
                    and (wb[y:y + 24, x] <= 48).mean() >= 0.92
                    and (wg[y:y + 18, x] - wb[y:y + 18, x] >= 30).mean() >= 0.35
                ):
                    fg_hill_top[x] = min(fg_hill_top[x], y)
                    break
    fg_hill_top[:185] = np.minimum(fg_hill_top[:185], 765)
    fg_hill_top[1665:] = np.minimum(fg_hill_top[1665:], 755)
    fg_hill_top = median_filter(fg_hill_top, size=15)

    # Above fg_hill_top: crisp dark stems/outlines (excluding mid-distance dark teal bushes) + intact cattail heads
    is_mid_dark_teal = (wr <= 15) & (wb >= 16) & (wg - wr >= 12)
    is_dark_stem = (np.maximum(np.maximum(wr, wg), wb) <= 44) & (~is_mid_dark_teal)
    near_dark = binary_dilation(is_dark_stem, structure=np.ones((7, 7)))
    near_cattail = binary_dilation(is_dark_stem, structure=np.ones((23, 23)))
    is_cattail_head = (wr - wb >= 22) & (wr >= 40) & (wb <= 84) & (wr >= wg - 16) & near_cattail
    is_fg_blade = (wg - wb >= 24) & (wg >= wr + 4) & (wb <= 56) & near_dark
    is_fg_moss = (wg - wb >= 34) & (wg >= wr + 10) & (wb <= 46)
    is_fg_cattail = (y_grid >= 675) & (is_dark_stem | is_cattail_head | is_fg_blade | is_fg_moss)

    is_tree_bark = in_trees & (
        (np.maximum(np.maximum(wr, wg), wb) <= 32)
        | ((wb <= 36) & (wg <= 75) & (wr <= 68) & (wg >= wr - 4))
    )
    is_spanish_moss = in_trees & (wg - wb >= 30) & (wg - wr >= 14) & (wg >= 74) & (wb <= 76)
    tree_seed = is_tree_bark | is_spanish_moss

    raw = is_fg_cattail | tree_seed
    for x in range(W):
        raw[fg_hill_top[x]:, x] = True

    filled = keep_small_holes_only(raw, max_hole_area=240)
    for x in range(W):
        filled[fg_hill_top[x]:, x] = True

    lbl_w, _ = label(filled, structure=np.ones((3, 3)))
    anchors_w = (
        set(np.unique(lbl_w[H - 5:, :]))
        | set(np.unique(lbl_w[:750, :15]))
        | set(np.unique(lbl_w[:750, -15:]))
    ) - {0}
    swamp_mask = np.isin(lbl_w, list(anchors_w))
    hills = np.zeros_like(bg)
    hills[swamp_mask] = bg[swamp_mask]
    hills = clean_edges(hills)

    # Layer 1: Twilight sunset sky + moon + clouds + fireflies
    # Keep original sky outside left/right trees, fill left (x < 335) & right (x > 1615) completely
    # from clean Bayer-dithered open sky (x=600..740), and cleanly strip canopy silhouettes in y=340..465.
    pure_strip = bg[:465, 600:740, :3]
    row_ref_tile = np.zeros((465, 140, 3), dtype=np.uint8)
    for y in range(465):
        med = np.median(pure_strip[y], axis=0)
        diff = np.abs(pure_strip[y].astype(float) - med).sum(axis=1)
        clean_cols = pure_strip[y][diff < 30]
        if len(clean_cols) >= 8:
            reps = (140 // len(clean_cols)) + 1
            row_ref_tile[y] = np.tile(clean_cols, (reps, 1))[:140]
        else:
            row_ref_tile[y] = med.astype(np.uint8)

    sky = bg.copy()
    # Identify distant canopy pixels in y=335..465 and dilate mask so 0px canopy edge remains
    canopy_mask = np.zeros((465, W), dtype=bool)
    canopy_mask[338:465, :] = (wg[:465, :] >= wr[:465, :] - 4)[338:465, :] & (wb[:465, :] < 110)[338:465, :]
    # Don't erase the glowing green moon or fireflies above y=365!
    canopy_mask[:362, 1050:1420] = False
    canopy_mask = binary_dilation(canopy_mask, structure=np.ones((9, 9)))

    tree_dil = binary_dilation(swamp_mask & in_trees, structure=np.ones((13, 13)))
    for y in range(465):
        for x in range(W):
            if x < 335 or x >= 1615 or canopy_mask[y, x] or tree_dil[y, x]:
                ref_px = row_ref_tile[y, x % 140].astype(float)
                if (not tree_dil[y, x]) and (not canopy_mask[y, x]) and (1470 <= x < 1615) and (170 <= y <= 295):
                    continue
                if (tree_dil[y, x] or x >= 1615) and (1472 <= x < 1615) and (175 <= y <= 290):
                    cloud_ref = bg[y, 1455:1472, :3].mean(axis=0).astype(float)
                    fade = max(0.0, 1.0 - (x - 1472) / 140.0)
                    sky[y, x, :3] = (cloud_ref * fade + ref_px * (1.0 - fade)).astype(np.uint8)
                else:
                    sky[y, x, :3] = ref_px.astype(np.uint8)
        sky[y, :, 3] = 255

    row_465 = np.median(row_ref_tile[460:465], axis=(0, 1)).astype(float)
    c_bot = np.array([104.0, 114.0, 66.0])
    for y in range(410, 465):
        t = (y - 410) / 55.0
        sky[y, :, :3] = (sky[y, :, :3] * (1.0 - 0.55 * t) + row_465 * (0.55 * t)).astype(np.uint8)
    for y in range(465, H):
        t = (y - 465) / float(H - 465)
        col = (row_465 * (1.0 - t) + c_bot * t).astype(np.uint8)
        sky[y, :, :3] = col
        sky[y, :, 3] = 255
    sky = clean_edges(sky)
    return sky, mtn, hills


def build_snow():
    print("Building Snow 3-layer parallax...")
    bg = np.array(Image.open(os.path.join(ASSETS, "snow", "background.png")).convert("RGBA"))
    y_grid = np.arange(H)[:, None]
    sr, sg, sb = bg[:, :, 0].astype(int), bg[:, :, 1].astype(int), bg[:, :, 2].astype(int)

    # Layer 2: Distant jagged glaciated mountains extended into deep dark-slate glacial valleys (zero round snowhills)
    mtn = extract_chroma_layer(EXT_MTNS["snow"], key="magenta")
    ext_top = np.argmax(mtn[:, :, 3] > 0, axis=0)

    # Fix the mirror-seam blue up-arrow inside the rightmost snow peak (x=1860..1920, y=495..645)
    # Paint clean smooth 4x4 pixel-art glacial snow facets so mirroring creates a crisp solid snow crest
    ref = mtn.copy()
    for y in range(495, 645):
        by = (y // 4) * 4
        for x in range(1860, W):
            if ref[y, x, 3] == 0:
                continue
            r, g, b, _ = ref[y, x]
            if r < 182 and int(b) - int(r) > 22:
                bx = (x // 4) * 4
                shelf_y = 594 + int((bx - 1860) * 0.25)
                if by < 532 + (1920 - bx) // 5:
                    col = np.array([224, 247, 253], dtype=np.uint8)
                elif by < shelf_y - 16:
                    col = np.array([200, 241, 252], dtype=np.uint8)
                elif by < shelf_y:
                    col = np.array([162, 219, 243], dtype=np.uint8)
                else:
                    rib = ((bx - by) // 8) % 3 == 0
                    col = np.array([124, 195, 230], dtype=np.uint8) if rib else np.array([108, 182, 220], dtype=np.uint8)
                mtn[y, x, :3] = col

    y_start = 632
    v2_full = Image.open(EXT_MTNS["snow_lower"]).convert("RGBA")
    v2_lower = np.array(
        v2_full.crop((0, 515, v2_full.width, v2_full.height)).resize((W, H - y_start), Image.Resampling.LANCZOS)
    )
    bright = np.clip((mtn[:, :, 0].astype(float) - 165.0) / 70.0, 0.0, 1.0)
    for x in range(W):
        wave = int(14 * math.sin(x * 0.014) + 7 * math.cos(x * 0.039))
        for y in range(y_start - 18, H):
            if mtn[y, x, 3] == 0:
                continue
            vy = min(v2_lower.shape[0] - 1, max(0, y - y_start))
            yb_local = y_start + wave + int(18 * bright[y, x])
            if y < yb_local:
                continue
            if y < yb_local + 24:
                t = (y - yb_local) / 24.0
                ts = t * t * (3.0 - 2.0 * t)
                mtn[y, x, :3] = (
                    mtn[y, x, :3].astype(float) * (1.0 - ts) + v2_lower[vy, x, :3].astype(float) * ts
                ).astype(np.uint8)
                mtn[y, x, 3] = 255
            else:
                mtn[y, x] = v2_lower[vy, x]
    mtn = clean_edges(mtn)

    # Layer 1: Polar night sky + aurora + crescent moon + stars + stratus clouds
    is_snow_sky = (sr < 82) & (sb > sr + 30) & (y_grid < 630)
    lbl_ss, _ = label(is_snow_sky)
    top_ss = set(np.unique(lbl_ss[0, :])) - {0}
    sky_mask = np.isin(lbl_ss, list(top_ss))
    bg_sky_bot = np.zeros(W, dtype=int)
    for x in range(W):
        ys = np.where(sky_mask[:, x])[0]
        bg_sky_bot[x] = ys.max() if len(ys) > 0 else 500
    cut_sky = np.minimum(bg_sky_bot, ext_top) - 9

    sky = np.zeros_like(bg)
    # Sample uniform horizon teal-blue from open sky valley at y=490..540 to avoid dark smudges under tall peak!
    clean_arctic_teal = np.median(bg[500:545, 760:920, :3], axis=(0, 1))
    top_samples = np.array([bg[max(0, cut_sky[x] - 4), x, :3] for x in range(W)], dtype=float)
    top_samples = gaussian_filter1d(top_samples, sigma=160, axis=0)
    for x in range(W):
        yb = max(340, cut_sky[x])
        sky[:yb, x] = bg[:yb, x]
        c_edge = bg[yb - 1, x, :3].astype(float)
        c_mid = 0.55 * top_samples[x] + 0.45 * clean_arctic_teal
        c_bot = np.array([25.0, 75.0, 125.0])
        for y in range(yb, H):
            t = (y - yb) / float(H - yb + 1)
            # Quick 25px blend from column edge into smooth arctic teal gradient
            blend_h = min(1.0, (y - yb) / 28.0)
            c0 = c_edge * (1.0 - blend_h) + c_mid * blend_h
            col = (c0 * (1.0 - t * 0.7) + c_bot * (t * 0.7)).astype(np.uint8)
            sky[y, x] = [col[0], col[1], col[2], 255]
    sky = clean_edges(sky)

    # Layer 3: Foreground white snow hills + ALL snow-covered evergreen pine trees
    is_needle = (sg > sr + 4) & (sb < 135) & (sr < 105) & (y_grid >= 590)
    is_trunk = (sr > 35) & (sg > 20) & (sb < 65) & (sr > sb + 8) & (y_grid >= 620)
    pine_seed = is_needle | is_trunk

    cost_snow = np.full((H, W), 30.0)
    for y in range(650, 825):
        above_blue = (sr[y - 4, :] < 155) & (sb[y - 4, :] - sr[y - 4, :] > 36)
        at_snow = (sr[y, :] >= 165) & (sb[y, :] >= 210)
        below_snow = sr[y + 4, :] >= 155
        in_p = pine_seed[y, :] | pine_seed[y - 3, :]
        c = np.where(above_blue & at_snow & below_snow, -15.0 - (sr[y, :] - sr[y - 4, :]) / 20.0, 15.0)
        c = np.where(in_p, 2.0, c)
        cost_snow[y, :] = c
    snow_crest = dp_ridge(cost_snow, 650, 825, max_step=2, step_penalty=1.8)

    near_pine = binary_dilation(pine_seed, structure=np.ones((11, 9)))
    snow_on_pine = (sr >= 155) & (sb >= 185) & near_pine & (y_grid >= 595)
    pine_cand = binary_closing(pine_seed | snow_on_pine, structure=np.ones((5, 5)))
    lbl_p, n_p = label(pine_cand)
    counts_p = np.bincount(lbl_p.ravel())
    pine_final = np.zeros((H, W), dtype=bool)
    for i in range(1, n_p + 1):
        if counts_p[i] >= 120 and is_needle[lbl_p == i].sum() >= 60:
            pine_final |= lbl_p == i

    hills = np.zeros_like(bg)
    for x in range(W):
        hills[snow_crest[x]:, x] = bg[snow_crest[x]:, x]
    hills[pine_final] = bg[pine_final]
    hills = clean_edges(hills)
    return sky, mtn, hills


def build_moon():
    print("Building Moon 3-layer parallax...")
    bg = np.array(Image.open(os.path.join(ASSETS, "moon", "background.png")).convert("RGBA"))
    mr, mg, mb = bg[:, :, 0].astype(int), bg[:, :, 1].astype(int), bg[:, :, 2].astype(int)
    lum_m = bg[:, :, :3].mean(axis=2)

    # Layer 2: Distant lunar crater rim + jagged high-contrast mountain range
    mtn = extract_chroma_layer(EXT_MTNS["moon"], key="green")
    ext_top = np.argmax(mtn[:, :, 3] > 0, axis=0)

    # Layer 1: Starry space + planet Earth + magenta/cyan nebulae
    # Preserve ALL stars and nebulae! Only cut below the actual lunar mountain skyline (never on nebulae or Earth!)
    is_nebula = (mr - mg > 18) | (mb - mr > 32)
    is_mtn_pixel = (lum_m >= 18) & (~is_nebula)
    bg_mtn_top = ext_top.copy()
    for x in range(W):
        lo = max(350, int(ext_top[x]) - 15)
        hi = min(620, int(ext_top[x]) + 25)
        for y in range(lo, hi):
            if is_mtn_pixel[y:y + 6, x].all():
                bg_mtn_top[x] = y
                break
    cut_sky = np.minimum(bg_mtn_top, ext_top) - 4

    sky = np.zeros_like(bg)
    np.random.seed(42)
    for x in range(W):
        yb = max(350, cut_sky[x])
        sky[:yb, x] = bg[:yb, x]
        sky[yb:, x] = [2, 3, 6, 255]
    for _ in range(220):
        sx, sy = np.random.randint(0, W), np.random.randint(390, H - 10)
        if sy > cut_sky[sx]:
            c = np.random.randint(70, 175)
            sky[sy, sx] = [c, c, min(255, c + 15), 255]
    sky = clean_edges(sky)

    # Layer 3: Foreground lighter-grey regolith hills + craters + rover + landers + dishes + solar panels
    cost_moon = np.full((H, W), 50.0)
    for y in range(655, 815):
        above_dark = np.minimum(lum_m[y - 3, :], lum_m[y - 6, :]) < 36
        at_bright = lum_m[y, :] >= 82
        below_solid = lum_m[y + 4, :] >= 65
        grad = np.clip(lum_m[y, :] - lum_m[y - 3, :], 0, 255) / 255.0
        c = 20.0 - 15.0 * grad
        c = np.where(above_dark & at_bright & below_solid, -10.0 - 10.0 * grad, c)
        c = np.where(lum_m[y, :] < 40, 35.0, c)
        cost_moon[y, :] = c
    moon_crest = dp_ridge(cost_moon, 655, 815, max_step=2, step_penalty=1.8)

    hills = np.zeros_like(bg)
    for x in range(W):
        hills[moon_crest[x]:, x] = bg[moon_crest[x]:, x]
    hills = clean_edges(hills)
    return sky, mtn, hills


def deploy_and_verify():
    builders = {
        "beach": build_beach,
        "swamp": build_swamp,
        "snow":  build_snow,
        "moon":  build_moon,
    }
    for theme, fn in builders.items():
        sky, mtn, hills = fn()
        out_dir = os.path.join(ASSETS, theme)
        os.makedirs(out_dir, exist_ok=True)

        make_seamless(sky).save(os.path.join(out_dir, "bg_sky.png"), optimize=True)
        make_seamless(mtn).save(os.path.join(out_dir, "bg_mountains.png"), optimize=True)
        make_seamless(hills).save(os.path.join(out_dir, "bg_hills.png"), optimize=True)

        Image.fromarray(sky).save(os.path.join(DEBUG_DIR, f"{theme}_sky.png"))
        save_checker(mtn, os.path.join(DEBUG_DIR, f"{theme}_mtn_chk.png"))
        save_checker(hills, os.path.join(DEBUG_DIR, f"{theme}_hills_chk.png"))

        s_s = np.hstack((sky, np.fliplr(sky)))
        m_s = np.hstack((mtn, np.fliplr(mtn)))
        h_s = np.hstack((hills, np.fliplr(hills)))
        sw = W * 2
        cam_x = 80.0 * 28.0
        sh1, sh2, sh3 = int((cam_x * 0.012) % sw), int((cam_x * 0.038) % sw), int((cam_x * 0.095) % sw)
        c80 = Image.fromarray(np.hstack((s_s, s_s))[:, sh1:sh1 + W], "RGBA")
        c80 = Image.alpha_composite(c80, Image.fromarray(np.hstack((m_s, m_s))[:, sh2:sh2 + W], "RGBA"))
        c80 = Image.alpha_composite(c80, Image.fromarray(np.hstack((h_s, h_s))[:, sh3:sh3 + W], "RGBA"))
        c80.save(os.path.join(DEBUG_DIR, f"{theme}_sim_80m.png"))

        print(f"  [+] Deployed {theme}/bg_sky.png, bg_mountains.png, bg_hills.png (3840x1080)")


if __name__ == "__main__":
    deploy_and_verify()
    print("\nAll 4 map parallax backgrounds built and deployed successfully!")
