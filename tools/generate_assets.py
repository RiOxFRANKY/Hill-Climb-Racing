import os
import struct
import zlib
import math

def write_png(filename, width, height, rgba_data):
    """Writes a valid PNG file using pure standard library (zlib + struct)."""
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    
    png = bytearray(b'\x89PNG\r\n\x1a\n')
    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    ihdr_crc = zlib.crc32(b'IHDR' + ihdr_data)
    png.extend(struct.pack('>I', len(ihdr_data)))
    png.extend(b'IHDR')
    png.extend(ihdr_data)
    png.extend(struct.pack('>I', ihdr_crc))
    
    raw_bytes = bytearray()
    for y in range(height):
        raw_bytes.append(0)  # filter type 0
        row_start = y * width * 4
        raw_bytes.extend(rgba_data[row_start : row_start + width * 4])
        
    compressed_data = zlib.compress(bytes(raw_bytes), level=9)
    idat_crc = zlib.crc32(b'IDAT' + compressed_data)
    png.extend(struct.pack('>I', len(compressed_data)))
    png.extend(b'IDAT')
    png.extend(compressed_data)
    png.extend(struct.pack('>I', idat_crc))
    
    iend_crc = zlib.crc32(b'IEND')
    png.extend(struct.pack('>I', 0))
    png.extend(b'IEND')
    png.extend(struct.pack('>I', iend_crc))
    
    with open(filename, 'wb') as f:
        f.write(png)
    print(f"Generated {filename} [{width}x{height}]")

class Canvas:
    def __init__(self, w, h):
        self.w = w
        self.h = h
        self.data = bytearray(w * h * 4)

    def set_pixel(self, x, y, r, g, b, a=255):
        # Wrap x horizontally for seamless toroidal pixel drawing
        x = x % self.w
        if 0 <= y < self.h:
            idx = (y * self.w + x) * 4
            self.data[idx] = r
            self.data[idx + 1] = g
            self.data[idx + 2] = b
            self.data[idx + 3] = a

    def fill_rect(self, x, y, w, h, r, g, b, a=255):
        for cy in range(max(0, y), min(self.h, y + h)):
            for cx in range(x, x + w):
                self.set_pixel(cx, cy, r, g, b, a)

    def fill_circle(self, cx, cy, radius, r, g, b, a=255):
        r_sq = radius * radius
        for y in range(int(cy - radius), int(cy + radius + 1)):
            for x in range(int(cx - radius), int(cx + radius + 1)):
                if (x - cx)**2 + (y - cy)**2 <= r_sq:
                    self.set_pixel(x, y, r, g, b, a)

def generate_sky():
    base_dir = "assets"
    W, H = 960, 540
    sb = Canvas(W, H)
    
    # 1. Smooth vertical twilight-to-horizon atmosphere
    for y in range(H):
        t = y / float(H)
        if t < 0.65:
            lt = t / 0.65
            # Deep cobalt twilight (#1e3a8a) -> Radiant cyan sky (#38bdf8)
            r = int(30 * (1 - lt) + 56 * lt)
            g = int(58 * (1 - lt) + 189 * lt)
            b = int(138 * (1 - lt) + 248 * lt)
        else:
            lt = (t - 0.65) / 0.35
            # Radiant cyan sky -> Soft golden sunset horizon (#fef08a)
            r = int(56 * (1 - lt) + 254 * lt)
            g = int(189 * (1 - lt) + 240 * lt)
            b = int(248 * (1 - lt) + 138 * lt)
            
        for x in range(W):
            sb.set_pixel(x, y, r, g, b, 255)

    # 2. Seamless Distant Mountains (Back Layer, Slate-Blue/Indigo)
    # Fundamental frequency omega_0 = 2*PI / 960 ensures EXACT periodicity at x=0 and x=960!
    omega0 = 2.0 * math.pi / float(W)
    for x in range(W):
        # Exact periodic harmonics (k = 1, 2, 3, 5)
        h1 = (48.0 * math.sin(1.0 * omega0 * x + 0.4) +
              26.0 * math.sin(2.0 * omega0 * x + 1.8) +
              14.0 * math.cos(3.0 * omega0 * x + 2.5) +
               8.0 * math.sin(5.0 * omega0 * x + 0.9))
        mh1 = int(340.0 + h1)
        for y in range(mh1, H):
            sb.set_pixel(x, y, 100, 116, 139, 255) # Slate-500

    # 3. Seamless Midground Ridge (Front Layer, Deep Slate)
    # Exact periodic harmonics (k = 2, 3, 4, 6)
    for x in range(W):
        h2 = (32.0 * math.sin(2.0 * omega0 * x + 2.1) +
              18.0 * math.cos(3.0 * omega0 * x + 0.7) +
              10.0 * math.sin(4.0 * omega0 * x + 1.4) +
               6.0 * math.cos(6.0 * omega0 * x + 3.2))
        mh2 = int(405.0 + h2)
        for y in range(mh2, H):
            sb.set_pixel(x, y, 71, 85, 105, 255) # Slate-600

    # 4. Fluffy Clouds (Positioned neatly across the canvas with wraparound support)
    clouds = [
        (160, 110, 42),
        (380,  85, 52),
        (620, 120, 48),
        (840,  95, 38)
    ]
    for cx, cy, rad in clouds:
        for ox, oy, orad in [(0, 0, rad), (-rad * 2 // 3, 5, rad * 3 // 5), (rad * 2 // 3, 4, rad * 3 // 5)]:
            sb.fill_circle(cx + ox, cy + oy, orad, 255, 255, 255, 220)

    write_png(os.path.join(base_dir, "backgrounds", "sky_bg.png"), W, H, sb.data)
    print("Seamless sky_bg.png generated successfully!")

if __name__ == "__main__":
    generate_sky()
