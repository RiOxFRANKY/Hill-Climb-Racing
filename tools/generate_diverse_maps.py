#!/usr/bin/env python3
"""
Generate radically diverse, unique, environment-specific maps and sections:
  1. Snow (Frostbite Glaciers & Alpine Horns):
     - Ski-jump slopes, serac ice mazes, 18m fragile ice crevasse, avalanche crash chute,
       timber railroad trestle, blizzard switchbacks, vertical frozen waterfall ice wall,
       2-tier cliff drop to fragile ice, crevasse gauntlet, horn peak summit leap.
  2. Beach (Sunken Dunes & Coral Coast):
     - Low-tide strand, barchan dunes with steep slip-faces, broken boardwalk pier on stilts,
       concave quicksand sink pit, coral sea arch & cave, tidal lagoon water splash basin,
       razor sand ridge, shipwreck galleon bridge, 3-lagoon dune jumps, lighthouse clifftop.
  3. Moon (Mare Tranquillitatis Craterlands):
     - 1/6th gravity mare plain, giant circular impact crater bowl with central peak,
       levitating magnetic basalt slabs, solar collector array causeway, 500px super-jump chasm,
       sharp basalt needle spires, subterranean lava tube, pockmarked meteorite minefield,
       giant step-down crater trench, orbital spaceport catapult ramp.
  4. Swamp (Bilewater Bog & Bayou):
     - Peat moss shallows, deep bubbling black mud wallows, rotting rope suspension bridge,
       giant cypress root cage tangles, alligator slough deep water, slimy sludge terraces,
       sunken floating stepping logs, toxic quagmire abyss, triple ravine bog hop,
       ancient willow canopy crest.

Outputs:
  temp/{snow,beach,moon,swamp}/sections/section_01.png .. section_10.png (40 images @ 1920x1080)
  temp/{snow,beach,moon,swamp}/overview/map_overview_10km.png           (4 images @ 3840x1080)
"""

import os
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS_DIR = os.path.join(ROOT, "assets")
TEMP_DIR = os.path.join(ROOT, "temp")

FONT_TITLE = ImageFont.truetype(os.path.join(ASSETS_DIR, "Silkscreen-Bold.ttf"), 28)
FONT_HEADING = ImageFont.truetype(os.path.join(ASSETS_DIR, "Silkscreen-Bold.ttf"), 20)
FONT_TEXT = ImageFont.truetype(os.path.join(ASSETS_DIR, "Silkscreen-Regular.ttf"), 15)
FONT_SMALL = ImageFont.truetype(os.path.join(ASSETS_DIR, "PressStart2P-Regular.ttf"), 11)
FONT_TINY = ImageFont.truetype(os.path.join(ASSETS_DIR, "PressStart2P-Regular.ttf"), 9)

# ==============================================================================
# BESPOKE TERRAIN ELEVATION CURVES PER MAP & SECTION
# ==============================================================================

def get_snow_elevation(sec_id, w=1920):
    y = np.zeros(w, dtype=float)
    if sec_id == 1: # Glacier Tongue Runout: Long downhill parabolic chute to launch ramp
        for x in range(w):
            nx = x / float(w)
            if nx < 0.7:
                y[x] = 450 + 350 * (nx / 0.7) ** 1.8
            else:
                # launch curve
                t = (nx - 0.7) / 0.3
                y[x] = 800 - 120 * math.sin(t * math.pi * 0.7)
    elif sec_id == 2: # Serac Ice Maze: Jagged, stepped sawtooth ice pinnacles
        for x in range(w):
            nx = x / float(w)
            saw = (math.sin(nx * 14 * math.pi) + math.cos(nx * 7 * math.pi)) * 60
            y[x] = 720 - saw - 80 * math.sin(nx * 2 * math.pi)
    elif sec_id == 3: # The Abyssal Crevasse: Cliff on left, massive 380px yawning hole, ascending right
        for x in range(w):
            if x < 750:
                y[x] = 520 - 40 * math.sin(x * 0.005)
            elif x > 1180:
                y[x] = 560 + 60 * math.cos((x - 1180) * 0.005)
            else:
                y[x] = 1150 # deep bottomless pit
    elif sec_id == 4: # Avalanche Slide: High 55-degree continuous drop of 400px
        for x in range(w):
            nx = x / float(w)
            if nx < 0.2:
                y[x] = 420 + nx * 50
            elif nx < 0.75:
                t = (nx - 0.2) / 0.55
                y[x] = 430 + t * 440
            else:
                t = (nx - 0.75) / 0.25
                y[x] = 870 - 100 * math.sin(t * math.pi * 0.6)
    elif sec_id == 5: # Timber Trestle Ravine: Canyon walls with 420px ravine gap
        for x in range(w):
            if x < 740:
                y[x] = 600 - 80 * math.sin(x * 0.006)
            elif x > 1200:
                y[x] = 620 - 60 * math.sin(x * 0.006)
            else:
                y[x] = 1150
    elif sec_id == 6: # Blizzard Pass Switchbacks: 4 distinct stepped flat terraces
        for x in range(w):
            nx = x / float(w)
            step = int(nx * 4)
            local_t = (nx * 4) - step
            step_base = 780 - step * 110
            y[x] = step_base - 30 * math.sin(local_t * math.pi)
    elif sec_id == 7: # Frozen Waterfall Wall: Near-vertical ice wall from 850 up to 350
        for x in range(w):
            nx = x / float(w)
            if nx < 0.35:
                y[x] = 840 - nx * 80
            elif nx < 0.65:
                t = (nx - 0.35) / 0.30
                # Steep S-curve wall
                s_curve = (1 - math.cos(t * math.pi)) * 0.5
                y[x] = 812 - s_curve * 460
            else:
                y[x] = 352 - (nx - 0.65) * 60
    elif sec_id == 8: # Fragile Ice Shelf Drop: 2-tier cliff drop
        for x in range(w):
            if x < 600:
                y[x] = 420
            elif 600 <= x < 800:
                y[x] = 1150 # drop gap
            elif 800 <= x < 1250:
                y[x] = 720 # middle ice shelf
            elif 1250 <= x < 1450:
                y[x] = 1150 # second pit
            else:
                y[x] = 820
    elif sec_id == 9: # Five-Crevasse Gauntlet: 3 sharp crevasse pits with spires
        for x in range(w):
            nx = x / float(w)
            if (500 <= x <= 700) or (900 <= x <= 1100) or (1300 <= x <= 1500):
                y[x] = 1150
            else:
                y[x] = 660 - 50 * math.sin(nx * 8 * math.pi)
    else: # sec_id == 10: Horn Peak Summit & Leap
        for x in range(w):
            nx = x / float(w)
            if nx < 0.65:
                y[x] = 820 - (nx / 0.65) * 480 # soaring mountain horn
            elif nx < 0.82:
                y[x] = 1150 # summit leap gap
            else:
                y[x] = 440 # finish plateau
    return np.clip(y, 160, 1150)


def get_beach_elevation(sec_id, w=1920):
    y = np.zeros(w, dtype=float)
    if sec_id == 1: # Low Tide Strand: Gentle flat shoreline with gentle sand waves
        for x in range(w):
            nx = x / float(w)
            y[x] = 760 - 25 * math.sin(nx * 4 * math.pi)
    elif sec_id == 2: # Barchan Dunes: Asymmetric dunes (gentle climb, steep slip-face drop)
        for x in range(w):
            nx = (x % 480) / 480.0
            if nx < 0.75:
                dune = (nx / 0.75) * 160
            else:
                dune = 160 - ((nx - 0.75) / 0.25) * 160 # steep slip face
            y[x] = 800 - dune
    elif sec_id == 3: # Broken Pier Boardwalk: Flat road over 450px water gap
        for x in range(w):
            if x < 720:
                y[x] = 720 - 30 * math.sin(x * 0.006)
            elif x > 1220:
                y[x] = 740 - 25 * math.cos(x * 0.006)
            else:
                y[x] = 1150 # water channel below pier
    elif sec_id == 4: # Quicksand Pit: Deep concave bowl between two dunes
        for x in range(w):
            nx = x / float(w)
            if 0.35 <= nx <= 0.65:
                t = (nx - 0.35) / 0.30
                bowl = math.sin(t * math.pi) * 220
                y[x] = 680 + bowl # sunken pit
            else:
                y[x] = 680 - 60 * math.sin(nx * 4 * math.pi)
    elif sec_id == 5: # Coral Sea Arch & Caves: Natural limestone arch with tunnel
        for x in range(w):
            nx = x / float(w)
            y[x] = 740 - 140 * math.sin(nx * math.pi) - 25 * math.sin(nx * 6 * math.pi)
    elif sec_id == 6: # Tidal Lagoon Basin: Road dips directly into water
        for x in range(w):
            nx = x / float(w)
            if 0.30 <= nx <= 0.70:
                t = (nx - 0.30) / 0.40
                y[x] = 720 + math.sin(t * math.pi) * 140 # water dip
            else:
                y[x] = 720 - 40 * math.cos(nx * 3 * math.pi)
    elif sec_id == 7: # Razor Sand Ridge: High peaked dunes with undulating ridge
        for x in range(w):
            nx = x / float(w)
            y[x] = 620 - 180 * (math.sin(nx * 3 * math.pi) ** 2)
    elif sec_id == 8: # Shipwreck Cove: Coastal cove with wrecked hull bridging gap
        for x in range(w):
            if x < 700:
                y[x] = 660 - 50 * math.sin(x * 0.007)
            elif x > 1220:
                y[x] = 680 - 45 * math.sin(x * 0.007)
            else:
                y[x] = 1150
    elif sec_id == 9: # Three Lagoon Jumps: 3 dune ramps and water channels
        for x in range(w):
            if (520 <= x <= 720) or (950 <= x <= 1150) or (1380 <= x <= 1580):
                y[x] = 1150
            else:
                y[x] = 720 - 60 * math.sin((x / float(w)) * 9 * math.pi)
    else: # sec_id == 10: Clifftop Lighthouse Headland
        for x in range(w):
            nx = x / float(w)
            y[x] = 840 - (nx ** 0.85) * 420
    return np.clip(y, 160, 1150)


def get_moon_elevation(sec_id, w=1920):
    y = np.zeros(w, dtype=float)
    if sec_id == 1: # Mare Tranquillitatis Plains: Vast low-g rolling mare
        for x in range(w):
            nx = x / float(w)
            y[x] = 720 - 40 * math.sin(nx * 3 * math.pi) - 20 * math.sin(nx * 8 * math.pi)
    elif sec_id == 2: # Tycho Crater Bowl: Massive concave impact basin with central peak
        for x in range(w):
            nx = x / float(w)
            if 0.15 <= nx <= 0.85:
                t = (nx - 0.15) / 0.70
                basin = math.sin(t * math.pi) * 260
                # Central peak at t=0.5
                c_peak = 110 * math.exp(-((t - 0.5) / 0.08) ** 2)
                y[x] = 580 + basin - c_peak
            else:
                y[x] = 580 - 40 * math.sin(nx * 4 * math.pi)
    elif sec_id == 3: # Levitating Basalt Arch: 480px chasm with floating rocks
        for x in range(w):
            if x < 700:
                y[x] = 640 - 50 * math.sin(x * 0.006)
            elif x > 1240:
                y[x] = 620 - 45 * math.sin(x * 0.006)
            else:
                y[x] = 1150 # deep space void
    elif sec_id == 4: # Solar Array Causeway: Narrow flat mesa road
        for x in range(w):
            nx = x / float(w)
            y[x] = 640 - 30 * math.sin(nx * 5 * math.pi)
    elif sec_id == 5: # The Great Impact Chasm: Huge 520px vacuum abyss
        for x in range(w):
            if x < 680:
                y[x] = 560 + (x / 680.0) * 80 # launch ramp
            elif x > 1280:
                y[x] = 720 - ((x - 1280) / 640.0) * 60
            else:
                y[x] = 1150
    elif sec_id == 6: # Basalt Needle Spires: Sharp, saw-toothed 60-degree needles
        for x in range(w):
            nx = (x % 320) / 320.0
            needle = (1.0 - abs(nx - 0.5) * 2.0) ** 0.5 * 180
            y[x] = 780 - needle
    elif sec_id == 7: # Subterranean Lava Tube: Tunnel roof and subterranean road
        for x in range(w):
            nx = x / float(w)
            y[x] = 720 - 70 * math.sin(nx * 4 * math.pi)
    elif sec_id == 8: # Micro-Meteorite Crater Field: Jagged, pockmarked mini-craters
        for x in range(w):
            nx = x / float(w)
            pits = (math.sin(nx * 26 * math.pi) + math.sin(nx * 14 * math.pi)) * 35
            y[x] = 720 - pits
    elif sec_id == 9: # Grand Lunar Trench Steps: 3 massive descending step terraces
        for x in range(w):
            nx = x / float(w)
            step = int(nx * 3)
            y[x] = 520 + step * 140 - 20 * math.sin(nx * 12 * math.pi)
    else: # sec_id == 10: Spaceport Orbital Catapult: Huge curved launch ramp
        for x in range(w):
            nx = x / float(w)
            if nx < 0.7:
                y[x] = 840 - (nx / 0.7) ** 2 * 460
            elif nx < 0.85:
                y[x] = 1150
            else:
                y[x] = 460
    return np.clip(y, 160, 1150)


def get_swamp_elevation(sec_id, w=1920):
    y = np.zeros(w, dtype=float)
    if sec_id == 1: # Peat Moss Shallows: Sinking marsh with shallow puddles
        for x in range(w):
            nx = x / float(w)
            y[x] = 740 + 35 * math.sin(nx * 6 * math.pi)
    elif sec_id == 2: # Black Muck Wallows: 2 deep, muddy bog depressions
        for x in range(w):
            nx = x / float(w)
            mud1 = 140 * math.exp(-((nx - 0.35) / 0.12) ** 2)
            mud2 = 160 * math.exp(-((nx - 0.72) / 0.14) ** 2)
            y[x] = 700 + mud1 + mud2
    elif sec_id == 3: # Rotting Rope Suspension Bridge: 420px murky bayou channel
        for x in range(w):
            if x < 720:
                y[x] = 680 - 40 * math.sin(x * 0.007)
            elif x > 1200:
                y[x] = 700 - 35 * math.sin(x * 0.007)
            else:
                y[x] = 1150 # deep muck canal
    elif sec_id == 4: # Giant Cypress Root Tangles: Knotted natural root mounds
        for x in range(w):
            nx = (x % 380) / 380.0
            root = math.sin(nx * math.pi) ** 3 * 160
            y[x] = 760 - root
    elif sec_id == 5: # Alligator Slough Deep Water: 460px flooded slough
        for x in range(w):
            nx = x / float(w)
            if 0.32 <= nx <= 0.68:
                t = (nx - 0.32) / 0.36
                y[x] = 720 + math.sin(t * math.pi) * 160 # deep water canal
            else:
                y[x] = 720 - 35 * math.sin(nx * 5 * math.pi)
    elif sec_id == 6: # Slimy Sludge Terraces: Organic terraced mounds
        for x in range(w):
            nx = x / float(w)
            mound = math.sin(nx * 8 * math.pi) * 50 + math.cos(nx * 3 * math.pi) * 40
            y[x] = 740 - mound
    elif sec_id == 7: # Sunken Timber Stepping Logs: Wide canal with stepping logs
        for x in range(w):
            if 650 <= x <= 1280:
                y[x] = 1150
            else:
                y[x] = 720 - 45 * math.sin(x * 0.006)
    elif sec_id == 8: # Acidic Quagmire Abyss: Pit filled with toxic sludge
        for x in range(w):
            if 680 <= x <= 1240:
                y[x] = 1150
            else:
                y[x] = 660 - 40 * math.sin(x * 0.007)
    elif sec_id == 9: # Triple Ravine Bog Hop: 3 separate murky chasms
        for x in range(w):
            if (500 <= x <= 700) or (900 <= x <= 1100) or (1300 <= x <= 1500):
                y[x] = 1150
            else:
                y[x] = 700 - 45 * math.sin((x / float(w)) * 7 * math.pi)
    else: # sec_id == 10: Ancient Willow Canopy Crest: Steep root climb to elevated ridge
        for x in range(w):
            nx = x / float(w)
            y[x] = 840 - (nx ** 0.8) * 440
    return np.clip(y, 160, 1150)

# ==============================================================================
# RICH VISUAL RENDERING FOR EACH THEME
# ==============================================================================

def draw_snow_features(draw, sec_id, y_profile, w=1920, h=1080):
    # Snow surface decoration & landmarks
    if sec_id == 1:
        # Ski slalom flags & avalanche fences
        for fx in [350, 650, 1050, 1450]:
            fy = int(y_profile[fx])
            draw.line([(fx, fy), (fx, fy - 65)], fill=(220, 220, 220), width=4)
            draw.polygon([(fx, fy - 65), (fx + 35, fy - 50), (fx, fy - 35)], fill=(240, 40, 40))
        # Avalanche barrier fences
        draw.line([(1200, int(y_profile[1200]) - 30), (1400, int(y_profile[1400]) - 30)], fill=(120, 80, 50), width=5)
    elif sec_id == 2:
        # Serac ice pillars & blue ice spikes
        for sx in [420, 880, 1340]:
            sy = int(y_profile[sx])
            draw.polygon([(sx - 20, sy), (sx, sy - 85), (sx + 20, sy)], fill=(170, 235, 255), outline=(100, 195, 245), width=2)
    elif sec_id == 3:
        # Fragile Ice Crevasse Bridge (750 to 1180)
        by = int(min(y_profile[740], y_profile[1190]))
        # Translucent ice shelf
        draw.rectangle([750, by - 8, 1180, by + 16], fill=(160, 235, 255, 240), outline=(90, 190, 245, 255), width=3)
        # Deep fracture cracks
        for cx in range(780, 1150, 40):
            draw.line([(cx, by - 6), (cx + 12, by + 14)], fill=(255, 255, 255), width=2)
            draw.line([(cx + 12, by + 14), (cx + 25, by + 4)], fill=(210, 245, 255), width=2)
        # Stalactite icicles hanging into abyss
        for ix in range(760, 1170, 18):
            ilen = np.random.randint(20, 60)
            draw.polygon([(ix - 4, by + 16), (ix + 4, by + 16), (ix, by + 16 + ilen)], fill=(200, 245, 255))
        # Emergency beacon
        draw.rectangle([710, by - 55, 735, by], fill=(220, 40, 40))
        draw.text((718, by - 50), "!", fill=(255, 255, 255), font=FONT_HEADING)
        draw.text((790, by - 55), "FRAGILE ICE: COLLAPSES ON IMPACT OR LOAD > 1.5s", fill=(210, 245, 255), font=FONT_TEXT)
    elif sec_id == 4:
        # Avalanche warning & rolling snow boulders
        draw.ellipse([850, int(y_profile[850]) - 50, 910, int(y_profile[850]) + 10], fill=(230, 240, 255), outline=(140, 180, 220), width=3)
        draw.text((450, 480), "AVALANCHE CHUTE: 55 DEGREE DOWNHILL SLIP", fill=(255, 180, 180), font=FONT_HEADING)
    elif sec_id == 5:
        # Timber Trestle Bridge (740 to 1200)
        by = int(y_profile[730])
        draw.rectangle([740, by - 6, 1200, by + 12], fill=(125, 90, 55), outline=(65, 45, 25), width=2)
        for tx in range(740, 1200, 16):
            draw.line([(tx, by - 6), (tx, by + 12)], fill=(65, 45, 25), width=2)
        # Broken gap in middle
        draw.rectangle([940, by - 8, 985, by + 14], fill=(15, 25, 60))
        # Support struts
        draw.line([(740, by + 8), (900, by + 140)], fill=(95, 70, 40), width=6)
        draw.line([(1200, by + 8), (1040, by + 140)], fill=(95, 70, 40), width=6)
        draw.text((780, by - 50), "BREAKABLE TIMBER TRESTLE: LANDING SHATTERS DECK", fill=(255, 210, 140), font=FONT_TEXT)
    elif sec_id == 6:
        # Cozy alpine refuge cabin at x=1500
        cx, cy = 1500, int(y_profile[1500])
        draw.rectangle([cx - 45, cy - 65, cx + 45, cy], fill=(110, 75, 45), outline=(50, 35, 20), width=3)
        draw.polygon([(cx - 55, cy - 65), (cx, cy - 110), (cx + 55, cy - 65)], fill=(240, 245, 255), outline=(160, 200, 230), width=3)
        draw.rectangle([cx - 15, cy - 45, cx + 15, cy - 20], fill=(255, 220, 100)) # glowing window
    elif sec_id == 7:
        # Frozen waterfall vertical wall
        draw.text((800, 520), "VERTICAL FROZEN WATERFALL: SPEED BOOST REQUIRED", fill=(180, 240, 255), font=FONT_HEADING)
        for iy in range(380, 800, 40):
            draw.line([(900, iy), (940, iy + 30)], fill=(210, 245, 255), width=4)
    elif sec_id == 8:
        # Two-tier drop with middle fragile ice shelf (800 to 1250)
        by = int(y_profile[950])
        draw.rectangle([800, by - 8, 1250, by + 14], fill=(160, 230, 255, 230), outline=(90, 190, 245), width=3)
        draw.text((820, by - 50), "CRASH HAZARD: HIGH FALL BREAKS THROUGH SHELF", fill=(255, 120, 120), font=FONT_HEADING)
    elif sec_id == 9:
        # 3 Crevasse Bridges
        for lx, rx in [(500, 700), (900, 1100), (1300, 1500)]:
            by = int(min(y_profile[lx - 10], y_profile[rx + 10]))
            draw.rectangle([lx, by - 6, rx, by + 10], fill=(170, 235, 255), outline=(90, 190, 245), width=2)
    elif sec_id == 10:
        # Summit Leap & Checkered Finish
        draw.rectangle([1600, 380, 1680, 440], fill=(255, 255, 255))
        for fx in range(1600, 1680, 20):
            for fy in range(380, 440, 20):
                if (fx // 20 + fy // 20) % 2 == 0:
                    draw.rectangle([fx, fy, fx + 20, fy + 20], fill=(0, 0, 0))
        draw.text((1580, 340), "FINISH 10,000m", fill=(255, 255, 255), font=FONT_HEADING)


def draw_beach_features(draw, sec_id, y_profile, w=1920, h=1080):
    if sec_id == 1:
        # Beach umbrellas, surfboards, tiki torches
        for bx in [320, 750, 1250, 1650]:
            by = int(y_profile[bx])
            # Umbrella
            draw.line([(bx, by), (bx, by - 90)], fill=(180, 140, 80), width=4)
            draw.pieslice([bx - 60, by - 120, bx + 60, by - 60], 180, 360, fill=(240, 60, 60), outline=(255, 255, 255), width=2)
            # Surfboard
            draw.ellipse([bx + 40, by - 60, bx + 55, by], fill=(40, 180, 240))
    elif sec_id == 2:
        # Barchan dune windward & slip face
        draw.text((620, 620), "BARCHAN DUNES: POWDERY SAND & STEEP SLIP-FACE", fill=(255, 220, 140), font=FONT_HEADING)
    elif sec_id == 3:
        # Broken Pier Boardwalk (720 to 1220)
        by = int(min(y_profile[710], y_profile[1230]))
        # Ocean water underneath
        draw.rectangle([720, 860, 1220, 1080], fill=(20, 145, 220))
        draw.text((820, 930), "~ PACIFIC TIDAL SURGE ~", fill=(200, 240, 255), font=FONT_HEADING)
        # Pier Deck
        draw.rectangle([720, by - 8, 1220, by + 12], fill=(160, 125, 80), outline=(95, 70, 45), width=2)
        for px in range(720, 1220, 18):
            draw.line([(px, by - 8), (px, by + 12)], fill=(95, 70, 45), width=2)
        # Missing planks gap
        draw.rectangle([930, by - 10, 970, by + 14], fill=(50, 150, 245))
        # Pilings
        for px in range(750, 1200, 100):
            draw.rectangle([px - 6, by + 12, px + 6, 860], fill=(120, 90, 55))
        draw.text((760, by - 50), "RICKETY BOARDWALK: LOOSE PLANKS OVER SURF", fill=(255, 220, 140), font=FONT_TEXT)
    elif sec_id == 4:
        # Quicksand Pit
        draw.rectangle([680, 720, 1250, 850], fill=(215, 170, 100), outline=(150, 105, 50), width=3)
        draw.text((750, 760), "! DANGER: SINKING QUICKSAND PIT !", fill=(180, 40, 40), font=FONT_HEADING)
        draw.text((800, 795), "CONTINUOUS THROTTLE REQUIRED", fill=(255, 245, 210), font=FONT_TEXT)
    elif sec_id == 5:
        # Coral Sea Arch Tunnel
        draw.text((680, 560), "LIMESTONE SEA ARCH: CAVE TUNNEL CROSSING", fill=(255, 215, 140), font=FONT_HEADING)
        draw.arc([650, 520, 1280, 950], 180, 360, fill=(180, 140, 90), width=18)
    elif sec_id == 6:
        # Tidal Lagoon Water Dip
        draw.rectangle([580, 760, 1340, 880], fill=(30, 160, 230, 200))
        draw.text((720, 800), "~ TIDAL LAGOON SPLASH BASIN ~", fill=(220, 250, 255), font=FONT_HEADING)
    elif sec_id == 7:
        draw.text((680, 520), "RAZOR DUNE RIDGE: TIP-OVER ROLL HAZARD", fill=(255, 200, 120), font=FONT_HEADING)
    elif sec_id == 8:
        # Galleon Shipwreck Bridge (700 to 1220)
        by = int(y_profile[700])
        # Wooden hull bridging the cove
        draw.polygon([(700, by), (960, by + 80), (1220, by), (1150, by + 120), (750, by + 120)], fill=(90, 65, 40), outline=(50, 35, 20), width=3)
        # Ship mast
        draw.line([(960, by + 80), (960, by - 120)], fill=(120, 85, 55), width=6)
        draw.text((780, by - 50), "PIRATE SHIPWRECK COVE: GALLEON HULL CROSSING", fill=(255, 220, 130), font=FONT_TEXT)
    elif sec_id == 9:
        # 3 Dune Jumps over tidal channels
        for lx, rx in [(520, 720), (950, 1150), (1380, 1580)]:
            draw.rectangle([lx, 860, rx, 1080], fill=(20, 140, 215))
        draw.text((700, 620), "TRIPLE DUNE LAUNCH JUMPS OVER TIDAL CHANNELS", fill=(255, 220, 140), font=FONT_HEADING)
    elif sec_id == 10:
        # Red and white lighthouse at x=1550
        lx, ly = 1550, int(y_profile[1550])
        draw.polygon([(lx - 30, ly), (lx - 15, ly - 160), (lx + 15, ly - 160), (lx + 30, ly)], fill=(245, 245, 245), outline=(180, 30, 30), width=4)
        draw.rectangle([lx - 25, ly - 100, lx + 25, ly - 70], fill=(220, 40, 40)) # red stripe
        draw.rectangle([lx - 18, ly - 180, lx + 18, ly - 160], fill=(255, 230, 100)) # lantern room
        draw.text((1440, ly - 210), "LIGHTHOUSE FINISH", fill=(255, 255, 255), font=FONT_HEADING)


def draw_moon_features(draw, sec_id, y_profile, w=1920, h=1080):
    if sec_id == 1:
        # Apollo Rover tracks, American/Artemis flag
        fx, fy = 800, int(y_profile[800])
        draw.line([(fx, fy), (fx, fy - 75)], fill=(220, 225, 235), width=4)
        draw.rectangle([fx, fy - 75, fx + 50, fy - 45], fill=(230, 50, 50))
        draw.rectangle([fx, fy - 75, fx + 25, fy - 60], fill=(40, 70, 180))
        # Lunar Lander Descent Stage at x=1350
        lx, ly = 1350, int(y_profile[1350])
        draw.polygon([(lx - 40, ly), (lx - 30, ly - 45), (lx + 30, ly - 45), (lx + 40, ly)], fill=(215, 175, 45))
        draw.text((620, 580), "APOLLO LANDING SITE: 1/6th LOW GRAVITY INTRO", fill=(180, 220, 255), font=FONT_HEADING)
    elif sec_id == 2:
        # Tycho Impact Basin
        draw.text((660, 480), "TYCHO CRATER BASIN: REBOUND PEAK LAUNCH", fill=(190, 230, 255), font=FONT_HEADING)
        draw.arc([600, 300, 1400, 750], 210, 330, fill=(80, 190, 255), width=3)
    elif sec_id == 3:
        # Levitating Basalt Causeway (700 to 1240)
        by = int(y_profile[680])
        for idx, sx in enumerate(range(740, 1200, 115)):
            sy = by + int(15 * math.sin(idx * 2))
            draw.polygon([(sx, sy), (sx + 85, sy), (sx + 75, sy + 25), (sx - 10, sy + 25)],
                         fill=(140, 145, 160), outline=(220, 230, 245), width=2)
            # Blue magnetic levitation field
            draw.ellipse([sx - 5, sy + 25, sx + 80, sy + 40], fill=(60, 180, 255))
        draw.text((720, by - 50), "MAGNETIC LEVITATING BASALT SLABS OVER VACUUM ABYSS", fill=(160, 230, 255), font=FONT_TEXT)
    elif sec_id == 4:
        # Solar Collector Array Towers
        for px in [380, 850, 1320, 1700]:
            py = int(y_profile[px])
            draw.line([(px, py), (px, py - 90)], fill=(160, 170, 185), width=5)
            draw.rectangle([px - 45, py - 130, px + 45, py - 90], fill=(30, 80, 160), outline=(180, 210, 255), width=2)
        draw.text((720, 540), "SOLAR ARRAY CAUSEWAY: PRECISION NARROW ROAD", fill=(180, 225, 255), font=FONT_HEADING)
    elif sec_id == 5:
        # 520px Super-Hangtime Chasm Leap
        draw.text((680, 460), "THE GREAT IMPACT CHASM: 520px SUPER-GLIDE LEAP", fill=(255, 210, 120), font=FONT_HEADING)
        draw.arc([650, 350, 1350, 780], 200, 340, fill=(110, 210, 255), width=4)
    elif sec_id == 6:
        # Basalt Needle Spires
        draw.text((680, 520), "BASALT NEEDLE SPIRES: TIP-TO-TIP BOUNCES", fill=(190, 225, 255), font=FONT_HEADING)
    elif sec_id == 7:
        # Subterranean Lava Tube
        draw.text((680, 580), "COLLAPSED LAVA TUBE: SUBTERRANEAN BASALT ROAD", fill=(170, 230, 255), font=FONT_HEADING)
    elif sec_id == 8:
        # Meteorite Swarm Field
        draw.text((660, 580), "METEORITE SWARM FIELD: SUSPENSION PUNISHER", fill=(255, 180, 180), font=FONT_HEADING)
    elif sec_id == 9:
        # Step-down Crater Trench
        draw.text((680, 440), "GRAND LUNAR TRENCH: STEP-DOWN TERRACES", fill=(180, 220, 255), font=FONT_HEADING)
    elif sec_id == 10:
        # Orbital Spaceport Catapult & Finish Gantry
        draw.text((1420, 360), "ORBITAL SPACEPORT FINISH", fill=(255, 255, 255), font=FONT_HEADING)
        # Radio dish at x=1650
        rx, ry = 1650, int(y_profile[1650])
        draw.line([(rx, ry), (rx, ry - 60)], fill=(180, 190, 205), width=5)
        draw.arc([rx - 45, ry - 110, rx + 45, ry - 50], 180, 360, fill=(230, 240, 255), width=4)


def draw_swamp_features(draw, sec_id, y_profile, w=1920, h=1080):
    if sec_id == 1:
        # Dilapidated wooden dock, swamp lanterns
        for lx in [380, 850, 1350]:
            ly = int(y_profile[lx])
            draw.line([(lx, ly), (lx, ly - 70)], fill=(90, 65, 40), width=4)
            draw.ellipse([lx - 12, ly - 85, lx + 12, ly - 65], fill=(245, 210, 80)) # lantern glow
        draw.text((620, 620), "PEAT MOSS SHALLOWS: HIGH MUD DRAG & TRACTION LOSS", fill=(190, 235, 110), font=FONT_HEADING)
    elif sec_id == 2:
        # Deep Black Muck Wallows
        draw.rectangle([550, 720, 1450, 860], fill=(35, 24, 15), outline=(18, 12, 7), width=3)
        draw.text((680, 755), "DEEP BUBBLING MUD WALLOW: BOOST ESSENTIAL", fill=(235, 180, 100), font=FONT_HEADING)
        draw.text((760, 790), "4x ROLLING DRAG - AVOID STOPPING", fill=(255, 230, 180), font=FONT_TEXT)
    elif sec_id == 3:
        # Rotting Rope Suspension Bridge (720 to 1200)
        by = int(min(y_profile[710], y_profile[1210]))
        # Murky muck canal
        draw.rectangle([720, 860, 1200, 1080], fill=(28, 38, 18))
        draw.text((820, 930), "DEEP QUAGMIRE SLUDGE", fill=(130, 180, 65), font=FONT_HEADING)
        # Sagging rope cable
        c_pts = []
        for bx in range(720, 1201, 12):
            st = (bx - 720) / float(1200 - 720)
            sag = 50 * math.sin(st * math.pi)
            c_pts.append((bx, by + sag))
        draw.line(c_pts, fill=(95, 75, 45), width=6)
        for bx, cy in c_pts[::3]:
            draw.rectangle([bx - 6, cy - 4, bx + 6, cy + 6], fill=(110, 85, 55), outline=(50, 38, 24))
        draw.text((750, by - 50), "ROTTING ROPE BRIDGE: PLANKS SNAP UNDER HEAVY IMPACT", fill=(210, 240, 120), font=FONT_TEXT)
    elif sec_id == 4:
        # Cypress Root Cage Tangles
        draw.text((640, 540), "CYPRESS ROOT ARCHE TANGLES: SUDDEN BUMPS & RAMPS", fill=(190, 230, 100), font=FONT_HEADING)
    elif sec_id == 5:
        # Alligator Slough Deep Water (620 to 1300)
        draw.rectangle([620, 740, 1300, 880], fill=(35, 55, 25))
        draw.text((720, 780), "~ ALLIGATOR BAYOU SLOUGH: DEEP WATER ~", fill=(160, 220, 90), font=FONT_HEADING)
        draw.text((780, 815), "ENGINE DROWNING & WATER RESISTANCE HAZARD", fill=(220, 240, 180), font=FONT_TEXT)
    elif sec_id == 6:
        draw.text((680, 580), "SLIMY SLUDGE TERRACES: MOSS SLIP HAZARD", fill=(185, 230, 95), font=FONT_HEADING)
    elif sec_id == 7:
        # Sunken Floating Stepping Logs
        by = int(y_profile[640])
        draw.rectangle([650, 820, 1280, 1080], fill=(25, 38, 18))
        # 4 Floating logs
        for lx in range(700, 1220, 130):
            draw.ellipse([lx, by - 10, lx + 90, by + 20], fill=(90, 68, 42), outline=(50, 38, 22), width=3)
        draw.text((740, by - 50), "SUNKEN TIMBER TRAIL: FLOATING LOGS SINK UNDER LOAD", fill=(210, 240, 110), font=FONT_TEXT)
    elif sec_id == 8:
        # Acidic Quagmire Abyss
        draw.rectangle([680, 820, 1240, 1080], fill=(45, 65, 20))
        draw.text((740, 860), "TOXIC QUAGMIRE ABYSS", fill=(175, 245, 60), font=FONT_HEADING)
        draw.text((710, 560), "SAG VINE BRIDGE: DECAYING CORROSIVE VAPORS", fill=(200, 245, 90), font=FONT_TEXT)
    elif sec_id == 9:
        # Triple Ravine Bog Hop
        for lx, rx in [(500, 700), (900, 1100), (1300, 1500)]:
            draw.rectangle([lx, 840, rx, 1080], fill=(25, 38, 18))
        draw.text((680, 580), "TRIPLE RAVINE BOG HOP: CONSECUTIVE ROPE SPANS", fill=(185, 235, 95), font=FONT_HEADING)
    elif sec_id == 10:
        # Ancient Willow Canopy Crest & Finish Line
        draw.text((1440, 360), "WILLOW RIDGE FINISH", fill=(235, 255, 180), font=FONT_HEADING)
        # Giant willow tree with hanging moss
        wx, wy = 1620, int(y_profile[1620])
        draw.polygon([(wx - 35, wy), (wx - 15, wy - 180), (wx + 15, wy - 180), (wx + 35, wy)], fill=(55, 42, 28))
        for mx in range(wx - 90, wx + 90, 25):
            draw.line([(mx, wy - 180), (mx + np.random.randint(-10, 10), wy - 60)], fill=(95, 125, 65), width=4)

# ==============================================================================
# MASTER GENERATOR FUNCTION
# ==============================================================================

def generate_all_diverse_assets():
    themes = {
        "snow": {
            "title": "FROSTBITE SUMMIT", "subtitle": "10 KM GLACIAL GAUNTLET",
            "gravity": "9.81 m/s² (EARTH)", "friction": "0.12 - 0.25 (EXTREME SLIP)", "drag": "1.00x",
            "hazards": "Fragile Ice, Breaking Trestles, 55° Avalanche Chutes, Glacial Chasms",
            "primary": (160, 220, 255), "accent": (40, 180, 240),
            "sky_col": (15, 25, 60), "ground_col": (65, 85, 115), "surf_col": (245, 250, 255),
            "fn_elev": get_snow_elevation, "fn_feat": draw_snow_features,
            "sec_names": [
                "GLACIER TONGUE RUNOUT", "SERAC ICE MAZE", "THE ABYSSAL CREVASSE", "AVALANCHE SLIDE & CRASH",
                "TIMBER TRESTLE RAVINE", "BLIZZARD PASS SWITCHBACKS", "FROZEN WATERFALL WALL",
                "FRAGILE ICE SHELF DROP", "FIVE-CREVASSE GAUNTLET", "HORN PEAK SUMMIT LEAP"
            ]
        },
        "beach": {
            "title": "SUNKEN DUNES & CORAL COAST", "subtitle": "10 KM COASTAL EXPEDITION",
            "gravity": "9.81 m/s² (EARTH)", "friction": "0.45 - 0.70 (SAND DRAG)", "drag": "1.00x",
            "hazards": "Powdery Dune Slip-Faces, Quicksand Pits, Rickety Boardwalks, Tidal Pools",
            "primary": (255, 215, 120), "accent": (250, 140, 30),
            "sky_col": (50, 150, 245), "ground_col": (150, 115, 65), "surf_col": (245, 215, 150),
            "fn_elev": get_beach_elevation, "fn_feat": draw_beach_features,
            "sec_names": [
                "LOW TIDE STRAND", "BARCHAN DUNE CRESTS", "BROKEN PIER BOARDWALK", "SINKING QUICKSAND PIT",
                "CORAL SEA ARCH & CAVES", "TIDAL LAGOON SPLASH BASIN", "RAZOR SAND RIDGE",
                "SHIPWRECK GALLEON BRIDGE", "THREE LAGOON DUNE JUMPS", "CLIFTOP LIGHTHOUSE HEADLAND"
            ]
        },
        "moon": {
            "title": "MARE TRANQUILLITATIS CRATERLANDS", "subtitle": "10 KM LUNAR VACUUM TRAIL",
            "gravity": "1.62 m/s² (1/6th G)", "friction": "0.85 (FINE REGOLITH)", "drag": "0.00x (VACUUM)",
            "hazards": "Extreme Airborne Float, Pitch Inversion, Impact Basins, Floating Basalt",
            "primary": (210, 225, 255), "accent": (110, 190, 255),
            "sky_col": (5, 8, 16), "ground_col": (65, 70, 85), "surf_col": (215, 220, 230),
            "fn_elev": get_moon_elevation, "fn_feat": draw_moon_features,
            "sec_names": [
                "MARE TRANQUILLITATIS PLAINS", "TYCHO CRATER BASIN BOUNCE", "LEVITATING BASALT CAUSEWAY",
                "SOLAR ARRAY NARROW ROAD", "THE GREAT IMPACT CHASM", "BASALT NEEDLE SPIRES",
                "COLLAPSED LAVA TUBE", "METEORITE SWARM FIELD", "GRAND LUNAR TRENCH STEPS",
                "SPACEPORT ORBITAL CATAPULT"
            ]
        },
        "swamp": {
            "title": "BILEWATER BOG & BAYOU", "subtitle": "10 KM MURKY QUAGMIRE",
            "gravity": "9.81 m/s² (EARTH)", "friction": "0.30 - 0.50 (VISCOUS MUD)", "drag": "1.15x (FOG)",
            "hazards": "Viscous Black Mud, Snapping Rope Bridges, Cypress Root Cages, Bayou Water",
            "primary": (180, 235, 90), "accent": (115, 195, 45),
            "sky_col": (35, 48, 30), "ground_col": (38, 26, 15), "surf_col": (95, 135, 45),
            "fn_elev": get_swamp_elevation, "fn_feat": draw_swamp_features,
            "sec_names": [
                "PEAT MOSS SHALLOWS", "BLACK MUCK WALLOWS", "ROTTING ROPE SUSPENSION BRIDGE",
                "CYPRESS ROOT TANGLES", "ALLIGATOR BAYOU SLOUGH", "SLIMY SLUDGE TERRACES",
                "SUNKEN TIMBER STEPS", "ACIDIC QUAGMIRE ABYSS", "TRIPLE RAVINE BOG HOP",
                "ANCIENT WILLOW CANOPY CREST"
            ]
        }
    }

    w, h = 1920, 1080

    for theme_name, data in themes.items():
        print(f"\n=======================================================")
        print(f"GENERATING HIGH-VARIETY MAP: {data['title']}")
        print(f"=======================================================")

        # Load vehicle sprites
        car_p = os.path.join(TEMP_DIR, theme_name, "vehicle", "car_body.png")
        wheel_p = os.path.join(TEMP_DIR, theme_name, "vehicle", "wheel.png")
        car_sprite = Image.open(car_p).convert("RGBA") if os.path.exists(car_p) else None
        wheel_sprite = Image.open(wheel_p).convert("RGBA") if os.path.exists(wheel_p) else None

        sec_out = os.path.join(TEMP_DIR, theme_name, "sections")
        over_out = os.path.join(TEMP_DIR, theme_name, "overview")
        os.makedirs(sec_out, exist_ok=True)
        os.makedirs(over_out, exist_ok=True)

        # -------------------------------------------------------------
        # 1. GENERATE 10 DIVERSE SECTION IMAGES
        # -------------------------------------------------------------
        for sec_id in range(1, 11):
            sec_name = data["sec_names"][sec_id - 1]
            img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
            draw = ImageDraw.Draw(img)

            # Background Gradient
            bg_arr = np.zeros((h, w, 4), dtype=np.uint8)
            sr, sg, sb = data["sky_col"]
            for y in range(h):
                t = y / float(h)
                bg_arr[y, :, 0] = int(np.clip(sr + 80 * t, 0, 255))
                bg_arr[y, :, 1] = int(np.clip(sg + 90 * t, 0, 255))
                bg_arr[y, :, 2] = int(np.clip(sb + 110 * t, 0, 255))
                bg_arr[y, :, 3] = 255
            img = Image.fromarray(bg_arr, "RGBA")
            draw = ImageDraw.Draw(img)

            # Moon/Earth/Sun/Mist backdrop
            if theme_name == "snow":
                draw.ellipse([1550, 120, 1620, 190], fill=(240, 248, 255))
                draw.ellipse([1570, 110, 1640, 180], fill=data["sky_col"] + (255,))
            elif theme_name == "beach":
                draw.ellipse([1450, 120, 1550, 220], fill=(255, 255, 230))
            elif theme_name == "moon":
                draw.ellipse([1420, 140, 1520, 240], fill=(35, 115, 215))
                draw.ellipse([1440, 160, 1490, 200], fill=(60, 155, 80))
            elif theme_name == "swamp":
                draw.ellipse([450, 160, 560, 270], fill=(210, 220, 170, 180))

            # Ground polyline
            y_profile = data["fn_elev"](sec_id, w)

            # Render Ground Strata
            g_deep = data["ground_col"]
            g_surf = data["surf_col"]

            for x in range(w):
                ty = int(y_profile[x])
                if ty >= h:
                    continue
                # 15px surface layer
                draw.line([(x, ty), (x, min(h, ty + 16))], fill=g_surf + (255,), width=1)
                # deep bedrock layer
                draw.line([(x, ty + 16), (x, h)], fill=g_deep + (255,), width=1)

            # Draw unique in-track landmark features
            data["fn_feat"](draw, sec_id, y_profile, w, h)

            # Composite Themed Vehicle
            if car_sprite and wheel_sprite:
                c_scale = 0.16
                cw = int(car_sprite.width * c_scale)
                ch = int(car_sprite.height * c_scale)
                car_small = car_sprite.resize((cw, ch), Image.Resampling.LANCZOS)
                w_sz = int(wheel_sprite.width * c_scale * 0.45)
                wheel_small = wheel_sprite.resize((w_sz, w_sz), Image.Resampling.LANCZOS)

                vx = 460
                vy = int(y_profile[vx]) - ch + 10
                if 120 < vy < h - 160:
                    img.alpha_composite(wheel_small, (vx + 32, vy + ch - w_sz // 2 - 4))
                    img.alpha_composite(wheel_small, (vx + cw - 75, vy + ch - w_sz // 2 - 4))
                    img.alpha_composite(car_small, (vx, vy))

            # HUD Header Card
            draw.rectangle([40, 35, 750, 145], fill=(12, 18, 28, 235), outline=data["primary"] + (255,), width=2)
            draw.text((55, 45), f"{data['title']} [10KM]", fill=data["primary"], font=FONT_HEADING)
            draw.text((55, 75), f"ZONE {sec_id:02d}: {sec_name}", fill=(255, 255, 255), font=FONT_TITLE)
            draw.text((55, 114), f"RANGE: {(sec_id-1)*1000}m - {sec_id*1000}m | DIFFICULTY: {'★' * min(5, (sec_id+1)//2)}{'☆' * (5 - min(5, (sec_id+1)//2))}",
                      fill=data["accent"], font=FONT_TEXT)

            # Telemetry Spec Box
            draw.rectangle([1180, 35, 1880, 160], fill=(12, 18, 28, 235), outline=data["primary"] + (255,), width=2)
            draw.text((1195, 45), "ENVIRONMENT TELEMETRY", fill=data["primary"], font=FONT_HEADING)
            draw.text((1195, 75), f"GRAVITY: {data['gravity']}", fill=(240, 245, 255), font=FONT_TEXT)
            draw.text((1195, 98), f"FRICTION: {data['friction']}", fill=(240, 245, 255), font=FONT_TEXT)
            draw.text((1195, 122), f"HAZARDS: {data['hazards']}", fill=(255, 185, 80), font=FONT_SMALL)

            # 10-Segment Progress Bar
            bx, by, bw, bh = 55, 1020, 1810, 18
            draw.rectangle([bx, by, bx + bw, by + bh], fill=(25, 35, 50, 255))
            sw = bw / 10.0
            for si in range(10):
                sx = bx + si * sw
                if si + 1 == sec_id:
                    draw.rectangle([sx + 2, by + 2, sx + sw - 2, by + bh - 2], fill=data["accent"] + (255,))
                elif si + 1 < sec_id:
                    draw.rectangle([sx + 2, by + 2, sx + sw - 2, by + bh - 2], fill=(70, 95, 130, 255))
                else:
                    draw.rectangle([sx + 2, by + 2, sx + sw - 2, by + bh - 2], fill=(40, 50, 70, 255))

            f_name = f"section_{sec_id:02d}.png"
            img.save(os.path.join(sec_out, f_name), optimize=True)
            print(f"  [+] Saved {f_name}: {sec_name}")

        # -------------------------------------------------------------
        # 2. GENERATE FULL 10KM OVERVIEW MAP (3840x1080)
        # -------------------------------------------------------------
        print(f"  Generating full 10km overview map...")
        ow, oh = 3840, 1080
        o_img = Image.new("RGBA", (ow, oh), (10, 14, 22, 255))
        o_draw = ImageDraw.Draw(o_img)

        # Technical Grid
        for gx in range(0, ow, 80):
            o_draw.line([(gx, 0), (gx, oh)], fill=(20, 28, 42, 255), width=1)
        for gy in range(0, oh, 60):
            o_draw.line([(0, gy), (ow, gy)], fill=(20, 28, 42, 255), width=1)

        # Header Card
        o_draw.rectangle([60, 40, ow - 60, 180], fill=(15, 24, 38, 250), outline=data["primary"] + (255,), width=3)
        o_draw.text((90, 55), f"{data['title']} — COMPLETE 10,000 METRE ELEVATION PROFILE", fill=data["primary"], font=FONT_TITLE)
        o_draw.text((90, 95), f"{data['subtitle']} | 10 DIVERSE SECTORS | BESPOKE TOPOGRAPHY", fill=data["accent"], font=FONT_HEADING)
        o_draw.text((90, 135), f"GRAVITY: {data['gravity']}   |   FRICTION: {data['friction']}   |   DRAG: {data['drag']}   |   HAZARDS: {data['hazards']}",
                    fill=(230, 240, 255), font=FONT_TEXT)

        # Construct continuous 10km elevation profile from the 10 distinct functions
        gx_start = 120
        gx_end = ow - 120
        gw = gx_end - gx_start
        gy_base = 740

        pts_10k = []
        sec_w = gw / 10.0

        for si in range(10):
            sec_id = si + 1
            sec_pts_count = int(sec_w)
            # Sample from the bespoke elevation curve
            sec_elev = data["fn_elev"](sec_id, 1920)
            for pi in range(sec_pts_count):
                samp_idx = int((pi / float(sec_pts_count)) * 1919)
                y_val = sec_elev[samp_idx]
                # Scale elevation to overview graph
                scaled_y = 280 + (y_val / 1150.0) * 440
                cur_x = gx_start + int(si * sec_w + pi)
                pts_10k.append((cur_x, scaled_y))

        # Fill profile ground
        poly = [(gx_start, gy_base + 60)] + pts_10k + [(gx_end, gy_base + 60)]
        o_draw.polygon(poly, fill=data["ground_col"] + (180,))
        o_draw.line(pts_10k, fill=data["primary"] + (255,), width=4)

        # 10 Section Divide Columns & Cards
        for si in range(10):
            sec_id = si + 1
            sx = gx_start + si * sec_w
            sec_name = data["sec_names"][si]

            # Demarcation line
            o_draw.line([(sx, 220), (sx, gy_base + 70)], fill=(60, 85, 120, 180), width=2)

            # Section Card Box
            card_y = 800
            o_draw.rectangle([sx + 6, card_y, sx + sec_w - 6, card_y + 220],
                             fill=(16, 24, 36, 240), outline=data["accent"] + (200,), width=2)
            o_draw.text((sx + 14, card_y + 12), f"SEC {sec_id:02d}", fill=data["primary"], font=FONT_TITLE)
            o_draw.text((sx + 14, card_y + 48), sec_name, fill=(255, 255, 255), font=FONT_TEXT)
            o_draw.text((sx + 14, card_y + 78), f"RANGE: {si*1000}m - {(si+1)*1000}m", fill=data["accent"], font=FONT_SMALL)
            o_draw.text((sx + 14, card_y + 102), f"DIFF: {'★' * min(5, (sec_id+1)//2)}{'☆' * (5 - min(5, (sec_id+1)//2))}", fill=(255, 215, 60), font=FONT_SMALL)

            # Distance tag
            o_draw.text((sx + 10, 230), f"{si*1000}m", fill=(160, 190, 225), font=FONT_SMALL)

        # Final 10000m marker
        o_draw.line([(gx_end, 220), (gx_end, gy_base + 70)], fill=(60, 85, 120, 180), width=2)
        o_draw.text((gx_end - 45, 230), "10000m", fill=(160, 190, 225), font=FONT_SMALL)
        o_draw.text((gx_end - 50, card_y - 30), "FINISH", fill=(255, 255, 255), font=FONT_HEADING)

        over_path = os.path.join(over_out, "map_overview_10km.png")
        o_img.save(over_path, optimize=True)
        print(f"  [+] Saved overview map: {over_path}")

    print("\n=======================================================")
    print("ALL RADICALLY DIVERSE MAPS & SECTIONS GENERATED!")
    print("=======================================================")

if __name__ == "__main__":
    generate_all_diverse_assets()
