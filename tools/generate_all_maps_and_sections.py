#!/usr/bin/env python3
"""
Generate 10 Section Images + 10km Overview Infographic Map for all 4 themes:
  - Snow (Frostbite Summit)
  - Beach (Sunken Dunes & Coral Coast)
  - Moon (Mare Tranquillitatis Craterlands)
  - Swamp (Bilewater Bog & Bayou)

Each section image is 1920x1080 with:
  - Parallax themed background
  - Themed ground strata & cross-section
  - Environment-specific gimmick/hazard (fragile ice, breakable bridges, quicksand, mud bogs, moon craters)
  - In-scene themed vehicle
  - Authentic retro HUD & telemetry card

Each overview map is 3840x1080 with:
  - Full 10,000m continuous elevation profile
  - 10 distinct marked sectors with hazard callouts
  - Environmental specs & survival mechanics
"""

import os
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS_DIR = os.path.join(ROOT, "assets")
TEMP_DIR = os.path.join(ROOT, "temp")

FONT_TITLE = ImageFont.truetype(os.path.join(ASSETS_DIR, "Silkscreen-Bold.ttf"), 28)
FONT_HEADING = ImageFont.truetype(os.path.join(ASSETS_DIR, "Silkscreen-Bold.ttf"), 20)
FONT_TEXT = ImageFont.truetype(os.path.join(ASSETS_DIR, "Silkscreen-Regular.ttf"), 15)
FONT_SMALL = ImageFont.truetype(os.path.join(ASSETS_DIR, "PressStart2P-Regular.ttf"), 11)
FONT_TINY = ImageFont.truetype(os.path.join(ASSETS_DIR, "PressStart2P-Regular.ttf"), 9)

# ==============================================================================
# SECTION DATA DEFINITIONS
# ==============================================================================
MAP_DATA = {
    "snow": {
        "title": "FROSTBITE SUMMIT",
        "subtitle": "10 KM GLACIAL GAUNTLET",
        "gravity": "9.81 m/s² (EARTH NORMAL)",
        "friction": "0.12 - 0.25 (EXTREME SLIP)",
        "air_drag": "NORMAL (1.0x)",
        "hazards": "Fragile Ice, Breaking Timber Bridges, Icy Drift, Crevasses",
        "primary_color": (160, 220, 255),
        "accent_color": (40, 180, 240),
        "ground_colors": {
            "top": (245, 250, 255),
            "sub": (180, 220, 245),
            "deep": (65, 85, 115),
            "outline": (30, 45, 70),
        },
        "sections": [
            {
                "id": 1, "name": "GLACIER FOOTHILLS", "range": "0 - 1,000m", "diff": 1,
                "gimmick": "LOW-FRICTION RUNWAY: Introduces icy traction loss. Keep steering balanced.",
                "type": "gentle_slopes", "gimmick_type": "ice_patch"
            },
            {
                "id": 2, "name": "SLIPPERY S-CURVES", "range": "1,000 - 2,000m", "diff": 2,
                "gimmick": "DRIFT HAZARD: Steep switchbacks cause rear wheel slideouts on sharp crests.",
                "type": "washboard", "gimmick_type": "slip_warning"
            },
            {
                "id": 3, "name": "FRAGILE ICE CREVASSE", "range": "2,000 - 3,000m", "diff": 3,
                "gimmick": "FRAGILE ICE BRIDGE: 18m ice shelf collapses under speed >12m/s or load >1.5s!",
                "type": "chasm", "gimmick_type": "fragile_ice"
            },
            {
                "id": 4, "name": "AVALANCHE SLOPES", "range": "3,000 - 4,000m", "diff": 3,
                "gimmick": "SHEER DOWNHILL SLIP: High drop where brakes fail; slide too far and crash!",
                "type": "steep_drop", "gimmick_type": "avalanche_drop"
            },
            {
                "id": 5, "name": "BREAKABLE TIMBER TRESTLE", "range": "4,000 - 5,000m", "diff": 4,
                "gimmick": "CRASHABLE BRIDGE: High drop from cliff onto wooden bridge shatters structure!",
                "type": "chasm", "gimmick_type": "timber_bridge"
            },
            {
                "id": 6, "name": "BLIZZARD RIDGE", "range": "5,000 - 6,000m", "diff": 3,
                "gimmick": "TERRACE ASCENT: Stepped icy shelves require momentum control without wheel spin.",
                "type": "terraces", "gimmick_type": "blizzard_drift"
            },
            {
                "id": 7, "name": "FROZEN WATERFALL ASCENT", "range": "6,000 - 7,000m", "diff": 4,
                "gimmick": "VERTICAL ICE WALL: Extreme incline where traction is near zero. Boost required.",
                "type": "steep_climb", "gimmick_type": "frozen_waterfall"
            },
            {
                "id": 8, "name": "PERMAFROST CRASH GAP", "range": "7,000 - 8,000m", "diff": 4,
                "gimmick": "HAZARD DROP: Fall off top place = crash through lower ice shelf into the pit.",
                "type": "cliff_drop", "gimmick_type": "crash_ice"
            },
            {
                "id": 9, "name": "GLACIAL CHASM GAUNTLET", "range": "8,000 - 9,000m", "diff": 5,
                "gimmick": "TRIPLE ICE SPAN: 3 consecutive breakable ice bridges across bottomless abyss.",
                "type": "multi_chasm", "gimmick_type": "multi_ice"
            },
            {
                "id": 10, "name": "PINNACLE SUMMIT", "range": "9,000 - 10,000m", "diff": 5,
                "gimmick": "SUMMIT JUMP: Final frozen precipice leap across summit chasm to finish line banner.",
                "type": "summit_leap", "gimmick_type": "finish_leap"
            }
        ]
    },
    "beach": {
        "title": "SUNKEN DUNES & CORAL COAST",
        "subtitle": "10 KM COASTAL EXPEDITION",
        "gravity": "9.81 m/s² (EARTH NORMAL)",
        "friction": "0.45 - 0.70 (SAND DRAG)",
        "air_drag": "NORMAL (1.0x)",
        "hazards": "Loose Sinking Sand, Quicksand, Tidal Water Surge, Pier Planks",
        "primary_color": (255, 215, 120),
        "accent_color": (250, 140, 30),
        "ground_colors": {
            "top": (245, 215, 150),
            "sub": (215, 175, 105),
            "deep": (150, 115, 65),
            "outline": (90, 65, 30),
        },
        "sections": [
            {
                "id": 1, "name": "SHORELINE FLATS", "range": "0 - 1,000m", "diff": 1,
                "gimmick": "WET PACKED SAND: Firm coastal flats; introduces mild wave spray drag.",
                "type": "gentle_slopes", "gimmick_type": "beach_flats"
            },
            {
                "id": 2, "name": "ROLLING DUNE WAVES", "range": "1,000 - 2,000m", "diff": 2,
                "gimmick": "POWDERY SAND DRAG: Soft dunes cause wheel spin; maintain throttle rhythm.",
                "type": "washboard", "gimmick_type": "dune_waves"
            },
            {
                "id": 3, "name": "RICKETY BOARDWALK PIER", "range": "2,000 - 3,000m", "diff": 3,
                "gimmick": "LOOSE PIER PLANKS: Wooden boardwalk across surf with missing boards & loose stilts.",
                "type": "chasm", "gimmick_type": "wooden_pier"
            },
            {
                "id": 4, "name": "QUICKSAND GULCH", "range": "3,000 - 4,000m", "diff": 3,
                "gimmick": "QUICKSAND PIT: Sinking silt hollow. Stopping sinks wheels completely!",
                "type": "sand_pit", "gimmick_type": "quicksand"
            },
            {
                "id": 5, "name": "CORAL REEF GAPS", "range": "4,000 - 5,000m", "diff": 4,
                "gimmick": "JAGGED CORAL LEDGES: Sharp limestone shelf jumps across tidal ocean channels.",
                "type": "chasm", "gimmick_type": "coral_gaps"
            },
            {
                "id": 6, "name": "TIDAL SPLASH BASIN", "range": "5,000 - 6,000m", "diff": 3,
                "gimmick": "WATER SURGE: Half-wheel deep ocean water creates heavy hydrodynamic drag.",
                "type": "terraces", "gimmick_type": "water_splash"
            },
            {
                "id": 7, "name": "DUNE DRIFT RIDGE", "range": "6,000 - 7,000m", "diff": 4,
                "gimmick": "SLIP-FACE PEAKS: Knife-edge sand ridges; over-speed flips vehicle backward.",
                "type": "steep_climb", "gimmick_type": "dune_ridge"
            },
            {
                "id": 8, "name": "SEA CAVE ARCH CROSSING", "range": "7,000 - 8,000m", "diff": 4,
                "gimmick": "COLLAPSING ARCH: Natural limestone bridge with fragile hollow roof over waves.",
                "type": "cliff_drop", "gimmick_type": "sea_arch"
            },
            {
                "id": 9, "name": "DUNE CHASM GAUNTLET", "range": "8,000 - 9,000m", "diff": 5,
                "gimmick": "COASTAL INLET JUMPS: High-speed dune launch ramps across deep ocean inlets.",
                "type": "multi_chasm", "gimmick_type": "dune_jumps"
            },
            {
                "id": 10, "name": "LIGHTHOUSE HEADLAND", "range": "9,000 - 10,000m", "diff": 5,
                "gimmick": "CLIFFTOP FINISH: Ascend towering coastal bluff to the historic lighthouse finish.",
                "type": "summit_leap", "gimmick_type": "lighthouse_finish"
            }
        ]
    },
    "moon": {
        "title": "MARE TRANQUILLITATIS CRATERLANDS",
        "subtitle": "10 KM LUNAR VACUUM TRAIL",
        "gravity": "1.62 m/s² (1/6th LOW GRAVITY)",
        "friction": "0.85 (POWDERY REGOLITH)",
        "air_drag": "0.00x (TOTAL VACUUM)",
        "hazards": "Extreme Float Time, Pitch Inversion, Meteor Craters, Floating Basalt",
        "primary_color": (210, 225, 255),
        "accent_color": (110, 190, 255),
        "ground_colors": {
            "top": (215, 220, 230),
            "sub": (145, 150, 165),
            "deep": (65, 70, 85),
            "outline": (25, 28, 35),
        },
        "sections": [
            {
                "id": 1, "name": "APOLLO LANDING PLAIN", "range": "0 - 1,000m", "diff": 1,
                "gimmick": "LOW-G INTRO: 1/6th gravity causes massive bounces. Gentle throttle recommended.",
                "type": "gentle_slopes", "gimmick_type": "moon_plain"
            },
            {
                "id": 2, "name": "LOW-G CRATER BOUNCES", "range": "1,000 - 2,000m", "diff": 2,
                "gimmick": "HANGTIME ZONE: Bowl craters catapult car 30m into the sky. Precise pitch required.",
                "type": "washboard", "gimmick_type": "crater_bounce"
            },
            {
                "id": 3, "name": "FLOATING BASALT ARCH", "range": "2,000 - 3,000m", "diff": 3,
                "gimmick": "MAGNETIC ROCK BRIDGE: Low-g floating basalt slabs spanning a deep crater rift.",
                "type": "chasm", "gimmick_type": "floating_basalt"
            },
            {
                "id": 4, "name": "SOLAR COLLECTOR FIELDS", "range": "3,000 - 4,000m", "diff": 3,
                "gimmick": "NARROW CAUSEWAY: High-tech elevated ramps between lunar solar collector arrays.",
                "type": "steep_drop", "gimmick_type": "solar_field"
            },
            {
                "id": 5, "name": "DEEP IMPACT RIFT", "range": "4,000 - 5,000m", "diff": 4,
                "gimmick": "COLOSSAL CHASM: 35m rift; requires high launch velocity to glide across.",
                "type": "chasm", "gimmick_type": "impact_rift"
            },
            {
                "id": 6, "name": "TYCHO RAY RIDGELINE", "range": "5,000 - 6,000m", "diff": 3,
                "gimmick": "VACUUM WASHBOARD: Zero air resistance allows unlimited airborne velocity.",
                "type": "terraces", "gimmick_type": "tycho_ridge"
            },
            {
                "id": 7, "name": "SUBTERRANEAN LAVA TUBE", "range": "6,000 - 7,000m", "diff": 4,
                "gimmick": "TILTING PLATFORMS: Basalt slabs pivot and tilt under car weight over lava pit.",
                "type": "steep_climb", "gimmick_type": "lava_tube"
            },
            {
                "id": 8, "name": "METEORITE SWARM FIELD", "range": "7,000 - 8,000m", "diff": 4,
                "gimmick": "BOULDER MINEFIELD: Micro-impact craters and basalt boulders create flip hazards.",
                "type": "cliff_drop", "gimmick_type": "meteor_field"
            },
            {
                "id": 9, "name": "GRAND LUNAR TRENCH", "range": "8,000 - 9,000m", "diff": 5,
                "gimmick": "CANYON STEP DOWNS: Massive step drops where landing misalignments cause fatal flips.",
                "type": "multi_chasm", "gimmick_type": "lunar_trench"
            },
            {
                "id": 10, "name": "ORBITAL LAUNCH RAMP", "range": "9,000 - 10,000m", "diff": 5,
                "gimmick": "ESCAPE VELOCITY JUMP: Climb towering crater rim for orbital leap to spaceport beacon.",
                "type": "summit_leap", "gimmick_type": "orbital_launch"
            }
        ]
    },
    "swamp": {
        "title": "BILEWATER BOG & BAYOU",
        "subtitle": "10 KM MURKY QUAGMIRE",
        "gravity": "9.81 m/s² (EARTH NORMAL)",
        "friction": "0.30 - 0.50 (VISCOUS MUD)",
        "air_drag": "1.15x (HEAVY FOG)",
        "hazards": "Sticky Peat Mud, Rotting Rope Bridges, Drowning Water, Roots",
        "primary_color": (180, 235, 90),
        "accent_color": (115, 195, 45),
        "ground_colors": {
            "top": (95, 135, 45),
            "sub": (65, 48, 28),
            "deep": (38, 26, 15),
            "outline": (22, 16, 9),
        },
        "sections": [
            {
                "id": 1, "name": "MURKY MARSHES", "range": "0 - 1,000m", "diff": 1,
                "gimmick": "PEAT MOSS INTRO: Wet marsh ground; mild tire traction loss and swamp mist.",
                "type": "gentle_slopes", "gimmick_type": "marsh_intro"
            },
            {
                "id": 2, "name": "DEEP MUD WALLOWS", "range": "1,000 - 2,000m", "diff": 2,
                "gimmick": "STICKY MUD PITS: Thick black sludge severely increases rolling drag. Boost needed.",
                "type": "washboard", "gimmick_type": "mud_wallows"
            },
            {
                "id": 3, "name": "ROTTING ROPE BRIDGE", "range": "2,000 - 3,000m", "diff": 3,
                "gimmick": "SNAPPING BRIDGE: Dilapidated rope bridge with rotted timber planks that snap!",
                "type": "chasm", "gimmick_type": "rope_bridge"
            },
            {
                "id": 4, "name": "CYPRESS ROOT TANGLE", "range": "3,000 - 4,000m", "diff": 3,
                "gimmick": "ROOT TRIP HAZARDS: Giant protruding cypress roots form sudden ramps and obstacles.",
                "type": "steep_drop", "gimmick_type": "cypress_roots"
            },
            {
                "id": 5, "name": "ALLIGATOR BAYOU SLOUGH", "range": "4,000 - 5,000m", "diff": 4,
                "gimmick": "DEEP BOG WATER: Stagnant bayou water submerges wheels; engine drowning risk.",
                "type": "chasm", "gimmick_type": "water_bog"
            },
            {
                "id": 6, "name": "BOG SLUDGE TERRACES", "range": "5,000 - 6,000m", "diff": 3,
                "gimmick": "SLIPPERY MOUNDS: Stepped mounds of rotting peat sludge require smooth throttle.",
                "type": "terraces", "gimmick_type": "sludge_terraces"
            },
            {
                "id": 7, "name": "SUNKEN TIMBER TRAIL", "range": "6,000 - 7,000m", "diff": 4,
                "gimmick": "FLOATING LOGS: Half-submerged logs bob and sink into murky water under weight.",
                "type": "steep_climb", "gimmick_type": "floating_logs"
            },
            {
                "id": 8, "name": "TOXIC QUAGMIRE ABYSS", "range": "7,000 - 8,000m", "diff": 4,
                "gimmick": "DECAYING VINE SPAN: Sagging vine bridge over bubbling toxic sludge pit.",
                "type": "cliff_drop", "gimmick_type": "toxic_quagmire"
            },
            {
                "id": 9, "name": "WITCH'S HOLLOW RAVINES", "range": "8,000 - 9,000m", "diff": 5,
                "gimmick": "MULTIPLE BOG GAPS: Chain of 3 treacherous rope bridges over bottomless quagmires.",
                "type": "multi_chasm", "gimmick_type": "multi_bog"
            },
            {
                "id": 10, "name": "ANCIENT WILLOW CANOPY", "range": "9,000 - 10,000m", "diff": 5,
                "gimmick": "CANOPY CREST: Climb out of the muck onto elevated ancient roots to finish line.",
                "type": "summit_leap", "gimmick_type": "willow_finish"
            }
        ]
    }
}

# ==============================================================================
# SECTION TERRAIN GENERATION
# ==============================================================================
def generate_section_terrain(sec_data, w=1920, h=1080):
    """Generate height profile array y[x] across x in [0, w-1]."""
    stype = sec_data["type"]
    sec_id = sec_data["id"]
    y_profile = np.zeros(w, dtype=float)
    base_y = 740 - (sec_id - 1) * 18 # gradually elevates over sections

    for x in range(w):
        nx = x / float(w)
        # General rolling topography
        h_val = base_y - 65 * math.sin(nx * 2 * math.pi) - 35 * math.sin(nx * 5 * math.pi)

        if stype == "gentle_slopes":
            h_val = base_y - 45 * math.sin(nx * 2.5 * math.pi) - 20 * math.cos(nx * 6 * math.pi)
        elif stype == "washboard":
            h_val = base_y - 50 * math.sin(nx * 2 * math.pi) - 35 * math.sin(nx * 14 * math.pi)
        elif stype == "chasm":
            # Chasm in middle: x=800..1150
            if 780 <= x <= 1180:
                h_val = 1120 # deep hole
            elif x < 780:
                h_val = base_y - 60 * math.sin(nx * 3 * math.pi)
            else:
                h_val = base_y - 40 * math.cos(nx * 3 * math.pi)
        elif stype == "steep_drop":
            if nx < 0.4:
                h_val = base_y - 140 + nx * 50
            elif nx < 0.7:
                # Steep drop
                t = (nx - 0.4) / 0.3
                h_val = base_y - 120 + t * 240
            else:
                h_val = base_y + 120 - (nx - 0.7) * 80
        elif stype == "terraces":
            step = int(nx * 5)
            h_val = base_y - step * 40 - 15 * math.sin(nx * 25 * math.pi)
        elif stype == "steep_climb":
            h_val = (base_y + 80) - (nx ** 1.3) * 220 - 25 * math.sin(nx * 8 * math.pi)
        elif stype == "cliff_drop":
            if nx < 0.45:
                h_val = base_y - 160
            elif nx < 0.55:
                h_val = 1120 # drop abyss
            else:
                h_val = base_y + 60 - 25 * math.sin(nx * 6 * math.pi)
        elif stype == "multi_chasm":
            # 2 chasms
            if (600 <= x <= 820) or (1150 <= x <= 1380):
                h_val = 1120
            else:
                h_val = base_y - 45 * math.sin(nx * 4 * math.pi)
        elif stype == "summit_leap":
            if nx < 0.65:
                # Mountain climb ramp
                h_val = (base_y + 100) - (nx / 0.65) * 260
            elif nx < 0.82:
                # Summit chasm
                h_val = 1120
            else:
                # Finish plateau
                h_val = base_y - 150

        y_profile[x] = np.clip(h_val, 150, 1120)

    return y_profile

# ==============================================================================
# RENDER SINGLE 1920x1080 SECTION IMAGE
# ==============================================================================
def render_section_image(theme_name, theme_data, sec_data, car_sprite, wheel_sprite):
    w, h = 1920, 1080
    img = Image.new("RGBA", (w, h))
    draw = ImageDraw.Draw(img)

    # 1. Background Sky & Distant Panorama
    # Draw backdrop gradient
    g_top = theme_data["ground_colors"]["top"]
    g_sub = theme_data["ground_colors"]["sub"]
    g_deep = theme_data["ground_colors"]["deep"]
    g_out = theme_data["ground_colors"]["outline"]

    bg_arr = np.zeros((h, w, 4), dtype=np.uint8)
    for y in range(h):
        t = y / float(h)
        if theme_name == "snow":
            r = int(15 + 80 * t)
            g = int(25 + 140 * t)
            b = int(60 + 180 * t)
        elif theme_name == "beach":
            r = int(50 + 150 * t)
            g = int(150 + 90 * t)
            b = int(245 + 10 * t)
        elif theme_name == "moon":
            r = int(5 + 10 * t)
            g = int(8 + 12 * t)
            b = int(18 + 20 * t)
        elif theme_name == "swamp":
            r = int(35 + 95 * t)
            g = int(48 + 75 * t)
            b = int(32 + 35 * t)
        bg_arr[y, :, :] = [r, g, b, 255]

    bg_img = Image.fromarray(bg_arr, "RGBA")
    b_draw = ImageDraw.Draw(bg_img)

    # Celestial / Atmosphere features
    if theme_name == "snow":
        # Crescent cold moon
        b_draw.ellipse([1540, 120, 1610, 190], fill=(240, 248, 255, 230))
        b_draw.ellipse([1560, 110, 1630, 180], fill=(25, 45, 95, 255))
        # Snowy mountains silhouette in midground
        for mx in range(0, w, 4):
            my = int(480 - 110 * math.sin(mx * 0.005) - 60 * math.cos(mx * 0.015))
            b_draw.line([(mx, my), (mx, 750)], fill=(85, 120, 165, 120), width=4)
    elif theme_name == "beach":
        # Blazing sun
        b_draw.ellipse([1420, 140, 1540, 260], fill=(255, 255, 240, 255))
        b_draw.ellipse([1380, 100, 1580, 300], fill=(255, 245, 180, 70))
        # Distant islands
        for mx in range(0, w, 4):
            my = int(620 - 45 * math.sin(mx * 0.006))
            b_draw.line([(mx, my), (mx, 750)], fill=(70, 135, 165, 140), width=4)
    elif theme_name == "moon":
        # Planet Earth
        b_draw.ellipse([1400, 130, 1520, 250], fill=(70, 160, 255, 60))
        b_draw.ellipse([1410, 140, 1510, 240], fill=(30, 105, 205, 255))
        b_draw.ellipse([1430, 160, 1480, 210], fill=(60, 150, 80, 220))
        # Stars
        np.random.seed(sec_data["id"] * 17)
        for _ in range(250):
            sx, sy = np.random.randint(0, w), np.random.randint(0, 650)
            c = np.random.randint(180, 255)
            b_draw.point((sx, sy), fill=(c, c, 255, 255))
    elif theme_name == "swamp":
        # Hazy pale full moon
        b_draw.ellipse([450, 160, 560, 270], fill=(210, 220, 175, 180))
        # Distant cypress canopy
        for mx in range(0, w, 4):
            my = int(520 - 55 * math.sin(mx * 0.008) - 30 * math.sin(mx * 0.03))
            b_draw.line([(mx, my), (mx, 780)], fill=(45, 58, 40, 160), width=4)

    img = Image.alpha_composite(img, bg_img)
    draw = ImageDraw.Draw(img)

    # 2. Generate and Render Terrain Profile
    y_profile = generate_section_terrain(sec_data, w, h)

    # Draw solid multi-layered ground strata
    for x in range(w):
        top_y = int(y_profile[x])
        if top_y >= h:
            continue
        
        # Strata depths
        surf_depth = 18
        sub_depth = 90
        
        for y in range(top_y, h):
            depth = y - top_y
            if depth <= 3:
                # Sharp outline edge
                draw.point((x, y), fill=g_out + (255,))
            elif depth <= surf_depth:
                # Top layer (snow, sand, regolith, moss)
                draw.point((x, y), fill=g_top + (255,))
            elif depth <= surf_depth + sub_depth:
                # Sub-surface layer
                draw.point((x, y), fill=g_sub + (255,))
            else:
                # Deep bedrock strata
                # Add stone texture noise
                speck = (math.sin(x * 1.5 + y * 2.3) > 0.85)
                col = g_deep if not speck else tuple(min(255, v + 25) for v in g_deep)
                draw.point((x, y), fill=col + (255,))

    # 3. Render Specific In-Track Gimmicks & Hazards
    gimmick_type = sec_data["gimmick_type"]

    if gimmick_type in ("fragile_ice", "crash_ice", "multi_ice"):
        # Draw translucent cracked ice bridges spanning the chasms
        chasm_spans = [(780, 1180)] if gimmick_type != "multi_ice" else [(600, 820), (1150, 1380)]
        for left_x, right_x in chasm_spans:
            bridge_y = int(y_profile[left_x - 5])
            # Draw ice shelf
            draw.rectangle([left_x, bridge_y - 6, right_x, bridge_y + 14], fill=(160, 230, 255, 230), outline=(100, 190, 240, 255), width=2)
            # Fracture stress cracks
            for fx in range(left_x + 30, right_x - 30, 45):
                draw.line([(fx, bridge_y - 4), (fx + 10, bridge_y + 12)], fill=(255, 255, 255, 255), width=2)
                draw.line([(fx + 10, bridge_y + 12), (fx + 22, bridge_y + 6)], fill=(220, 245, 255, 255), width=2)
            # Hanging stalactite icicles
            for ix in range(left_x + 15, right_x - 15, 20):
                icicle_len = np.random.randint(12, 35)
                draw.polygon([(ix - 3, bridge_y + 14), (ix + 3, bridge_y + 14), (ix, bridge_y + 14 + icicle_len)], fill=(210, 245, 255, 220))
            # Caution warning beacon
            draw.rectangle([left_x - 35, bridge_y - 50, left_x - 5, bridge_y], fill=(220, 40, 40, 255))
            draw.text((left_x - 30, bridge_y - 45), "!", fill=(255, 255, 255), font=FONT_HEADING)
            # Floating warning text
            draw.text((left_x + 40, bridge_y - 45), "! FRAGILE ICE: BREAKS ON HIGH SPEED / LOAD !", fill=(200, 240, 255), font=FONT_TEXT)

    elif gimmick_type == "timber_bridge":
        # Breakable wooden trestle
        left_x, right_x = 780, 1180
        bridge_y = int(y_profile[left_x - 5])
        # Wooden deck planks
        draw.rectangle([left_x, bridge_y - 4, right_x, bridge_y + 8], fill=(130, 95, 60, 255), outline=(75, 50, 30, 255), width=2)
        for px in range(left_x, right_x, 15):
            draw.line([(px, bridge_y - 4), (px, bridge_y + 8)], fill=(75, 50, 30, 255), width=2)
        # Broken/missing plank gap in middle
        draw.rectangle([960, bridge_y - 5, 995, bridge_y + 9], fill=(0, 0, 0, 0)) # broken
        # Support ropes & timber stilts
        draw.line([(left_x, bridge_y + 4), (left_x + 200, bridge_y + 120)], fill=(90, 65, 40, 255), width=4)
        draw.line([(right_x, bridge_y + 4), (right_x - 200, bridge_y + 120)], fill=(90, 65, 40, 255), width=4)
        draw.text((left_x + 30, bridge_y - 45), "! WEAK TRESTLE: IMPACT SHATTERS STRUCTURE !", fill=(255, 180, 100), font=FONT_TEXT)

    elif gimmick_type in ("ice_patch", "slip_warning", "frozen_waterfall"):
        # Blue icy glossy slip tracks
        draw.rectangle([800, 600, 1300, 615], fill=(120, 220, 255, 140))
        draw.text((820, 565), "SLIP HAZARD: TRACTION 0.12", fill=(160, 230, 255), font=FONT_TEXT)

    elif gimmick_type in ("wooden_pier", "coral_gaps", "dune_jumps"):
        # Wooden seaside boardwalk stilt bridge
        chasm_spans = [(780, 1180)] if sec_data["type"] != "multi_chasm" else [(600, 820), (1150, 1380)]
        for left_x, right_x in chasm_spans:
            bridge_y = int(min(y_profile[left_x - 5], 780))
            # Water underneath
            draw.rectangle([left_x, max(bridge_y + 40, 850), right_x, 1080], fill=(20, 140, 210, 220))
            draw.text((left_x + 20, max(bridge_y + 50, 860)), "~ TIDAL SURGE ~", fill=(180, 235, 255), font=FONT_HEADING)
            # Pier Deck
            draw.rectangle([left_x, bridge_y - 6, right_x, bridge_y + 10], fill=(155, 120, 75, 255), outline=(90, 65, 40, 255), width=2)
            for px in range(left_x, right_x, 18):
                draw.line([(px, bridge_y - 6), (px, bridge_y + 10)], fill=(90, 65, 40, 255), width=2)
            # Pilings
            p_bottom = max(bridge_y + 40, 960)
            for px in range(left_x + 40, right_x - 20, 90):
                draw.rectangle([px - 5, bridge_y + 10, px + 5, p_bottom], fill=(110, 80, 50, 255))
            draw.text((left_x + 30, bridge_y - 45), "! RICKETY BOARDWALK: LOOSE PLANKS !", fill=(255, 220, 120), font=FONT_TEXT)

    elif gimmick_type == "quicksand":
        # Sinking sand pit
        draw.rectangle([750, 720, 1200, 840], fill=(210, 165, 95, 255), outline=(140, 95, 45, 255), width=3)
        draw.text((820, 760), "! DANGER: QUICKSAND PIT !", fill=(180, 40, 40), font=FONT_HEADING)
        draw.text((840, 795), "VEHICLE SINKS ON HALT", fill=(255, 240, 200), font=FONT_TEXT)

    elif gimmick_type in ("floating_basalt", "impact_rift", "lunar_trench"):
        # Floating low-gravity basalt slabs
        chasm_spans = [(780, 1180)] if sec_data["type"] != "multi_chasm" else [(600, 820), (1150, 1380)]
        for left_x, right_x in chasm_spans:
            bridge_y = int(min(y_profile[left_x - 5], 780))
            for slab_idx, sx in enumerate(range(left_x + 40, right_x - 40, 110)):
                sy = bridge_y + int(15 * math.sin(slab_idx * 2))
                draw.polygon([(sx, sy), (sx + 85, sy), (sx + 75, sy + 25), (sx - 10, sy + 25)],
                             fill=(135, 140, 155, 255), outline=(210, 220, 235, 255), width=2)
                # Blue magnetic levitation glow
                draw.ellipse([sx - 5, sy + 25, sx + 80, sy + 40], fill=(60, 180, 255, 80))
            draw.text((left_x + 40, bridge_y - 50), "1/6th G: FLOATING BASALT STEPPING STONES", fill=(140, 210, 255), font=FONT_TEXT)

    elif gimmick_type in ("crater_bounce", "orbital_launch"):
        # Trajectory arc
        draw.arc([600, 320, 1300, 780], 200, 340, fill=(100, 210, 255, 200), width=3)
        draw.text((780, 460), "LOW GRAVITY CATAPULT TRAJECTORY", fill=(160, 230, 255), font=FONT_TEXT)

    elif gimmick_type in ("rope_bridge", "multi_bog", "toxic_quagmire"):
        # Rotting rope suspension bridge over murky mud/water
        chasm_spans = [(780, 1180)] if sec_data["type"] != "multi_chasm" else [(600, 820), (1150, 1380)]
        for left_x, right_x in chasm_spans:
            bridge_y = int(min(y_profile[left_x - 5], 780))
            # Murky mud water below
            draw.rectangle([left_x, max(bridge_y + 40, 850), right_x, 1080], fill=(35, 45, 22, 255))
            draw.text((left_x + 20, max(bridge_y + 50, 860)), "DEEP QUAGMIRE SLUDGE", fill=(140, 190, 75), font=FONT_HEADING)
            # Sagging bridge cable
            cable_pts = []
            for bx in range(left_x, right_x + 1, 10):
                st = (bx - left_x) / float(right_x - left_x)
                sag = 45 * math.sin(st * math.pi)
                cable_pts.append((bx, bridge_y + sag))
            draw.line(cable_pts, fill=(90, 70, 45, 255), width=5)
            # Planks
            for bx, by in cable_pts[::3]:
                draw.rectangle([bx - 6, by - 4, bx + 6, by + 6], fill=(110, 85, 55, 255), outline=(50, 38, 24, 255))
            draw.text((left_x + 30, bridge_y - 45), "! ROTTING ROPE BRIDGE: SNAPS UNDER STRESS !", fill=(180, 230, 70), font=FONT_TEXT)

    elif gimmick_type == "mud_wallows":
        # Deep bubbling mud wallow
        draw.rectangle([700, 700, 1250, 820], fill=(45, 30, 16, 255), outline=(25, 16, 8, 255), width=3)
        draw.text((780, 735), "STICKY MUD WALLOW: HIGH DRAG", fill=(195, 140, 80), font=FONT_HEADING)
        draw.text((820, 770), "MAINTAIN CONTINUOUS BOOST", fill=(235, 200, 150), font=FONT_TEXT)

    # 4. Composite Themed Vehicle in the Scene
    if car_sprite and wheel_sprite:
        car_scale = 0.16
        c_w = int(car_sprite.width * car_scale)
        c_h = int(car_sprite.height * car_scale)
        car_small = car_sprite.resize((c_w, c_h), Image.Resampling.LANCZOS)
        
        w_size = int(wheel_sprite.width * car_scale * 0.45)
        wheel_small = wheel_sprite.resize((w_size, w_size), Image.Resampling.LANCZOS)

        # Vehicle position on the track
        vx = 480
        vy = int(y_profile[vx]) - c_h + 10
        if vy > 150 and vy < h - 150:
            # Place wheels
            img.alpha_composite(wheel_small, (vx + 32, vy + c_h - w_size // 2 - 4))
            img.alpha_composite(wheel_small, (vx + c_w - 75, vy + c_h - w_size // 2 - 4))
            # Place body
            img.alpha_composite(car_small, (vx, vy))

    # 5. Professional HUD & Technical Telemetry Card Overlay
    # Top-Left Header Banner
    draw.rectangle([40, 35, 680, 140], fill=(15, 22, 35, 225), outline=theme_data["primary_color"] + (255,), width=2)
    draw.text((55, 45), f"{theme_data['title']} [10KM]", fill=theme_data["primary_color"], font=FONT_HEADING)
    draw.text((55, 75), f"ZONE {sec_data['id']:02d}: {sec_data['name']}", fill=(255, 255, 255), font=FONT_TITLE)
    draw.text((55, 112), f"INTERVAL: {sec_data['range']} | DIFFICULTY: {'★' * sec_data['diff']}{'☆' * (5 - sec_data['diff'])}",
              fill=theme_data["accent_color"], font=FONT_TEXT)

    # Top-Right Telemetry Spec Box
    draw.rectangle([1220, 35, 1880, 160], fill=(15, 22, 35, 225), outline=theme_data["primary_color"] + (255,), width=2)
    draw.text((1235, 45), "ENVIRONMENT TELEMETRY", fill=theme_data["primary_color"], font=FONT_HEADING)
    draw.text((1235, 75), f"GRAVITY: {theme_data['gravity']}", fill=(240, 245, 255), font=FONT_TEXT)
    draw.text((1235, 98), f"SURFACE FRICTION: {theme_data['friction']}", fill=(240, 245, 255), font=FONT_TEXT)
    draw.text((1235, 122), f"HAZARDS: {theme_data['hazards']}", fill=(255, 180, 80), font=FONT_SMALL)

    # Bottom Mechanics Advisory Ribbon
    draw.rectangle([40, 975, 1880, 1045], fill=(12, 18, 28, 235), outline=theme_data["accent_color"] + (255,), width=2)
    draw.text((55, 985), f"ZONE MECHANIC:", fill=theme_data["accent_color"], font=FONT_HEADING)
    draw.text((260, 987), sec_data["gimmick"], fill=(255, 255, 255), font=FONT_TEXT)

    # 10-Section Navigation Progress Bar
    bar_x, bar_y, bar_w, bar_h = 55, 1018, 1810, 16
    draw.rectangle([bar_x, bar_y, bar_x + bar_w, bar_y + bar_h], fill=(30, 40, 55, 255))
    seg_w = bar_w / 10.0
    for s_i in range(10):
        sx = bar_x + s_i * seg_w
        if s_i + 1 == sec_data["id"]:
            # Active section highlighted
            draw.rectangle([sx + 2, bar_y + 2, sx + seg_w - 2, bar_y + bar_h - 2], fill=theme_data["accent_color"] + (255,))
        elif s_i + 1 < sec_data["id"]:
            # Cleared section
            draw.rectangle([sx + 2, bar_y + 2, sx + seg_w - 2, bar_y + bar_h - 2], fill=(70, 95, 130, 255))
        else:
            # Future section
            draw.rectangle([sx + 2, bar_y + 2, sx + seg_w - 2, bar_y + bar_h - 2], fill=(45, 55, 75, 255))

    return img

# ==============================================================================
# RENDER 10KM FULL OVERVIEW INFOGRAPHIC MAP (3840x1080)
# ==============================================================================
def render_10km_overview_map(theme_name, theme_data):
    w, h = 3840, 1080
    img = Image.new("RGBA", (w, h))
    draw = ImageDraw.Draw(img)

    # Dark technical background
    draw.rectangle([0, 0, w, h], fill=(10, 14, 22, 255))
    # Grid lines
    for gx in range(0, w, 80):
        draw.line([(gx, 0), (gx, h)], fill=(20, 28, 42, 255), width=1)
    for gy in range(0, h, 60):
        draw.line([(0, gy), (w, gy)], fill=(20, 28, 42, 255), width=1)

    p_col = theme_data["primary_color"]
    a_col = theme_data["accent_color"]

    # Top Title Banner
    draw.rectangle([60, 40, w - 60, 180], fill=(15, 24, 38, 250), outline=p_col + (255,), width=3)
    draw.text((90, 55), f"{theme_data['title']} — FULL 10,000 METRE COURSE OVERVIEW", fill=p_col, font=FONT_TITLE)
    draw.text((90, 95), f"{theme_data['subtitle']} | 10 SECTIONS | VARYING DIFFICULTY & MECHANICS", fill=a_col, font=FONT_HEADING)
    draw.text((90, 135), f"GRAVITY: {theme_data['gravity']}   |   FRICTION: {theme_data['friction']}   |   DRAG: {theme_data['air_drag']}   |   HAZARDS: {theme_data['hazards']}",
              fill=(230, 240, 255), font=FONT_TEXT)

    # Course Elevation Profile Graph Area: x from 120 to w - 120, y from 260 to 760
    gx_start = 120
    gx_end = w - 120
    gw = gx_end - gx_start
    gy_base = 740

    # Draw continuous 10km elevation profile across all 10 sections
    profile_pts = []
    sec_w = gw / 10.0

    for i in range(gw):
        nx = i / float(gw)
        sec_idx = min(9, int(nx * 10))
        sec = theme_data["sections"][sec_idx]
        local_nx = (nx * 10) - sec_idx

        # Elevation formula tailored to 10km gauntlet
        base_h = 100 + sec_idx * 35 # continuous climb over 10km
        h_noise = 40 * math.sin(nx * 30 * math.pi) + 25 * math.sin(nx * 75 * math.pi)
        
        if sec["type"] == "chasm":
            if 0.45 <= local_nx <= 0.65:
                elev = base_h - 110 # chasm dip
            else:
                elev = base_h + h_noise
        elif sec["type"] == "steep_drop":
            elev = base_h + 120 * (1.0 - local_nx) + h_noise * 0.5
        elif sec["type"] == "steep_climb":
            elev = base_h + 180 * (local_nx ** 1.2) + h_noise * 0.5
        elif sec["type"] == "multi_chasm":
            if (0.3 <= local_nx <= 0.45) or (0.65 <= local_nx <= 0.8):
                elev = base_h - 90
            else:
                elev = base_h + h_noise
        elif sec["type"] == "summit_leap":
            elev = base_h + 240 * min(1.0, local_nx * 1.4)
        else:
            elev = base_h + h_noise

        py = gy_base - elev
        profile_pts.append((gx_start + i, py))

    # Fill ground polygon under profile
    poly_pts = [(gx_start, gy_base + 60)] + profile_pts + [(gx_end, gy_base + 60)]
    draw.polygon(poly_pts, fill=theme_data["ground_colors"]["deep"] + (180,))
    # Profile line
    draw.line(profile_pts, fill=p_col + (255,), width=4)

    # Draw 10 Section Dividing Columns & Labels
    for s_i in range(10):
        sx = gx_start + s_i * sec_w
        sec = theme_data["sections"][s_i]

        # Vertical demarcation line
        draw.line([(sx, 220), (sx, gy_base + 70)], fill=(60, 85, 120, 180), width=2)

        # Section Card Box
        card_y = 800
        draw.rectangle([sx + 6, card_y, sx + sec_w - 6, card_y + 220],
                       fill=(16, 24, 36, 240), outline=a_col + (200,), width=2)
        
        draw.text((sx + 14, card_y + 12), f"SEC {sec['id']:02d}", fill=p_col, font=FONT_TITLE)
        draw.text((sx + 14, card_y + 48), sec["name"], fill=(255, 255, 255), font=FONT_TEXT)
        draw.text((sx + 14, card_y + 75), f"RANGE: {sec['range']}", fill=a_col, font=FONT_SMALL)
        draw.text((sx + 14, card_y + 95), f"DIFF: {'★' * sec['diff']}{'☆' * (5 - sec['diff'])}", fill=(255, 215, 60), font=FONT_SMALL)
        
        # Wrapped gimmick text
        words = sec["gimmick"].split(" ")
        line1 = " ".join(words[:5])
        line2 = " ".join(words[5:11])
        line3 = " ".join(words[11:])
        draw.text((sx + 14, card_y + 125), line1, fill=(210, 225, 240), font=FONT_TINY)
        draw.text((sx + 14, card_y + 145), line2, fill=(210, 225, 240), font=FONT_TINY)
        draw.text((sx + 14, card_y + 165), line3, fill=(210, 225, 240), font=FONT_TINY)

        # Distance tag on the graph
        draw.text((sx + 10, 230), f"{(s_i)*1000}m", fill=(160, 190, 225), font=FONT_SMALL)

    # Final 10000m marker
    draw.line([(gx_end, 220), (gx_end, gy_base + 70)], fill=(60, 85, 120, 180), width=2)
    draw.text((gx_end - 45, 230), "10000m", fill=(160, 190, 225), font=FONT_SMALL)

    # Checkered Finish Flag at 10000m
    fx, fy = gx_end - 15, int(profile_pts[-1][1]) - 60
    draw.rectangle([fx, fy, fx + 30, fy + 24], fill=(255, 255, 255))
    draw.rectangle([fx, fy, fx + 15, fy + 12], fill=(0, 0, 0))
    draw.rectangle([fx + 15, fy + 12, fx + 30, fy + 24], fill=(0, 0, 0))
    draw.line([(fx, fy), (fx, fy + 65)], fill=(200, 200, 200), width=3)
    draw.text((fx - 40, fy - 25), "FINISH", fill=(255, 255, 255), font=FONT_HEADING)

    return img

# ==============================================================================
# MAIN EXECUTION
# ==============================================================================
def main():
    print("=================================================================")
    print("STARTING FULL MAP & SECTION GENERATION FOR 4 THEMES")
    print("=================================================================")

    for theme_name, theme_data in MAP_DATA.items():
        print(f"\n>>> PROCESSING THEME: {theme_name.upper()} ({theme_data['title']})")
        
        # Load theme vehicle sprites if available
        car_path = os.path.join(TEMP_DIR, theme_name, "vehicle", "car_body.png")
        wheel_path = os.path.join(TEMP_DIR, theme_name, "vehicle", "wheel.png")
        car_sprite = Image.open(car_path).convert("RGBA") if os.path.exists(car_path) else None
        wheel_sprite = Image.open(wheel_path).convert("RGBA") if os.path.exists(wheel_path) else None

        sec_out_dir = os.path.join(TEMP_DIR, theme_name, "sections")
        os.makedirs(sec_out_dir, exist_ok=True)

        # 1. Generate 10 Section Images
        for sec in theme_data["sections"]:
            sec_img = render_section_image(theme_name, theme_data, sec, car_sprite, wheel_sprite)
            # Save standard section_XX.png
            file_name = f"section_{sec['id']:02d}.png"
            full_path = os.path.join(sec_out_dir, file_name)
            sec_img.save(full_path, optimize=True)
            print(f"  [+] Saved {file_name}: {sec['name']} ({sec_img.size})")

        # 2. Generate 10km Full Overview Map
        print(f"  Generating 10km Overview Map for {theme_name}...")
        overview_img = render_10km_overview_map(theme_name, theme_data)
        over_out_dir = os.path.join(TEMP_DIR, theme_name, "overview")
        os.makedirs(over_out_dir, exist_ok=True)
        over_path = os.path.join(over_out_dir, "map_overview_10km.png")
        overview_img.save(over_path, optimize=True)
        print(f"  [+] Saved {over_path} ({overview_img.size})")

    print("\n=================================================================")
    print("ALL 40 SECTION IMAGES + 4 10KM OVERVIEW MAPS GENERATED SUCCESSFULLY!")
    print("=================================================================")

if __name__ == "__main__":
    main()
