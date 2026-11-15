"""Pure-Python generator for DirectorCutJam Act 1 placeholder sprites, environment tiles, and retro SFX.

Uses ONLY Python 3 standard library (zlib, struct, math, wave, os) so it runs directly inside:
    D:\\UE_5.8\\Engine\\Binaries\\ThirdParty\\Python3\\Win64\\python.exe

Rendering contract (docs/06_ART_DIRECTION.md & docs/08_LEVEL_01_FIREFIGHTER.md):
- Characters & world interaction props: 512x512 RGBA PNG, true 1:1 proportions, facing RIGHT by default.
- 2x supersampling (1024x1024) -> 1x downsampling ONLY on internal color boundaries.
- Strictly binary outer silhouette alpha {0, 255} (zero semi-transparent halo, zero ground shadows,
  zero cel-shading/gradients, zero thick black comic outer outlines, zero letters/numbers).
- Grounded sprites locked to Bottom-Center foot baseline at y = 511.
- Environment textures: 256x256 RGBA PNG, fully opaque (alpha = 255).
- Audio SFX: 44100 Hz 16-bit Mono PCM .wav files.
"""

import math
import os
import struct
import wave
import zlib


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))
CHARS_DIR = os.path.join(PROJECT_ROOT, "assets", "characters")
ENV_DIR = os.path.join(PROJECT_ROOT, "assets", "envirments")
INTERACT_DIR = os.path.join(PROJECT_ROOT, "assets", "envirments-character-interactions")
AUDIO_DIR = os.path.join(PROJECT_ROOT, "assets", "audio")


# ---------------------------------------------------------------------------
# Color Palette (Flat Illustration Tokens, sRGB uint8)
# ---------------------------------------------------------------------------
INK = (43, 39, 36)            # #2B2724 warm dark ink (facial features & thin internal accents)
PAPER = (255, 246, 230)       # #FFF6E6 warm cream (reflective stripes, bandages, eyeshine, foam)
SKIN = (247, 209, 164)        # #F7D1A4 warm Toro human skin
NOSE = (230, 159, 102)        # #E69F66 cute round potato nose
BLUSH = (244, 169, 160)       # #F4A9A0 blush ovals
HAIR_DARK = (110, 66, 38)     # #6E4226 chestnut brown hair (firefighter)
HAIR_LIGHT = (143, 91, 52)    # #8F5B34 soft brown hair (civilian)
HAIR_GREY = (107, 91, 80)     # #6B5B50 muted brown-grey (captain)

COAT_OCHRE = (229, 159, 56)   # #E59F38 firefighter ginger-ochre coat
COAT_TRIM = (206, 137, 40)    # #CE8928 subtle coat pocket/hem flat accent
NAVY_PANTS = (43, 62, 94)     # #2B3E5E navy firefighter trousers
BOOTS_DARK = (59, 49, 43)     # #3B312B dark boots
HELMET_RED = (201, 66, 50)    # #C94232 firefighter red helmet
HELMET_TRIM = (173, 50, 36)   # #AD3224 brim rim flat accent
GOLD_BADGE = (255, 215, 94)   # #FFD75E butter gold emblem / brass knob

CAPTAIN_BLUE = (79, 116, 166) # #4F74A6 captain slate-blue uniform
CAPTAIN_DARK = (46, 67, 99)   # #2E4363 captain pants / cap band

CIV_MINT = (77, 196, 158)     # #4DC49E civilian mint-green shirt
CIV_MINT_TRIM = (62, 168, 134)# #3EA886 subtle collar flat tone
CIV_SHORTS = (125, 110, 86)   # #7D6E56 warm olive-khaki pants
SOOT = (92, 83, 77)           # #5C534D flat soot smudge

SWEAT_BLUE = (158, 212, 238)  # #9ED4EE sky-blue sweat drop / water stream
WATER_DEEP = (107, 183, 222)  # #6BB7DE deeper sky stream accent

EXT_RED = (217, 59, 50)       # #D93B32 extinguisher body
EXT_BRASS = (232, 184, 75)    # #E8B84B valve collar

DOOR_OAK = (168, 109, 62)     # #A86D3E warm oak door body
DOOR_PANEL = (138, 83, 43)    # #8A532B recessed flat oak panels
DOOR_FRAME = (107, 62, 32)    # #6B3E20 dark oak trim

FIRE_CORAL = (255, 99, 38)    # #FF6326 outer flame tongue
FIRE_AMBER = (255, 166, 43)   # #FFA62B mid flame tongue
FIRE_BUTTER = (255, 232, 133) # #FFE885 bright butter core


# ---------------------------------------------------------------------------
# Fast Pure-Python 2x Supersampled Canvas & Scanline Rasterizer
# ---------------------------------------------------------------------------
class Canvas2x:
    """2x supersampled flat-color rasterizer outputting binary-alpha PNGs."""

    def __init__(self, out_width, out_height, opaque_bg=None):
        self.out_w = out_width
        self.out_h = out_height
        self.w = out_width * 2
        self.h = out_height * 2
        size = self.w * self.h
        if opaque_bg is not None:
            r, g, b = opaque_bg
            self.r = bytearray([r]) * size
            self.g = bytearray([g]) * size
            self.b = bytearray([b]) * size
            self.a = bytearray([255]) * size
            self.min_x = 0
            self.max_x = self.w - 1
            self.min_y = 0
            self.max_y = self.h - 1
        else:
            self.r = bytearray(size)
            self.g = bytearray(size)
            self.b = bytearray(size)
            self.a = bytearray(size)
            self.min_x = self.w
            self.max_x = -1
            self.min_y = self.h
            self.max_y = -1

    def _expand_bounds(self, x0, y0, x1, y1):
        if x0 < self.min_x:
            self.min_x = max(0, x0)
        if x1 > self.max_x:
            self.max_x = min(self.w - 1, x1)
        if y0 < self.min_y:
            self.min_y = max(0, y0)
        if y1 > self.max_y:
            self.max_y = min(self.h - 1, y1)

    def _fill_span(self, y, x0, x1, color, inside_only=False):
        if y < 0 or y >= self.h:
            return
        if x0 < 0:
            x0 = 0
        if x1 > self.w:
            x1 = self.w
        if x0 >= x1:
            return
        cr, cg, cb = color
        row_off = y * self.w
        s = row_off + x0
        e = row_off + x1
        if not inside_only:
            span = e - s
            self.r[s:e] = bytes((cr,)) * span
            self.g[s:e] = bytes((cg,)) * span
            self.b[s:e] = bytes((cb,)) * span
            self.a[s:e] = b"\xff" * span
            self._expand_bounds(x0, y, x1 - 1, y)
        else:
            a_buf = self.a
            r_buf = self.r
            g_buf = self.g
            b_buf = self.b
            for idx in range(s, e):
                if a_buf[idx]:
                    r_buf[idx] = cr
                    g_buf[idx] = cg
                    b_buf[idx] = cb

    def fill_rect(self, x0, y0, x1, y1, color, inside_only=False):
        ix0 = max(0, int(math.floor(x0)))
        iy0 = max(0, int(math.floor(y0)))
        ix1 = min(self.w, int(math.ceil(x1)))
        iy1 = min(self.h, int(math.ceil(y1)))
        if ix0 >= ix1 or iy0 >= iy1:
            return
        for y in range(iy0, iy1):
            self._fill_span(y, ix0, ix1, color, inside_only=inside_only)

    def fill_round_rect(self, x0, y0, x1, y1, radius, color, inside_only=False):
        if x0 > x1:
            x0, x1 = x1, x0
        if y0 > y1:
            y0, y1 = y1, y0
        r = max(0.0, min(radius, 0.5 * (x1 - x0), 0.5 * (y1 - y0)))
        ix0 = max(0, int(math.floor(x0)))
        iy0 = max(0, int(math.floor(y0)))
        ix1 = min(self.w, int(math.ceil(x1)))
        iy1 = min(self.h, int(math.ceil(y1)))
        if ix0 >= ix1 or iy0 >= iy1:
            return
        inner_x0 = x0 + r
        inner_x1 = x1 - r
        inner_y0 = y0 + r
        inner_y1 = y1 - r
        r2 = r * r
        for y in range(iy0, iy1):
            py = y + 0.5
            if py < inner_y0:
                dy = inner_y0 - py
                dx = math.sqrt(max(0.0, r2 - dy * dy))
                sx0 = int(math.floor(inner_x0 - dx))
                sx1 = int(math.ceil(inner_x1 + dx))
            elif py > inner_y1:
                dy = py - inner_y1
                dx = math.sqrt(max(0.0, r2 - dy * dy))
                sx0 = int(math.floor(inner_x0 - dx))
                sx1 = int(math.ceil(inner_x1 + dx))
            else:
                sx0 = ix0
                sx1 = ix1
            self._fill_span(y, sx0, sx1, color, inside_only=inside_only)

    def fill_circle(self, cx, cy, r, color, inside_only=False):
        if r <= 0:
            return
        iy0 = max(0, int(math.floor(cy - r)))
        iy1 = min(self.h, int(math.ceil(cy + r)))
        r2 = r * r
        for y in range(iy0, iy1):
            dy = (y + 0.5) - cy
            rem = r2 - dy * dy
            if rem >= 0.0:
                dx = math.sqrt(rem)
                sx0 = int(math.floor(cx - dx))
                sx1 = int(math.ceil(cx + dx))
                self._fill_span(y, sx0, sx1, color, inside_only=inside_only)

    def fill_ellipse(self, cx, cy, rx, ry, color, angle_deg=0.0, inside_only=False):
        if rx <= 0 or ry <= 0:
            return
        if abs(angle_deg) < 1e-4:
            iy0 = max(0, int(math.floor(cy - ry)))
            iy1 = min(self.h, int(math.ceil(cy + ry)))
            inv_ry2 = 1.0 / (ry * ry)
            for y in range(iy0, iy1):
                dy = (y + 0.5) - cy
                t = 1.0 - (dy * dy) * inv_ry2
                if t >= 0.0:
                    dx = rx * math.sqrt(t)
                    sx0 = int(math.floor(cx - dx))
                    sx1 = int(math.ceil(cx + dx))
                    self._fill_span(y, sx0, sx1, color, inside_only=inside_only)
            return

        rad = math.radians(angle_deg)
        ca = math.cos(rad)
        sa = math.sin(rad)
        # Quadratic equation for rotated ellipse in local coordinates:
        # ((dx*ca + dy*sa)/rx)^2 + ((-dx*sa + dy*ca)/ry)^2 <= 1
        inv_rx2 = 1.0 / (rx * rx)
        inv_ry2 = 1.0 / (ry * ry)
        A = ca * ca * inv_rx2 + sa * sa * inv_ry2
        B_half = ca * sa * (inv_rx2 - inv_ry2)
        C = sa * sa * inv_rx2 + ca * ca * inv_ry2
        bound_x = math.sqrt(rx * rx * ca * ca + ry * ry * sa * sa)
        bound_y = math.sqrt(rx * rx * sa * sa + ry * ry * ca * ca)
        iy0 = max(0, int(math.floor(cy - bound_y)))
        iy1 = min(self.h, int(math.ceil(cy + bound_y)))
        for y in range(iy0, iy1):
            dy = (y + 0.5) - cy
            # A * dx^2 + (2 * B_half * dy) * dx + (C * dy^2 - 1) <= 0
            disc = (B_half * dy) ** 2 - A * (C * dy * dy - 1.0)
            if disc >= 0.0:
                sd = math.sqrt(disc)
                mid = -B_half * dy / A
                half = sd / A
                sx0 = int(math.floor(cx + mid - half))
                sx1 = int(math.ceil(cx + mid + half))
                self._fill_span(y, sx0, sx1, color, inside_only=inside_only)

    def fill_capsule(self, x0, y0, x1, y1, r, color, inside_only=False):
        if r <= 0:
            return
        dx = x1 - x0
        dy = y1 - y0
        len2 = dx * dx + dy * dy
        if len2 < 1e-6:
            self.fill_circle(x0, y0, r, color, inside_only=inside_only)
            return
        length = math.sqrt(len2)
        ux = dx / length
        uy = dy / length
        nx = -uy * r
        ny = ux * r
        # Rounded ends + connecting quad polygon
        self.fill_circle(x0, y0, r, color, inside_only=inside_only)
        self.fill_circle(x1, y1, r, color, inside_only=inside_only)
        self.fill_poly(
            [(x0 + nx, y0 + ny), (x1 + nx, y1 + ny), (x1 - nx, y1 - ny), (x0 - nx, y0 - ny)],
            color,
            inside_only=inside_only,
        )

    def draw_polyline(self, pts, thickness, color, inside_only=False):
        r = thickness * 0.5
        for i in range(len(pts) - 1):
            self.fill_capsule(pts[i][0], pts[i][1], pts[i + 1][0], pts[i + 1][1], r, color, inside_only=inside_only)

    def fill_poly(self, pts, color, inside_only=False):
        n = len(pts)
        if n < 3:
            return
        ys = [p[1] for p in pts]
        iy0 = max(0, int(math.floor(min(ys))))
        iy1 = min(self.h, int(math.ceil(max(ys))))
        for y in range(iy0, iy1):
            py = y + 0.5
            xs = []
            j = n - 1
            for i in range(n):
                xi, yi = pts[i]
                xj, yj = pts[j]
                if (yi <= py < yj) or (yj <= py < yi):
                    x_hit = xi + (py - yi) * (xj - xi) / (yj - yi)
                    xs.append(x_hit)
                j = i
            if len(xs) >= 2:
                xs.sort()
                for k in range(0, len(xs) - 1, 2):
                    sx0 = int(math.floor(xs[k]))
                    sx1 = int(math.ceil(xs[k + 1]))
                    self._fill_span(y, sx0, sx1, color, inside_only=inside_only)

    def downsample_binary_alpha_rgba(self, force_opaque=False):
        """Downsamples 2x buffer to 1x RGBA with strict {0, 255} outer alpha and anti-aliased interior."""
        ow = self.out_w
        oh = self.out_h
        out = bytearray(ow * oh * 4)
        if self.max_x < self.min_x or self.max_y < self.min_y:
            return out

        ox_min = max(0, self.min_x // 2)
        ox_max = min(ow - 1, self.max_x // 2)
        oy_min = max(0, self.min_y // 2)
        oy_max = min(oh - 1, self.max_y // 2)

        w2 = self.w
        rb = self.r
        gb = self.g
        bb = self.b
        ab = self.a

        for y in range(oy_min, oy_max + 1):
            row0 = (2 * y) * w2
            row1 = row0 + w2
            out_row = y * ow * 4
            for x in range(ox_min, ox_max + 1):
                i00 = row0 + 2 * x
                i01 = i00 + 1
                i10 = row1 + 2 * x
                i11 = i10 + 1

                a00 = ab[i00]
                a01 = ab[i01]
                a10 = ab[i10]
                a11 = ab[i11]

                if force_opaque or (a00 == 255 and a01 == 255 and a10 == 255 and a11 == 255):
                    p = out_row + x * 4
                    out[p] = (rb[i00] + rb[i01] + rb[i10] + rb[i11] + 2) >> 2
                    out[p + 1] = (gb[i00] + gb[i01] + gb[i10] + gb[i11] + 2) >> 2
                    out[p + 2] = (bb[i00] + bb[i01] + bb[i10] + bb[i11] + 2) >> 2
                    out[p + 3] = 255
                    continue

                m00 = a00 & 1
                m01 = a01 & 1
                m10 = a10 & 1
                m11 = a11 & 1
                cnt = m00 + m01 + m10 + m11
                if cnt >= 2:
                    p = out_row + x * 4
                    half = cnt >> 1
                    out[p] = (rb[i00] * m00 + rb[i01] * m01 + rb[i10] * m10 + rb[i11] * m11 + half) // cnt
                    out[p + 1] = (gb[i00] * m00 + gb[i01] * m01 + gb[i10] * m10 + gb[i11] * m11 + half) // cnt
                    out[p + 2] = (bb[i00] * m00 + bb[i01] * m01 + bb[i10] * m10 + bb[i11] * m11 + half) // cnt
                    out[p + 3] = 255
        return out


def write_png_rgba(filepath, width, height, rgba_bytes):
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    stride = width * 4
    raw = bytearray((stride + 1) * height)
    for y in range(height):
        src_s = y * stride
        dst_s = y * (stride + 1)
        raw[dst_s] = 0  # filter type None
        raw[dst_s + 1: dst_s + 1 + stride] = rgba_bytes[src_s: src_s + stride]

    def make_chunk(chunk_type, data):
        crc = zlib.crc32(chunk_type + data) & 0xFFFFFFFF
        return struct.pack(">I", len(data)) + chunk_type + data + struct.pack(">I", crc)

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    idat = zlib.compress(bytes(raw), level=6)
    png = b"\x89PNG\r\n\x1a\n" + make_chunk(b"IHDR", ihdr) + make_chunk(b"IDAT", idat) + make_chunk(b"IEND", b"")
    with open(filepath, "wb") as f:
        f.write(png)


def save_canvas(canvas, filepath, force_opaque=False):
    rgba = canvas.downsample_binary_alpha_rgba(force_opaque=force_opaque)
    write_png_rgba(filepath, canvas.out_w, canvas.out_h, rgba)


# ---------------------------------------------------------------------------
# Geometry Helpers (Bezier & Curves for Organic Flat Illustration Shapes)
# ---------------------------------------------------------------------------
def sample_quad_bezier(p0, p1, p2, steps=12):
    pts = []
    for i in range(steps + 1):
        t = i / steps
        mt = 1.0 - t
        x = mt * mt * p0[0] + 2.0 * mt * t * p1[0] + t * t * p2[0]
        y = mt * mt * p0[1] + 2.0 * mt * t * p1[1] + t * t * p2[1]
        pts.append((x, y))
    return pts


def draw_teardrop(c, tip_x, tip_y, base_x, base_y, width_r, color):
    """Draws a crisp tapered teardrop / flame tongue from base to tip."""
    dx = tip_x - base_x
    dy = tip_y - base_y
    length = math.hypot(dx, dy)
    if length < 1e-3:
        return
    ux, uy = dx / length, dy / length
    nx, ny = -uy, ux
    p_left = (base_x + nx * width_r, base_y + ny * width_r)
    p_right = (base_x - nx * width_r, base_y - ny * width_r)
    ctrl_l = (base_x + dx * 0.42 + nx * width_r * 1.15, base_y + dy * 0.42 + ny * width_r * 1.15)
    ctrl_r = (base_x + dx * 0.42 - nx * width_r * 1.15, base_y + dy * 0.42 - ny * width_r * 1.15)
    ctrl_b = (base_x - ux * width_r * 0.9, base_y - uy * width_r * 0.9)
    poly = (
        sample_quad_bezier(p_left, ctrl_l, (tip_x, tip_y), 10)
        + sample_quad_bezier((tip_x, tip_y), ctrl_r, p_right, 10)[1:]
        + sample_quad_bezier(p_right, ctrl_b, p_left, 8)[1:-1]
    )
    c.fill_poly(poly, color)


def draw_diamond(c, cx, cy, rx, ry, color):
    c.fill_poly([(cx, cy - ry), (cx + rx, cy), (cx, cy + ry), (cx - rx, cy)], color)


def draw_star4(c, cx, cy, r_outer, r_inner, color):
    pts = []
    for i in range(8):
        ang = math.radians(i * 45.0)
        r = r_outer if (i % 2 == 0) else r_inner
        pts.append((cx + math.cos(ang) * r, cy + math.sin(ang) * r))
    c.fill_poly(pts, color)


# ---------------------------------------------------------------------------
# Toro 3.2-Head Human Character Parts (2x space: 1024x1024, y2=1024 is foot line)
# ---------------------------------------------------------------------------
def draw_toro_head(
    c,
    cx,
    cy,
    s=1.0,
    facing_dx=22.0,
    hair_color=HAIR_DARK,
    hair_style="bangs",
    expr="determined",
    soot=False,
    bandage=False,
    sweat=False,
    head_tilt_deg=0.0,
):
    """Draws cute rounded Toro bun head, ears, hair, potato nose, blush & expressive ink face."""
    # Back-left ear (visible in 3/4 right pose) and smaller right ear
    ear_lx = cx - 160 * s
    ear_ly = cy + 12 * s
    c.fill_circle(ear_lx, ear_ly, 26 * s, SKIN)
    c.fill_circle(ear_lx + 4 * s, ear_ly + 2 * s, 10 * s, NOSE)

    if facing_dx < 10.0:
        ear_rx = cx + 160 * s
        c.fill_circle(ear_rx, ear_ly, 26 * s, SKIN)
        c.fill_circle(ear_rx - 4 * s, ear_ly + 2 * s, 10 * s, NOSE)
    else:
        ear_rx = cx + 152 * s
        c.fill_circle(ear_rx, ear_ly + 2 * s, 19 * s, SKIN)

    # Rounded bun/squircle face
    c.fill_ellipse(cx, cy - 6 * s, 156 * s, 120 * s, SKIN, head_tilt_deg)
    c.fill_ellipse(cx + 2 * s, cy + 28 * s, 162 * s, 90 * s, SKIN, head_tilt_deg)

    # Hair cap
    if hair_style == "bangs":
        c.fill_ellipse(cx - 6 * s, cy - 52 * s, 160 * s, 88 * s, hair_color)
        # Back-left hair side sweep
        c.fill_ellipse(cx - 122 * s, cy - 18 * s, 38 * s, 70 * s, hair_color)
        # Cute jagged bangs across forehead
        bangs_y0 = cy - 110 * s
        bangs_y1 = cy - 42 * s
        bangs_pts = [
            (cx - 156 * s, cy - 30 * s),
            (cx - 148 * s, bangs_y0),
            (cx + 150 * s, bangs_y0),
            (cx + 152 * s, cy - 38 * s),
            (cx + 108 * s, bangs_y1 - 8 * s),
            (cx + 76 * s, bangs_y1 - 30 * s),
            (cx + 42 * s, bangs_y1),
            (cx + 8 * s, bangs_y1 - 28 * s),
            (cx - 32 * s, bangs_y1 + 2 * s),
            (cx - 66 * s, bangs_y1 - 28 * s),
            (cx - 102 * s, bangs_y1 - 4 * s),
            (cx - 134 * s, bangs_y1 - 20 * s),
        ]
        c.fill_poly(bangs_pts, hair_color)
    elif hair_style == "fluffy":
        # Fluffy civilian puff hair (like example_5 right character)
        c.fill_ellipse(cx - 14 * s, cy - 72 * s, 142 * s, 72 * s, hair_color)
        c.fill_circle(cx - 72 * s, cy - 92 * s, 58 * s, hair_color)
        c.fill_circle(cx + 26 * s, cy - 98 * s, 62 * s, hair_color)
        c.fill_circle(cx + 100 * s, cy - 72 * s, 44 * s, hair_color)
        c.fill_ellipse(cx - 126 * s, cy - 26 * s, 32 * s, 62 * s, hair_color)
    elif hair_style == "short_cap":
        # Neat side hair peeking under captain cap
        c.fill_ellipse(cx - 134 * s, cy - 24 * s, 28 * s, 46 * s, hair_color)
        c.fill_ellipse(cx + 134 * s, cy - 24 * s, 24 * s, 42 * s, hair_color)
        c.fill_ellipse(cx, cy - 72 * s, 152 * s, 44 * s, hair_color)

    # Soot smudge on cheek/forehead for injured/dead civilian (kept clear of eyes & blush)
    if soot:
        c.fill_ellipse(cx + facing_dx * s - 92 * s, cy - 36 * s, 20 * s, 12 * s, SOOT, -18.0, inside_only=True)
        c.fill_ellipse(cx + facing_dx * s + 94 * s, cy + 16 * s, 18 * s, 11 * s, SOOT, 15.0, inside_only=True)
        c.fill_circle(cx + facing_dx * s - 28 * s, cy + 68 * s, 10 * s, SOOT, inside_only=True)
        c.fill_circle(cx + facing_dx * s - 42 * s, cy + 64 * s, 7 * s, SOOT, inside_only=True)

    # Small forehead/temple bandage (injured civilian, clear of eyes & blush)
    if bandage:
        c.fill_round_rect(cx + facing_dx * s + 24 * s, cy - 58 * s, cx + facing_dx * s + 74 * s, cy - 36 * s, 6 * s, PAPER)
        c.draw_polyline(
            [(cx + facing_dx * s + 40 * s, cy - 54 * s), (cx + facing_dx * s + 40 * s, cy - 40 * s)],
            3 * s,
            (215, 200, 182),
        )
        c.draw_polyline(
            [(cx + facing_dx * s + 58 * s, cy - 54 * s), (cx + facing_dx * s + 58 * s, cy - 40 * s)],
            3 * s,
            (215, 200, 182),
        )

    # Facial feature center shifted right by facing_dx
    fx = cx + facing_dx * s

    # Blush ovals (#F4A9A0)
    c.fill_ellipse(fx - 76 * s, cy + 44 * s, 27 * s, 16 * s, BLUSH)
    c.fill_ellipse(fx + 74 * s, cy + 44 * s, 23 * s, 15 * s, BLUSH)

    # Cute round potato nose (#E69F66)
    c.fill_ellipse(fx + 4 * s, cy + 16 * s, 18 * s, 14 * s, NOSE)

    # Eyes, eyebrows & mouth by expression
    elx = fx - 54 * s
    erx = fx + 54 * s
    ey = cy - 4 * s

    if expr == "determined":
        c.fill_ellipse(elx, ey, 15 * s, 20 * s, INK)
        c.fill_ellipse(erx, ey, 14 * s, 19 * s, INK)
        c.fill_circle(elx - 4 * s, ey - 7 * s, 5 * s, PAPER)
        c.fill_circle(erx - 4 * s, ey - 7 * s, 5 * s, PAPER)
        # Determined angled brows
        c.fill_capsule(elx - 20 * s, ey - 38 * s, elx + 16 * s, ey - 30 * s, 4.5 * s, INK)
        c.fill_capsule(erx - 16 * s, ey - 30 * s, erx + 20 * s, ey - 38 * s, 4.5 * s, INK)
        # Confident little mouth
        c.draw_polyline(
            [(fx - 12 * s, cy + 50 * s), (fx + 4 * s, cy + 55 * s), (fx + 20 * s, cy + 49 * s)],
            6.5 * s,
            INK,
        )

    elif expr == "idle_blink_soft":
        # Subtle breathing frame 1: slightly wider cheerful eyes + open small smile
        c.fill_ellipse(elx, ey + 2 * s, 16 * s, 17 * s, INK)
        c.fill_ellipse(erx, ey + 2 * s, 15 * s, 16 * s, INK)
        c.fill_circle(elx - 4 * s, ey - 4 * s, 5 * s, PAPER)
        c.fill_circle(erx - 4 * s, ey - 4 * s, 5 * s, PAPER)
        c.fill_capsule(elx - 20 * s, ey - 35 * s, elx + 16 * s, ey - 28 * s, 4.5 * s, INK)
        c.fill_capsule(erx - 16 * s, ey - 28 * s, erx + 20 * s, ey - 35 * s, 4.5 * s, INK)
        c.draw_polyline(
            [(fx - 14 * s, cy + 49 * s), (fx + 4 * s, cy + 57 * s), (fx + 22 * s, cy + 49 * s)],
            6.5 * s,
            INK,
        )

    elif expr == "scared":
        # Startled round eyes + 八字苦瓜眉 + wobbly mouth + sweat drop
        c.fill_circle(elx, ey + 2 * s, 19 * s, INK)
        c.fill_circle(erx, ey + 2 * s, 18 * s, INK)
        c.fill_circle(elx - 5 * s, ey - 5 * s, 6 * s, PAPER)
        c.fill_circle(erx - 5 * s, ey - 5 * s, 6 * s, PAPER)
        # 八字苦瓜眉 (inner ends UP)
        c.fill_capsule(elx - 22 * s, ey - 28 * s, elx + 16 * s, ey - 44 * s, 5.0 * s, INK)
        c.fill_capsule(erx - 16 * s, ey - 44 * s, erx + 22 * s, ey - 28 * s, 5.0 * s, INK)
        # Wobbly mouth
        c.draw_polyline(
            [
                (fx - 26 * s, cy + 56 * s),
                (fx - 12 * s, cy + 46 * s),
                (fx + 4 * s, cy + 58 * s),
                (fx + 20 * s, cy + 46 * s),
                (fx + 32 * s, cy + 56 * s),
            ],
            6.5 * s,
            INK,
        )
        if sweat:
            draw_teardrop(c, cx - 108 * s, cy - 36 * s, cx - 108 * s, cy + 10 * s, 13 * s, SWEAT_BLUE)

    elif expr == "spray":
        # Squinting > < eyes + furrowed brows + 咬牙囧脸 clenched teeth
        c.draw_polyline([(elx - 18 * s, ey - 14 * s), (elx + 14 * s, ey), (elx - 18 * s, ey + 14 * s)], 8 * s, INK)
        c.draw_polyline([(erx + 18 * s, ey - 14 * s), (erx - 14 * s, ey), (erx + 18 * s, ey + 14 * s)], 8 * s, INK)
        c.fill_capsule(elx - 20 * s, ey - 38 * s, elx + 16 * s, ey - 24 * s, 5.0 * s, INK)
        c.fill_capsule(erx - 16 * s, ey - 24 * s, erx + 20 * s, ey - 38 * s, 5.0 * s, INK)
        # Clenched teeth box
        c.fill_round_rect(fx - 28 * s, cy + 40 * s, fx + 34 * s, cy + 68 * s, 11 * s, INK)
        c.fill_round_rect(fx - 22 * s, cy + 45 * s, fx + 28 * s, cy + 63 * s, 7 * s, PAPER)
        c.fill_capsule(fx - 6 * s, cy + 45 * s, fx - 6 * s, cy + 63 * s, 2.5 * s, INK)
        c.fill_capsule(fx + 12 * s, cy + 45 * s, fx + 12 * s, cy + 63 * s, 2.5 * s, INK)

    elif expr == "captain_stern":
        # Stern flat brows + half-lidded grumpy eyes + frown
        c.fill_ellipse(elx, ey + 6 * s, 13 * s, 11 * s, INK)
        c.fill_ellipse(erx, ey + 6 * s, 13 * s, 11 * s, INK)
        # Flat eyelid lines
        c.fill_capsule(elx - 20 * s, ey - 4 * s, elx + 18 * s, ey - 4 * s, 4.5 * s, INK)
        c.fill_capsule(erx - 18 * s, ey - 4 * s, erx + 20 * s, ey - 4 * s, 4.5 * s, INK)
        # Heavy stern flat brows
        c.fill_capsule(elx - 24 * s, ey - 26 * s, elx + 22 * s, ey - 24 * s, 6.5 * s, INK)
        c.fill_capsule(erx - 22 * s, ey - 24 * s, erx + 24 * s, ey - 26 * s, 6.5 * s, INK)
        # Grumpy frown
        c.draw_polyline(
            [(fx - 20 * s, cy + 60 * s), (fx, cy + 48 * s), (fx + 20 * s, cy + 60 * s)],
            7 * s,
            INK,
        )

    elif expr == "civ_help":
        # Wide-eyed pleading face
        c.fill_ellipse(elx, ey, 16 * s, 21 * s, INK)
        c.fill_ellipse(erx, ey, 15 * s, 20 * s, INK)
        c.fill_circle(elx - 4 * s, ey - 7 * s, 5.5 * s, PAPER)
        c.fill_circle(erx - 4 * s, ey - 7 * s, 5.5 * s, PAPER)
        c.fill_capsule(elx - 20 * s, ey - 32 * s, elx + 14 * s, ey - 44 * s, 4.5 * s, INK)
        c.fill_capsule(erx - 14 * s, ey - 44 * s, erx + 20 * s, ey - 32 * s, 4.5 * s, INK)
        c.fill_ellipse(fx + 2 * s, cy + 54 * s, 15 * s, 18 * s, INK)
        c.fill_ellipse(fx + 2 * s, cy + 56 * s, 9 * s, 11 * s, BLUSH)

    elif expr == "civ_injured":
        # 八字苦瓜眉 + squint wincing arcs + trembling mouth
        c.draw_polyline([(elx - 18 * s, ey - 10 * s), (elx + 12 * s, ey + 2 * s), (elx - 16 * s, ey + 12 * s)], 7 * s, INK)
        c.draw_polyline([(erx + 18 * s, ey - 10 * s), (erx - 12 * s, ey + 2 * s), (erx + 16 * s, ey + 12 * s)], 7 * s, INK)
        c.fill_capsule(elx - 22 * s, ey - 24 * s, elx + 14 * s, ey - 40 * s, 5.0 * s, INK)
        c.fill_capsule(erx - 14 * s, ey - 40 * s, erx + 22 * s, ey - 24 * s, 5.0 * s, INK)
        c.draw_polyline(
            [(fx - 20 * s, cy + 58 * s), (fx - 4 * s, cy + 48 * s), (fx + 14 * s, cy + 58 * s)],
            6.5 * s,
            INK,
        )

    elif expr == "civ_dead":
        # Comedic dizzy X eyes + wavy mouth
        for ex in (elx, erx):
            c.fill_capsule(ex - 14 * s, ey - 12 * s, ex + 14 * s, ey + 12 * s, 4.5 * s, INK)
            c.fill_capsule(ex - 14 * s, ey + 12 * s, ex + 14 * s, ey - 12 * s, 4.5 * s, INK)
        c.draw_polyline(
            [(fx - 20 * s, cy + 52 * s), (fx - 6 * s, cy + 60 * s), (fx + 10 * s, cy + 50 * s), (fx + 24 * s, cy + 58 * s)],
            6.0 * s,
            INK,
        )


def draw_helmet(c, cx, cy, s=1.0, facing_dx=22.0):
    """Draws cute symmetric red firefighter helmet sitting cleanly ABOVE brows & eyes."""
    # Upper red dome: ONLY top half-ellipse ending cleanly at brim_y so eyes/bangs stay visible!
    brim_y = cy - 96 * s
    rx = 152 * s
    ry = 114 * s
    dome_pts = []
    steps = 28
    for i in range(steps + 1):
        ang = math.pi + (math.pi * i / steps)  # pi (left) -> 2*pi (right) over upper half
        dome_pts.append((cx + math.cos(ang) * rx, brim_y + math.sin(ang) * ry))
    c.fill_poly(dome_pts, HELMET_RED)

    # Top crest ridge
    c.fill_round_rect(cx - 42 * s, brim_y - 122 * s, cx + 42 * s, brim_y - 84 * s, 16 * s, HELMET_RED)
    # Brim capsule across brim_y
    c.fill_capsule(cx - 178 * s, brim_y + 4 * s, cx + 178 * s, brim_y + 4 * s, 15 * s, HELMET_TRIM)
    c.fill_capsule(cx - 174 * s, brim_y, cx + 174 * s, brim_y, 12 * s, HELMET_RED)
    # Symmetric cream/gold diamond emblem on front-right brow
    draw_diamond(c, cx + facing_dx * s, brim_y - 52 * s, 21 * s, 28 * s, PAPER)
    draw_diamond(c, cx + facing_dx * s, brim_y - 52 * s, 13 * s, 18 * s, GOLD_BADGE)


def draw_extinguisher_prop(c, cx, cy, s=1.0, angle_deg=0.0):
    """Draws chubby flat-color red extinguisher (with cream gauge, brass valve & black hose, NO letters)."""
    rad = math.radians(angle_deg)
    ca = math.cos(rad)
    sa = math.sin(rad)

    def rot(lx, ly):
        return (cx + (lx * ca - ly * sa) * s, cy + (lx * sa + ly * ca) * s)

    # Canister body (rounded rect rotated)
    c.fill_ellipse(*rot(0, 12), 54 * s, 84 * s, EXT_RED, angle_deg)
    c.fill_ellipse(*rot(0, -34), 50 * s, 46 * s, EXT_RED, angle_deg)
    c.fill_ellipse(*rot(0, 56), 54 * s, 40 * s, EXT_RED, angle_deg)
    # Subtle cream reflective band (no text!)
    p_band = [rot(-48, 22), rot(48, 22), rot(48, 42), rot(-48, 42)]
    c.fill_poly(p_band, PAPER, inside_only=True)
    # Neck + brass valve + handle
    c.fill_ellipse(*rot(0, -82), 20 * s, 16 * s, EXT_BRASS, angle_deg)
    c.fill_capsule(*rot(-22, -96), *rot(18, -96), 8 * s, INK)
    c.fill_capsule(*rot(-18, -110), *rot(26, -102), 6 * s, INK)
    # Pressure gauge (cream circle + dark rim + tiny hand)
    gx, gy = rot(10, -56)
    c.fill_circle(gx, gy, 17 * s, INK)
    c.fill_circle(gx, gy, 12 * s, PAPER)
    c.fill_capsule(gx, gy, *rot(16, -63), 2.5 * s, EXT_RED)
    # Hose & nozzle pointing right
    h0 = rot(16, -84)
    h1 = rot(52, -74)
    h2 = rot(70, -40)
    h3 = rot(94, -36)
    c.draw_polyline([h0, h1, h2, h3], 13 * s, INK)
    # Flared nozzle horn
    nozzle_pts = [rot(84, -46), rot(116, -54), rot(116, -18), rot(84, -26)]
    c.fill_poly(nozzle_pts, INK)


def draw_leg(c, hip_x, hip_y, foot_x, foot_y, pants_color, boot_color, s=1.0, knee_bend_x=0.0):
    """Draws thin tube leg + rounded boot ending cleanly at foot_y."""
    mid_x = 0.5 * (hip_x + foot_x) + knee_bend_x * s
    mid_y = 0.5 * (hip_y + foot_y)
    boot_top_y = foot_y - 16 * s
    c.fill_capsule(hip_x, hip_y, mid_x, mid_y, 20 * s, pants_color)
    c.fill_capsule(mid_x, mid_y, foot_x, boot_top_y, 19 * s, pants_color)
    # Rounded boot sole resting horizontally at foot_y
    c.fill_capsule(foot_x - 8 * s, boot_top_y + 2 * s, foot_x + 22 * s, boot_top_y + 2 * s, 16 * s, boot_color)


def draw_arm(c, sh_x, sh_y, hand_x, hand_y, sleeve_color, s=1.0, elbow_dx=0.0, elbow_dy=0.0, cuff_color=None):
    mid_x = 0.5 * (sh_x + hand_x) + elbow_dx * s
    mid_y = 0.5 * (sh_y + hand_y) + elbow_dy * s
    cuff_x = hand_x - (hand_x - mid_x) * 0.14
    cuff_y = hand_y - (hand_y - mid_y) * 0.14
    c.fill_capsule(sh_x, sh_y, mid_x, mid_y, 17 * s, sleeve_color)
    c.fill_capsule(mid_x, mid_y, cuff_x, cuff_y, 16 * s, sleeve_color)
    if cuff_color is not None:
        c.fill_circle(cuff_x, cuff_y, 16.5 * s, cuff_color)
    c.fill_circle(hand_x, hand_y, 18 * s, SKIN)


def draw_firefighter_figure(
    c,
    cx=512,
    bob_y=0.0,
    lean_deg=0.0,
    leg_l_dx=-42.0,
    leg_l_lift=0.0,
    leg_r_dx=42.0,
    leg_r_lift=0.0,
    arm_l=None,
    arm_r=None,
    expr="determined",
    holding_tool=False,
    spraying=False,
    sweat=False,
    s=1.0,
):
    """Draws complete firefighter centered at cx with feet baseline at y2 = 1024."""
    foot_base = 1024.0
    # Body anchor Y shifts up only by bob_y while planted foot touches 1024
    hip_y = 772.0 - bob_y * s
    sh_y = 574.0 - bob_y * s
    head_cx = cx + lean_deg * 2.2 * s
    head_cy = 398.0 - bob_y * s

    # Far-side arm behind coat (right arm in 3/4 right facing)
    if arm_r is not None:
        draw_arm(c, cx + 44 * s, sh_y + 8 * s, arm_r[0], arm_r[1], COAT_OCHRE, s, arm_r[2], arm_r[3])

    # Back leg
    draw_leg(c, cx - 34 * s, hip_y, cx + leg_l_dx * s, foot_base - leg_l_lift * s, NAVY_PANTS, BOOTS_DARK, s, 8.0)
    # Front leg
    draw_leg(c, cx + 34 * s, hip_y, cx + leg_r_dx * s, foot_base - leg_r_lift * s, NAVY_PANTS, BOOTS_DARK, s, 10.0)

    # Neck
    c.fill_capsule(head_cx - 4 * s, head_cy + 98 * s, cx, sh_y + 12 * s, 26 * s, SKIN)

    # Ginger-ochre trapezoid coat (#E59F38) + cream reflective band (#FFF6E6)
    top_dx = lean_deg * 0.8 * s
    coat_x0_top = cx - 64 * s + top_dx
    coat_x1_top = cx + 68 * s + top_dx
    coat_y_top = sh_y - 12 * s
    coat_x0_bot = cx - 98 * s
    coat_x1_bot = cx + 104 * s
    coat_y_bot = hip_y + 24 * s
    coat_poly = [
        (coat_x0_top, coat_y_top),
        (coat_x1_top, coat_y_top),
        (coat_x1_bot, coat_y_bot),
        (coat_x0_bot, coat_y_bot),
    ]
    c.fill_poly(coat_poly, COAT_OCHRE)
    # Rounded hem softness
    c.fill_capsule(cx - 90 * s, hip_y + 22 * s, cx + 96 * s, hip_y + 22 * s, 14 * s, COAT_OCHRE)
    # Cream reflective stripe matching exact coat trapezoid slope (no spillover onto arms!)
    t0 = 0.56
    t1 = 0.72
    sx0_a = coat_x0_top + (coat_x0_bot - coat_x0_top) * t0
    sx1_a = coat_x1_top + (coat_x1_bot - coat_x1_top) * t0
    sy_a = coat_y_top + (coat_y_bot - coat_y_top) * t0
    sx0_b = coat_x0_top + (coat_x0_bot - coat_x0_top) * t1
    sx1_b = coat_x1_top + (coat_x1_bot - coat_x1_top) * t1
    sy_b = coat_y_top + (coat_y_bot - coat_y_top) * t1
    c.fill_poly([(sx0_a, sy_a), (sx1_a, sy_a), (sx1_b, sy_b), (sx0_b, sy_b)], PAPER)
    # Cute little flat collar V
    c.fill_poly(
        [(cx - 34 * s + top_dx, sh_y - 12 * s), (cx + 38 * s + top_dx, sh_y - 12 * s), (cx + 4 * s + top_dx, sh_y + 22 * s)],
        COAT_TRIM,
    )

    # Head & helmet
    draw_toro_head(c, head_cx, head_cy, s=s, facing_dx=22.0, hair_color=HAIR_DARK, hair_style="bangs", expr=expr, sweat=sweat)
    draw_helmet(c, head_cx, head_cy, s=s, facing_dx=22.0)

    # Held extinguisher in front of coat
    if holding_tool:
        ext_x = cx + (136.0 if not spraying else 156.0) * s
        ext_y = 668.0 - bob_y * s
        ext_ang = 12.0 if not spraying else -8.0
        draw_extinguisher_prop(c, ext_x, ext_y, s=0.82 * s, angle_deg=ext_ang)

    # Near-side arm in front of coat / gripping extinguisher
    if arm_l is not None:
        draw_arm(c, cx - 52 * s, sh_y + 12 * s, arm_l[0], arm_l[1], COAT_OCHRE, s, arm_l[2], arm_l[3], cuff_color=COAT_TRIM)


# ---------------------------------------------------------------------------
# 1. Character Asset Generators (31 PNGs @ 512x512 RGBA)
# ---------------------------------------------------------------------------
def generate_characters():
    # 1. Firefighter Idle (0 & 1 subtle breathing)
    c = Canvas2x(512, 512)
    draw_firefighter_figure(
        c,
        cx=506,
        bob_y=0.0,
        leg_l_dx=-38,
        leg_r_dx=42,
        arm_l=(406, 720, -14, 10),
        arm_r=(606, 718, 12, 8),
        expr="determined",
    )
    save_canvas(c, os.path.join(CHARS_DIR, "T_Firefighter_Idle_0.png"))

    c = Canvas2x(512, 512)
    draw_firefighter_figure(
        c,
        cx=506,
        bob_y=-6.0,
        leg_l_dx=-38,
        leg_r_dx=42,
        arm_l=(400, 728, -18, 12),
        arm_r=(612, 726, 16, 10),
        expr="idle_blink_soft",
    )
    save_canvas(c, os.path.join(CHARS_DIR, "T_Firefighter_Idle_1.png"))

    # 2. Firefighter Walk 0..3 (scurrying 小碎步)
    walk_frames = [
        # (bob_y, lean, leg_l_dx, leg_l_lift, leg_r_dx, leg_r_lift, arm_l, arm_r)
        (0.0, 5.0, -76, 8, 72, 0, (592, 680, 12, 22), (430, 688, -12, 16)),
        (14.0, 6.0, -16, 38, 18, 0, (526, 712, 8, 18), (496, 712, -8, 16)),
        (0.0, 5.0, 68, 0, -74, 8, (422, 686, -18, 16), (602, 682, 16, 20)),
        (14.0, 6.0, 14, 0, -18, 38, (492, 712, -10, 18), (532, 712, 8, 16)),
    ]
    for i, (by, ln, ldx, llf, rdx, rlf, al, ar) in enumerate(walk_frames):
        c = Canvas2x(512, 512)
        draw_firefighter_figure(c, cx=502, bob_y=by, lean_deg=ln, leg_l_dx=ldx, leg_l_lift=llf, leg_r_dx=rdx, leg_r_lift=rlf, arm_l=al, arm_r=ar, expr="determined")
        save_canvas(c, os.path.join(CHARS_DIR, f"T_Firefighter_Walk_{i}.png"))

    # 3. Firefighter Scared (blocked at door)
    c = Canvas2x(512, 512)
    draw_firefighter_figure(
        c,
        cx=506,
        bob_y=-8.0,
        lean_deg=-5.0,
        leg_l_dx=-46,
        leg_r_dx=46,
        arm_l=(378, 526, -30, 26),
        arm_r=(632, 522, 24, 24),
        expr="scared",
        sweat=True,
    )
    save_canvas(c, os.path.join(CHARS_DIR, "T_Firefighter_Scared.png"))

    # 4. Firefighter Holding Extinguisher Idle & Walk 0..3
    c = Canvas2x(512, 512)
    draw_firefighter_figure(
        c,
        cx=486,
        bob_y=0.0,
        leg_l_dx=-38,
        leg_r_dx=42,
        arm_l=(592, 672, -4, 30),
        arm_r=(622, 616, 12, 22),
        expr="determined",
        holding_tool=True,
    )
    save_canvas(c, os.path.join(CHARS_DIR, "T_Firefighter_Tool_Idle.png"))

    for i, (by, ln, ldx, llf, rdx, rlf, _, _) in enumerate(walk_frames):
        c = Canvas2x(512, 512)
        draw_firefighter_figure(
            c,
            cx=484,
            bob_y=by,
            lean_deg=ln,
            leg_l_dx=ldx,
            leg_l_lift=llf,
            leg_r_dx=rdx,
            leg_r_lift=rlf,
            arm_l=(592, 670 - by, -4, 30),
            arm_r=(622, 614 - by, 12, 22),
            expr="determined",
            holding_tool=True,
        )
        save_canvas(c, os.path.join(CHARS_DIR, f"T_Firefighter_Tool_Walk_{i}.png"))

    # 5. Firefighter Spray 0 & 1 (leaning back, squinting 咬牙囧脸 bracing nozzle forward)
    for i, (recoil_x, recoil_y) in enumerate([(0.0, 0.0), (-8.0, 6.0)]):
        c = Canvas2x(512, 512)
        cx_s = 468 + recoil_x
        draw_firefighter_figure(
            c,
            cx=cx_s,
            bob_y=-10.0 + recoil_y,
            lean_deg=-10.0,
            leg_l_dx=-64,
            leg_r_dx=58,
            arm_l=(cx_s + 114, 670 - recoil_y, -10, 26),
            arm_r=(cx_s + 148, 620 - recoil_y, 8, 22),
            expr="spray",
            holding_tool=True,
            spraying=True,
        )
        # Tiny crisp sky-blue spray droplets bursting right from nozzle tip (binary alpha!)
        tip_x = cx_s + 242
        tip_y = 636 - recoil_y
        c.fill_circle(tip_x + 14 + i * 8, tip_y - 10, 12, SWEAT_BLUE)
        c.fill_circle(tip_x + 34 - i * 4, tip_y + 12, 9, PAPER)
        c.fill_circle(tip_x + 48 + i * 6, tip_y - 2, 7, SWEAT_BLUE)
        save_canvas(c, os.path.join(CHARS_DIR, f"T_Firefighter_Spray_{i}.png"))

    # 6. Combined Escort Uninjured (cradling wide-eyed mint-shirt Civilian in arms!)
    def draw_escort_uninjured(bob_y, lean, ldx, llf, rdx, rlf, civ_bob):
        cv = Canvas2x(512, 512)
        fx = 446.0
        # Firefighter without front arm yet so we layer civilian inside arms
        draw_firefighter_figure(
            cv,
            cx=fx,
            bob_y=bob_y,
            lean_deg=lean,
            leg_l_dx=ldx,
            leg_l_lift=llf,
            leg_r_dx=rdx,
            leg_r_lift=rlf,
            arm_l=None,
            arm_r=(fx + 140, 672 - bob_y, 18, 24),
            expr="determined",
        )
        # Cradled wide-eyed Civilian (curled across firefighter's arms, head right, legs left)
        cy_c = 626.0 - bob_y + civ_bob
        # Dangling legs on left
        draw_leg(cv, fx - 54, cy_c + 26, fx - 120, cy_c + 104, CIV_SHORTS, BOOTS_DARK, s=0.78, knee_bend_x=-14)
        draw_leg(cv, fx - 32, cy_c + 40, fx - 82, cy_c + 122, CIV_SHORTS, BOOTS_DARK, s=0.78, knee_bend_x=-10)
        # Mint-green torso horizontal-tilted
        cv.fill_ellipse(fx + 52, cy_c + 16, 98, 56, CIV_MINT, -14.0)
        # Civilian head (right side, 0.80 scale)
        draw_toro_head(cv, fx + 190, cy_c - 48, s=0.76, facing_dx=12.0, hair_color=HAIR_LIGHT, hair_style="fluffy", expr="civ_help")
        # Civilian arm clinging to firefighter neck
        draw_arm(cv, fx + 74, cy_c - 18, fx + 18, 578 - bob_y, CIV_MINT, s=0.78, elbow_dx=-16, elbow_dy=-14)
        # Firefighter front supporting arm cradling underneath civilian
        draw_arm(cv, fx - 54, 588 - bob_y, fx + 88, cy_c + 44, COAT_OCHRE, s=1.0, elbow_dx=-16, elbow_dy=38)
        return cv

    save_canvas(draw_escort_uninjured(0.0, 2.0, -38, 0, 42, 0, 0.0), os.path.join(CHARS_DIR, "T_Firefighter_EscortUninjured_Idle.png"))
    for i, (by, ln, ldx, llf, rdx, rlf, _, _) in enumerate(walk_frames):
        cb = -6.0 if (i % 2 == 1) else 2.0
        save_canvas(draw_escort_uninjured(by, ln, ldx, llf, rdx, rlf, cb), os.path.join(CHARS_DIR, f"T_Firefighter_EscortUninjured_Walk_{i}.png"))

    # 7. Combined Escort Injured (piggybacking soot-smudged squinting Civilian!)
    def draw_escort_injured(bob_y, lean, ldx, llf, rdx, rlf, civ_bob):
        cv = Canvas2x(512, 512)
        fx = 564.0
        # Piggybacked Civilian behind/on back of firefighter (left side since facing right)
        civ_hx = fx - 196
        civ_hy = 416 - bob_y + civ_bob
        # Civilian mint torso hunched onto firefighter's back
        cv.fill_ellipse(fx - 124, 594 - bob_y + civ_bob, 82, 108, CIV_MINT, 18.0)
        cv.fill_circle(fx - 144, 574 - bob_y + civ_bob, 24, SOOT, inside_only=True)
        cv.fill_circle(fx - 102, 624 - bob_y + civ_bob, 18, SOOT, inside_only=True)
        # Civilian leg wrapped forward around firefighter waist
        draw_leg(cv, fx - 128, 686 - bob_y + civ_bob, fx - 24, 756 - bob_y + civ_bob, CIV_SHORTS, BOOTS_DARK, s=0.78, knee_bend_x=20)
        # Civilian head peeking cleanly left of firefighter's helmet
        draw_toro_head(cv, civ_hx, civ_hy, s=0.76, facing_dx=14.0, hair_color=HAIR_LIGHT, hair_style="fluffy", expr="civ_injured", soot=True, bandage=True)

        # Firefighter in front carrying
        draw_firefighter_figure(
            cv,
            cx=fx,
            bob_y=bob_y,
            lean_deg=lean + 6.0,
            leg_l_dx=ldx,
            leg_l_lift=llf,
            leg_r_dx=rdx,
            leg_r_lift=rlf,
            arm_l=(fx - 70, 718 - bob_y, -28, 22),
            arm_r=(fx + 96, 684 - bob_y, 14, 18),
            expr="determined",
        )
        # Civilian's clinging arm over firefighter's shoulder (with white bandage!)
        draw_arm(cv, fx - 118, 542 - bob_y + civ_bob, fx + 34, 594 - bob_y, CIV_MINT, s=0.78, elbow_dx=-8, elbow_dy=-18)
        cv.fill_round_rect(fx - 34, 562 - bob_y, fx - 6, 588 - bob_y, 6, PAPER)
        return cv

    save_canvas(draw_escort_injured(0.0, 4.0, -38, 0, 42, 0, 0.0), os.path.join(CHARS_DIR, "T_Firefighter_EscortInjured_Idle.png"))
    for i, (by, ln, ldx, llf, rdx, rlf, _, _) in enumerate(walk_frames):
        cb = -5.0 if (i % 2 == 1) else 3.0
        save_canvas(draw_escort_injured(by, ln, ldx, llf, rdx, rlf, cb), os.path.join(CHARS_DIR, f"T_Firefighter_EscortInjured_Walk_{i}.png"))

    # 8. Captain Block 0 & 1 (both tube arms stretched straight out horizontally!)
    for i, (by, arm_y_off) in enumerate([(0.0, 0.0), (-6.0, -8.0)]):
        c = Canvas2x(512, 512)
        cx_c = 512.0
        foot_base = 1024.0
        hip_y = 772.0 - by
        sh_y = 564.0 - by
        head_cy = 384.0 - by

        # Both tube arms stretched straight out horizontally!
        arm_y = sh_y + 22.0 + arm_y_off
        c.fill_capsule(cx_c - 64, arm_y, 92.0, arm_y, 18.0, CAPTAIN_BLUE)
        c.fill_circle(76.0, arm_y - 6.0, 22.0, SKIN)  # Left stop hand
        c.fill_capsule(cx_c + 64, arm_y, 932.0, arm_y, 18.0, CAPTAIN_BLUE)
        c.fill_circle(948.0, arm_y - 6.0, 22.0, SKIN)  # Right stop hand

        # Legs
        draw_leg(c, cx_c - 40, hip_y, cx_c - 54, foot_base, CAPTAIN_DARK, BOOTS_DARK, s=1.02, knee_bend_x=-4)
        draw_leg(c, cx_c + 40, hip_y, cx_c + 54, foot_base, CAPTAIN_DARK, BOOTS_DARK, s=1.02, knee_bend_x=4)

        # Neck & Slate-Blue Uniform Torso (#4F74A6)
        c.fill_capsule(cx_c, head_cy + 100, cx_c, sh_y + 12, 26.0, SKIN)
        uniform_poly = [
            (cx_c - 78, sh_y - 10),
            (cx_c + 78, sh_y - 10),
            (cx_c + 96, hip_y + 24),
            (cx_c - 96, hip_y + 24),
        ]
        c.fill_poly(uniform_poly, CAPTAIN_BLUE)
        # Cream shirt V-collar & crimson/dark tie
        c.fill_poly([(cx_c - 34, sh_y - 10), (cx_c + 34, sh_y - 10), (cx_c, sh_y + 42)], PAPER)
        c.fill_poly([(cx_c - 10, sh_y - 4), (cx_c + 10, sh_y - 4), (cx_c + 14, sh_y + 72), (cx_c, sh_y + 92), (cx_c - 14, sh_y + 72)], HELMET_RED)
        # Dark belt & symmetric gold buckle
        c.fill_rect(cx_c - 100, hip_y - 16, cx_c + 100, hip_y + 6, INK, inside_only=True)
        c.fill_round_rect(cx_c - 20, hip_y - 20, cx_c + 20, hip_y + 10, 6, GOLD_BADGE)

        # Head + Peaked Cap with symmetric gold diamond emblem
        draw_toro_head(c, cx_c, head_cy, s=1.02, facing_dx=0.0, hair_color=HAIR_GREY, hair_style="short_cap", expr="captain_stern")
        cap_y = head_cy - 118
        # Flared peaked cap crown
        crown_pts = [
            (cx_c - 168, cap_y - 54),
            (cx_c, cap_y - 84),
            (cx_c + 168, cap_y - 54),
            (cx_c + 144, cap_y + 34),
            (cx_c - 144, cap_y + 34),
        ]
        c.fill_poly(crown_pts, CAPTAIN_BLUE)
        c.fill_round_rect(cx_c - 148, cap_y + 16, cx_c + 148, cap_y + 42, 8, CAPTAIN_DARK)
        # Glossy dark visor
        c.fill_ellipse(cx_c, cap_y + 46, 134, 18, INK)
        # Symmetric gold diamond emblem on cap
        draw_diamond(c, cx_c, cap_y - 22, 20, 26, GOLD_BADGE)
        save_canvas(c, os.path.join(CHARS_DIR, f"T_Captain_Block_{i}.png"))

    # 9. Civilian Uninjured 0 & 1 (waving arms for help!)
    for i, (by, wave_y, spread_x) in enumerate([(0.0, 290.0, 175.0), (14.0, 385.0, 210.0)]):
        c = Canvas2x(512, 512)
        cx_u = 506.0
        foot_base = 1024.0
        hip_y = 782.0 - by
        sh_y = 592.0 - by
        head_cy = 418.0 - by

        # Waving arms
        draw_arm(c, cx_u + 52, sh_y + 10, cx_u + spread_x, wave_y - by, CIV_MINT, s=0.95, elbow_dx=28, elbow_dy=10)
        draw_arm(c, cx_u - 54, sh_y + 10, cx_u - spread_x, wave_y + 16 - by, CIV_MINT, s=0.95, elbow_dx=-28, elbow_dy=12)

        # Legs
        draw_leg(c, cx_u - 30, hip_y, cx_u - 36, foot_base, CIV_SHORTS, BOOTS_DARK, s=0.95)
        draw_leg(c, cx_u + 30, hip_y, cx_u + 38, foot_base, CIV_SHORTS, BOOTS_DARK, s=0.95)

        # Neck + Mint shirt (#4DC49E)
        c.fill_capsule(cx_u, head_cy + 94, cx_u, sh_y + 10, 24, SKIN)
        shirt_poly = [
            (cx_u - 58, sh_y - 10),
            (cx_u + 60, sh_y - 10),
            (cx_u + 88, hip_y + 20),
            (cx_u - 84, hip_y + 20),
        ]
        c.fill_poly(shirt_poly, CIV_MINT)
        c.fill_poly([(cx_u - 30, sh_y - 10), (cx_u + 32, sh_y - 10), (cx_u + 2, sh_y + 24)], CIV_MINT_TRIM)
        draw_toro_head(c, cx_u, head_cy, s=0.95, facing_dx=18.0, hair_color=HAIR_LIGHT, hair_style="fluffy", expr="civ_help")
        save_canvas(c, os.path.join(CHARS_DIR, f"T_Civilian_Uninjured_{i}.png"))

    # 10. Civilian Injured 0 & 1 (soot smudges, 八字苦瓜眉 crouched holding arm)
    for i, (shiver_x, shiver_y) in enumerate([(0.0, 0.0), (8.0, 10.0)]):
        c = Canvas2x(512, 512)
        cx_i = 496.0 + shiver_x
        foot_base = 1024.0
        hip_y = 816.0 - shiver_y
        sh_y = 648.0 - shiver_y
        head_cx = cx_i + 26.0
        head_cy = 482.0 - shiver_y

        # Crouched bent knees
        draw_leg(c, cx_i - 36, hip_y, cx_i - 48, foot_base, CIV_SHORTS, BOOTS_DARK, s=0.95, knee_bend_x=28)
        draw_leg(c, cx_i + 28, hip_y, cx_i + 44, foot_base, CIV_SHORTS, BOOTS_DARK, s=0.95, knee_bend_x=32)

        # Hunched mint-green shirt + organic soot smudges
        c.fill_capsule(head_cx - 8, head_cy + 88, cx_i + 6, sh_y + 10, 24, SKIN)
        shirt_poly = [
            (cx_i - 54, sh_y - 8),
            (cx_i + 64, sh_y + 4),
            (cx_i + 88, hip_y + 18),
            (cx_i - 82, hip_y + 18),
        ]
        c.fill_poly(shirt_poly, CIV_MINT)
        c.fill_circle(cx_i - 30, sh_y + 72, 22, SOOT, inside_only=True)
        c.fill_circle(cx_i - 12, sh_y + 84, 14, SOOT, inside_only=True)
        c.fill_ellipse(cx_i + 38, sh_y + 132, 28, 16, SOOT, -14, inside_only=True)

        # Injured arm bent across chest + white bandage, other hand holding it
        draw_arm(c, cx_i + 48, sh_y + 16, cx_i + 44, sh_y + 114, CIV_MINT, s=0.92, elbow_dx=32, elbow_dy=6)
        c.fill_round_rect(cx_i + 34, sh_y + 62, cx_i + 72, sh_y + 92, 8, PAPER)
        draw_arm(c, cx_i - 52, sh_y + 12, cx_i + 38, sh_y + 98, CIV_MINT, s=0.92, elbow_dx=-16, elbow_dy=36)

        draw_toro_head(c, head_cx, head_cy, s=0.95, facing_dx=20.0, hair_color=HAIR_LIGHT, hair_style="fluffy", expr="civ_injured", soot=True, bandage=True)
        save_canvas(c, os.path.join(CHARS_DIR, f"T_Civilian_Injured_{i}.png"))

    # 11. Civilian Dead (comedic cartoon face-plant along baseline y=511, zero gore!)
    c = Canvas2x(512, 512)
    foot_base = 1024.0
    # Lying horizontally from left feet (x~120) to right head (x~720), resting flat on y2=1024
    # Left floppy legs on floor
    draw_leg(c, 370, 916, 130, foot_base - 8, CIV_SHORTS, BOOTS_DARK, s=0.95, knee_bend_x=-10)
    draw_leg(c, 376, 960, 165, foot_base, CIV_SHORTS, BOOTS_DARK, s=0.95, knee_bend_x=-8)
    # Flattened mint-shirt torso along floor
    c.fill_ellipse(490, 932, 146, 68, CIV_MINT, -4.0)
    c.fill_circle(455, 912, 26, SOOT, inside_only=True)
    c.fill_circle(540, 956, 22, SOOT, inside_only=True)
    # Back arm splayed up-left
    draw_arm(c, 558, 888, 462, 818, CIV_MINT, s=0.88, elbow_dx=-24, elbow_dy=-12)
    # Head face-planted sideways on baseline right side
    draw_toro_head(c, 742, 892, s=0.86, facing_dx=16.0, hair_color=HAIR_LIGHT, hair_style="fluffy", expr="civ_dead", soot=True)
    # Front arm splayed limp on baseline floor
    draw_arm(c, 582, 954, 650, foot_base - 18, CIV_MINT, s=0.88, elbow_dx=-6, elbow_dy=24)
    save_canvas(c, os.path.join(CHARS_DIR, "T_Civilian_Dead.png"))


# ---------------------------------------------------------------------------
# 2. Interaction Props & VFX Generators (14 PNGs @ 512x512 RGBA)
# ---------------------------------------------------------------------------
def generate_interactions():
    # 1. T_Extinguisher.png (chubby flat-color red extinguisher grounded y=511, zero letters)
    c = Canvas2x(512, 512)
    cx = 442.0
    # Canister centered so canister + hose balance nicely around x=512, resting on y2=1024
    c.fill_round_rect(cx - 122, 974, cx + 122, 1024, 22, EXT_RED)
    c.fill_round_rect(cx - 122, 420, cx + 122, 1016, 86, EXT_RED)
    # Dark rubber base rim at bottom y2=1024
    c.fill_round_rect(cx - 118, 988, cx + 118, 1024, 16, BOOTS_DARK)
    # Cream label band (no letters or numbers!)
    c.fill_rect(cx - 130, 640, cx + 130, 745, PAPER, inside_only=True)
    # Brass neck collar + dark squeeze handles
    c.fill_round_rect(cx - 48, 336, cx + 48, 412, 14, EXT_BRASS)
    c.fill_capsule(cx - 72, 294, cx + 48, 294, 18, INK)
    c.fill_capsule(cx - 56, 250, cx + 68, 274, 14, INK)
    # Pressure gauge
    c.fill_circle(cx + 26, 464, 42, INK)
    c.fill_circle(cx + 26, 464, 30, PAPER)
    c.fill_capsule(cx + 26, 464, cx + 42, 446, 5, EXT_RED)
    # Black hose curving out to the right + flared horn nozzle
    hose_pts = [
        (cx + 42, 360),
        (cx + 150, 378),
        (cx + 220, 480),
        (cx + 228, 630),
        (cx + 286, 684),
    ]
    c.draw_polyline(hose_pts, 32, INK)
    c.fill_poly([(cx + 262, 650), (cx + 354, 624), (cx + 354, 748), (cx + 262, 718)], INK)
    save_canvas(c, os.path.join(INTERACT_DIR, "T_Extinguisher.png"))

    # 2. T_Door_Closed.png & T_Door_Open.png (warm oak paneled door with round brass knob, y=511)
    # Closed door (grounded at y2=1024)
    c = Canvas2x(512, 512)
    c.fill_round_rect(256, 34, 768, 1024, 14, DOOR_FRAME)
    c.fill_rect(288, 66, 736, 1024, DOOR_OAK)
    # Recessed top & bottom oak panels
    c.fill_round_rect(344, 126, 680, 490, 14, DOOR_PANEL)
    c.fill_round_rect(344, 560, 680, 954, 14, DOOR_PANEL)
    # Round brass knob
    c.fill_circle(682, 534, 32, INK)
    c.fill_circle(682, 534, 24, GOLD_BADGE)
    save_canvas(c, os.path.join(INTERACT_DIR, "T_Door_Closed.png"))

    # Open door (swung open frame + foreshortened swung-in door leaf grounded y2=1024)
    c = Canvas2x(512, 512)
    # Outer open doorway jamb & lintel
    c.fill_round_rect(256, 34, 768, 1024, 14, DOOR_FRAME)
    c.fill_rect(288, 66, 736, 1024, (226, 214, 192))  # warm cream interior light inside open frame
    # Swung-open oak door leaf angled inward on right hinges
    leaf_poly = [(736, 66), (546, 118), (546, 984), (736, 1024)]
    c.fill_poly(leaf_poly, DOOR_OAK)
    c.fill_poly([(702, 134), (582, 168), (582, 488), (702, 474)], DOOR_PANEL)
    c.fill_poly([(702, 540), (582, 546), (582, 916), (702, 944)], DOOR_PANEL)
    c.fill_circle(586, 532, 22, INK)
    c.fill_circle(586, 532, 16, GOLD_BADGE)
    save_canvas(c, os.path.join(INTERACT_DIR, "T_Door_Open.png"))

    # 3. T_Fire_Stage1_0..2, Stage2_0..2, Stage3_0..2 (3-frame dancing flames grounded y=511)
    def draw_flame_tongue(cv, tip_x, tip_y, base_x, base_y, width_r, lean, color):
        p_l = (base_x - width_r, base_y)
        p_r = (base_x + width_r, base_y)
        ctrl_l = (base_x - width_r * 1.15 - lean, base_y - (base_y - tip_y) * 0.45)
        ctrl_r = (base_x + width_r * 1.15 - lean, base_y - (base_y - tip_y) * 0.45)
        poly = (
            sample_quad_bezier(p_l, ctrl_l, (tip_x, tip_y), 14)
            + sample_quad_bezier((tip_x, tip_y), ctrl_r, p_r, 14)[1:]
        )
        cv.fill_poly(poly, color)
        cv.fill_ellipse(base_x, base_y, width_r, width_r * 0.38, color)

    stage_configs = [
        # (stage_idx, base_w, main_h, side_h, ember_count)
        (1, 180, 480, 320, 2),
        (2, 260, 660, 460, 3),
        (3, 340, 820, 580, 4),
    ]
    for stage_idx, base_w, main_h, side_h, ember_cnt in stage_configs:
        for f in range(3):
            c = Canvas2x(512, 512)
            cx = 512.0
            base_y = 990.0
            wiggle = (f - 1) * 34.0
            alt_wiggle = ((f + 1) % 3 - 1) * 30.0

            # Outer coral-orange flames (#FF6326)
            draw_flame_tongue(c, cx - base_w * 0.56 + alt_wiggle, base_y - side_h, cx - base_w * 0.34, base_y - 4, base_w * 0.42, alt_wiggle * 0.5, FIRE_CORAL)
            draw_flame_tongue(c, cx + base_w * 0.56 - alt_wiggle, base_y - side_h * 0.92, cx + base_w * 0.34, base_y - 4, base_w * 0.42, -alt_wiggle * 0.5, FIRE_CORAL)
            draw_flame_tongue(c, cx + wiggle, base_y - main_h, cx, base_y - 4, base_w * 0.62, wiggle * 0.6, FIRE_CORAL)
            c.fill_round_rect(cx - base_w * 0.74, 982, cx + base_w * 0.74, 1024, 20, FIRE_CORAL)

            # Mid amber tongues (#FFA62B)
            draw_flame_tongue(c, cx - base_w * 0.36 + alt_wiggle * 0.7, base_y - side_h * 0.74, cx - base_w * 0.22, base_y + 6, base_w * 0.28, alt_wiggle * 0.35, FIRE_AMBER)
            draw_flame_tongue(c, cx + base_w * 0.36 - alt_wiggle * 0.7, base_y - side_h * 0.70, cx + base_w * 0.22, base_y + 6, base_w * 0.26, -alt_wiggle * 0.35, FIRE_AMBER)
            draw_flame_tongue(c, cx + wiggle * 0.8, base_y - main_h * 0.74, cx, base_y + 6, base_w * 0.44, wiggle * 0.45, FIRE_AMBER)
            c.fill_round_rect(cx - base_w * 0.47, 990, cx + base_w * 0.47, 1024, 16, FIRE_AMBER)

            # Inner butter core (#FFE885)
            draw_flame_tongue(c, cx + wiggle * 0.45, base_y - main_h * 0.46, cx, base_y + 12, base_w * 0.25, wiggle * 0.2, FIRE_BUTTER)
            c.fill_round_rect(cx - base_w * 0.24, 996, cx + base_w * 0.24, 1024, 12, FIRE_BUTTER)

            # Cute dancing floating ember circles (kept clear of flame peak & top edge!)
            for e in range(ember_cnt):
                ang = f * 1.15 + e * 2.1
                ex = cx + math.sin(ang) * (base_w * 0.68)
                ey = max(86.0, base_y - main_h * (0.76 + 0.08 * e) - ((f + e) % 3) * 28.0)
                c.fill_circle(ex, ey, 15 + (e % 2) * 5, FIRE_AMBER if (e % 2 == 0) else FIRE_BUTTER)

            save_canvas(c, os.path.join(INTERACT_DIR, f"T_Fire_Stage{stage_idx}_{f}.png"))

    # 4. T_WaterJet_0.png & T_WaterJet_1.png (sky-blue #9ED4EE + cream foam stream shooting right)
    for f in range(2):
        c = Canvas2x(512, 512)
        cy = 512.0
        phase = f * 1.5
        # Undulating sky-blue stream widening from rounded nozzle start (x=48) to splash end (x=848)
        top_pts = []
        bot_pts = []
        steps = 22
        for k in range(steps + 1):
            t = k / steps
            x = 48.0 + t * 800.0
            half_w = 34.0 + t * 116.0 + 14.0 * math.sin(t * 9.0 + phase)
            wave_y = 18.0 * math.sin(t * 8.0 + phase)
            top_pts.append((x, cy + wave_y - half_w))
            bot_pts.append((x, cy + wave_y + half_w))
        c.fill_poly(top_pts + list(reversed(bot_pts)), WATER_DEEP)
        c.fill_circle(48.0, cy + 18.0 * math.sin(phase), 34.0 + 14.0 * math.sin(phase), WATER_DEEP)

        # Brighter sky-blue inner stream (#9ED4EE)
        top_in = []
        bot_in = []
        for k in range(steps + 1):
            t = k / steps
            x = 52.0 + t * 794.0
            half_w = 22.0 + t * 90.0 + 10.0 * math.cos(t * 9.0 + phase)
            wave_y = 16.0 * math.sin(t * 8.0 + phase)
            top_in.append((x, cy + wave_y - half_w))
            bot_in.append((x, cy + wave_y + half_w))
        c.fill_poly(top_in + list(reversed(bot_in)), SWEAT_BLUE)
        c.fill_circle(52.0, cy + 16.0 * math.sin(phase), 24.0, SWEAT_BLUE)

        # Frothy rounded splash cloud at the right end (completely engulfs stream tip top & bottom)
        splash_clouds = [
            (842, cy - 116, 82, SWEAT_BLUE),
            (844, cy + 106, 88, SWEAT_BLUE),
            (876, cy - 22, 86, PAPER),
            (910, cy + 44, 68, SWEAT_BLUE),
            (906, cy - 72, 58, PAPER),
            (948, cy - 18, 38, SWEAT_BLUE),
            (940, cy + 96, 34, PAPER),
            (964, cy - 134, 22, SWEAT_BLUE),
            (968, cy + 154, 20, SWEAT_BLUE),
        ]
        for sx, sy, sr, scol in splash_clouds:
            dy_off = (14 if f == 1 else -10) * (1 if sy < cy else -1)
            c.fill_circle(sx, sy + dy_off, sr, scol)

        # Cream foam puffs (#FFF6E6) inside the stream
        for k in range(6):
            t = 0.14 + k * 0.14
            fx = 54.0 + t * 760.0 + (f * 22.0)
            fy = cy + 16.0 * math.sin(t * 8.0 + phase) + ((k % 2) * 2 - 1) * (14 + k * 5)
            c.fill_circle(fx, fy, 22 + k * 6, PAPER)
        save_canvas(c, os.path.join(INTERACT_DIR, f"T_WaterJet_{f}.png"))


# ---------------------------------------------------------------------------
# 3. Environment Textures (4 PNGs @ 256x256 RGBA, fully opaque)
# ---------------------------------------------------------------------------
def generate_environments():
    # 1. T_Wall_VintageGreen.png (seamless olive-sage vertical stripe wallpaper inspired by example_0)
    c = Canvas2x(256, 256, opaque_bg=(72, 90, 56))  # warm olive-sage base (#485A38)
    stripe_period = 64  # divides 512 cleanly into 8 stripes
    for sx in range(0, 512, stripe_period):
        c.fill_rect(sx, 0, sx + 30, 512, (62, 79, 48))        # subtle alternating stripe
        c.fill_rect(sx + 28, 0, sx + 32, 512, (84, 102, 66))  # soft thin pinstripe
    save_canvas(c, os.path.join(ENV_DIR, "T_Wall_VintageGreen.png"), force_opaque=True)

    # 2. T_Floor_WarmWoven.png (seamless muted warm oatmeal-tan woven mat)
    c = Canvas2x(256, 256, opaque_bg=(160, 139, 110))  # warm oatmeal-tan (#A08B6E)
    tile = 64
    for gy in range(0, 512, tile):
        for gx in range(0, 512, tile):
            alt = ((gx // tile) + (gy // tile)) % 2
            tone = (153, 132, 103) if alt == 0 else (167, 146, 117)
            c.fill_rect(gx + 2, gy + 2, gx + tile - 2, gy + tile - 2, tone)
            # Subtle low-contrast woven ribs
            for rib in range(8, tile, 16):
                if alt == 0:
                    c.fill_rect(gx + 4, gy + rib, gx + tile - 4, gy + rib + 3, (145, 124, 96))
                else:
                    c.fill_rect(gx + rib, gy + 4, gx + rib + 3, gy + tile - 4, (158, 138, 110))
    save_canvas(c, os.path.join(ENV_DIR, "T_Floor_WarmWoven.png"), force_opaque=True)

    # 3. T_Rug_LivingRoom.png (warm cream-terracotta flat geometric border rug)
    c = Canvas2x(256, 256, opaque_bg=(181, 96, 73))  # muted terracotta outer rug edge
    c.fill_round_rect(26, 26, 486, 486, 18, (232, 215, 186))  # warm cream inner field
    c.fill_round_rect(48, 48, 464, 464, 12, (181, 96, 73))   # terracotta inner border line
    c.fill_round_rect(64, 64, 448, 448, 10, (238, 224, 198)) # cream center rug
    # Subtle olive & ochre flat geometric corner diamonds + center medallion
    for cx, cy in [(104, 104), (408, 104), (104, 408), (408, 408)]:
        draw_diamond(c, cx, cy, 22, 22, (104, 122, 82))
        draw_diamond(c, cx, cy, 10, 10, (218, 160, 82))
    draw_diamond(c, 256, 256, 72, 72, (218, 160, 82))
    draw_diamond(c, 256, 256, 48, 48, (181, 96, 73))
    draw_diamond(c, 256, 256, 24, 24, (238, 224, 198))
    save_canvas(c, os.path.join(ENV_DIR, "T_Rug_LivingRoom.png"), force_opaque=True)

    # 4. T_NightWindow.png (deep indigo night sky, crescent moon, stars & warm oak mullions)
    c = Canvas2x(256, 256, opaque_bg=(107, 62, 32))  # warm oak window frame
    c.fill_rect(28, 28, 484, 484, (29, 41, 74))      # deep indigo night sky (#1D294A)
    # Soft dark distant rooftop/horizon along bottom panes
    c.fill_poly([(28, 484), (28, 390), (140, 360), (240, 404), (360, 352), (484, 386), (484, 484)], (20, 29, 54))
    # Crescent moon (butter yellow #FFE885)
    c.fill_circle(168, 146, 48, FIRE_BUTTER)
    c.fill_circle(192, 134, 42, (29, 41, 74))
    # Cute crisp 4-point flat stars
    for sx, sy, sr in [(340, 110, 18), (410, 168, 13), (112, 296, 14), (390, 304, 17), (310, 200, 11)]:
        draw_star4(c, sx, sy, sr, sr * 0.34, PAPER)
    # Warm sage curtain drapes in upper corners (inspired by example_0.png)
    c.fill_poly([(28, 28), (138, 28), (74, 200), (28, 250)], (76, 98, 60))
    c.fill_poly([(484, 28), (374, 28), (438, 200), (484, 250)], (76, 98, 60))
    # Warm oak window cross mullions
    c.fill_rect(244, 28, 268, 484, (107, 62, 32))
    c.fill_rect(28, 244, 484, 268, (107, 62, 32))
    save_canvas(c, os.path.join(ENV_DIR, "T_NightWindow.png"), force_opaque=True)


# ---------------------------------------------------------------------------
# 4. Cute Retro Synthesized Audio SFX (11 WAVs @ 44100 Hz 16-bit Mono PCM)
# ---------------------------------------------------------------------------
SAMPLE_RATE = 44100


def write_wav_mono16(filepath, samples):
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    pcm = [max(-32767, min(32767, int(round(s * 32767.0)))) for s in samples]
    with wave.open(filepath, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(SAMPLE_RATE)
        wf.writeframes(struct.pack("<" + "h" * len(pcm), *pcm))


def _lcg_noise(seed, n):
    """Deterministic PRNG in [-1, 1] without relying on global state."""
    x = seed & 0xFFFFFFFF
    out = []
    for _ in range(n):
        x = (1664525 * x + 1013904223) & 0xFFFFFFFF
        out.append(((x >> 16) / 32767.5) - 1.0)
    return out


def _env_ad(t, dur, atk=0.01):
    if t < atk:
        return t / max(1e-5, atk)
    rem = max(0.0, dur - t)
    return min(1.0, rem / max(1e-5, dur - atk))


def _mix_tone_into(buf, start_sec, dur_sec, f0, f1, amp, wave_type="sine", atk=0.006):
    i0 = max(0, int(start_sec * SAMPLE_RATE))
    n = int(dur_sec * SAMPLE_RATE)
    phase = 0.0
    for i in range(n):
        idx = i0 + i
        if idx >= len(buf):
            break
        t = i / SAMPLE_RATE
        u = t / max(1e-5, dur_sec)
        freq = f0 + (f1 - f0) * u
        phase += 2.0 * math.pi * freq / SAMPLE_RATE
        if wave_type == "sine":
            val = math.sin(phase)
        elif wave_type == "tri":
            val = (2.0 / math.pi) * math.asin(math.sin(phase))
        elif wave_type == "soft_sq":
            val = math.tanh(2.4 * math.sin(phase))
        else:
            val = math.sin(phase)
        env = _env_ad(t, dur_sec, atk)
        buf[idx] += val * amp * (env ** 1.3)


def generate_audio():
    # 1. SFX_PaperOpen.wav — cozy crisp paper rustle + warm journal chord tap
    dur = 0.28
    n = int(dur * SAMPLE_RATE)
    noise = _lcg_noise(101, n)
    buf = [0.0] * n
    lp = 0.0
    for i in range(n):
        t = i / SAMPLE_RATE
        lp = lp * 0.72 + noise[i] * 0.28
        hp = noise[i] - lp
        env = math.sin(math.pi * min(1.0, t / 0.22)) if t < 0.22 else 0.0
        buf[i] += hp * 0.28 * env
    _mix_tone_into(buf, 0.02, 0.14, 392.0, 523.25, 0.22, "tri", 0.008)
    _mix_tone_into(buf, 0.07, 0.18, 523.25, 659.25, 0.18, "sine", 0.008)
    write_wav_mono16(os.path.join(AUDIO_DIR, "SFX_PaperOpen.wav"), buf)

    # 2. SFX_MarkerSwipe.wav — snappy felt-tip marker swoosh across paper
    dur = 0.24
    n = int(dur * SAMPLE_RATE)
    noise = _lcg_noise(202, n)
    buf = [0.0] * n
    lp = 0.0
    for i in range(n):
        t = i / SAMPLE_RATE
        u = t / dur
        alpha = 0.12 + 0.38 * math.sin(math.pi * u)
        lp = lp * (1.0 - alpha) + noise[i] * alpha
        env = math.sin(math.pi * u) ** 0.8
        buf[i] = lp * 0.48 * env
    _mix_tone_into(buf, 0.03, 0.16, 480.0, 880.0, 0.15, "tri", 0.015)
    write_wav_mono16(os.path.join(AUDIO_DIR, "SFX_MarkerSwipe.wav"), buf)

    # 3. SFX_ActionPop.wav — cheerful bouncy two-note pop
    buf = [0.0] * int(0.28 * SAMPLE_RATE)
    _mix_tone_into(buf, 0.00, 0.11, 523.25, 587.33, 0.42, "soft_sq", 0.005)
    _mix_tone_into(buf, 0.07, 0.19, 783.99, 820.00, 0.45, "sine", 0.005)
    write_wav_mono16(os.path.join(AUDIO_DIR, "SFX_ActionPop.wav"), buf)

    # 4. SFX_ThoughtBubble.wav — cute soft upward "bloop-bloop"
    buf = [0.0] * int(0.24 * SAMPLE_RATE)
    _mix_tone_into(buf, 0.00, 0.09, 340.0, 560.0, 0.38, "sine", 0.004)
    _mix_tone_into(buf, 0.08, 0.13, 460.0, 740.0, 0.42, "sine", 0.004)
    write_wav_mono16(os.path.join(AUDIO_DIR, "SFX_ThoughtBubble.wav"), buf)

    # 5. SFX_DoorOpen.wav — latch click + warm wooden swing open chime
    buf = [0.0] * int(0.34 * SAMPLE_RATE)
    _mix_tone_into(buf, 0.00, 0.04, 240.0, 120.0, 0.45, "soft_sq", 0.002)
    _mix_tone_into(buf, 0.05, 0.14, 329.63, 440.00, 0.30, "tri", 0.010)
    _mix_tone_into(buf, 0.12, 0.20, 440.00, 659.25, 0.30, "sine", 0.010)
    write_wav_mono16(os.path.join(AUDIO_DIR, "SFX_DoorOpen.wav"), buf)

    # 6. SFX_PickupTool.wav — bright canister clink + cheerful two-tone pickup
    buf = [0.0] * int(0.28 * SAMPLE_RATE)
    _mix_tone_into(buf, 0.00, 0.08, 659.25, 659.25, 0.36, "tri", 0.003)
    _mix_tone_into(buf, 0.06, 0.20, 880.00, 880.00, 0.42, "sine", 0.004)
    _mix_tone_into(buf, 0.06, 0.14, 1318.5, 1318.5, 0.16, "sine", 0.004)
    write_wav_mono16(os.path.join(AUDIO_DIR, "SFX_PickupTool.wav"), buf)

    # 7. SFX_WaterSpray.wav — airy pressurized water hiss + bubbly foam shimmer
    dur = 0.46
    n = int(dur * SAMPLE_RATE)
    noise = _lcg_noise(707, n)
    buf = [0.0] * n
    lp = 0.0
    for i in range(n):
        t = i / SAMPLE_RATE
        u = t / dur
        lp = lp * 0.65 + noise[i] * 0.35
        shimmer = 0.85 + 0.15 * math.sin(2.0 * math.pi * 16.0 * t)
        env = min(1.0, t / 0.05) * min(1.0, (dur - t) / 0.08)
        buf[i] = lp * 0.32 * shimmer * env
    _mix_tone_into(buf, 0.02, 0.38, 620.0, 540.0, 0.06, "sine", 0.03)
    write_wav_mono16(os.path.join(AUDIO_DIR, "SFX_WaterSpray.wav"), buf)

    # 8. SFX_FireOut.wav — steam "pssshh!" quench + happy relief chime
    dur = 0.44
    n = int(dur * SAMPLE_RATE)
    noise = _lcg_noise(808, n)
    buf = [0.0] * n
    lp = 0.0
    for i in range(n):
        t = i / SAMPLE_RATE
        if t < 0.24:
            lp = lp * 0.70 + noise[i] * 0.30
            env = (1.0 - t / 0.24) ** 1.4
            buf[i] += lp * 0.42 * env
    _mix_tone_into(buf, 0.09, 0.14, 783.99, 783.99, 0.32, "sine", 0.006)
    _mix_tone_into(buf, 0.18, 0.24, 1046.50, 1046.50, 0.38, "sine", 0.006)
    write_wav_mono16(os.path.join(AUDIO_DIR, "SFX_FireOut.wav"), buf)

    # 9. SFX_StarEarn.wav — bright sparkling 4-note arpeggio
    buf = [0.0] * int(0.54 * SAMPLE_RATE)
    notes = [(0.00, 523.25), (0.08, 659.25), (0.16, 783.99), (0.24, 1046.50)]
    for st, freq in notes:
        _mix_tone_into(buf, st, 0.26, freq, freq, 0.32, "sine", 0.005)
        _mix_tone_into(buf, st, 0.14, freq * 2.0, freq * 2.0, 0.10, "tri", 0.005)
    write_wav_mono16(os.path.join(AUDIO_DIR, "SFX_StarEarn.wav"), buf)

    # 10. SFX_StarBurn.wav — comedic sizzle + goofy cartoon drop
    dur = 0.46
    n = int(dur * SAMPLE_RATE)
    noise = _lcg_noise(1010, n)
    buf = [0.0] * n
    lp = 0.0
    for i in range(n):
        t = i / SAMPLE_RATE
        if t < 0.22:
            lp = lp * 0.45 + noise[i] * 0.55
            env = math.sin(math.pi * t / 0.22)
            buf[i] += (noise[i] - lp * 0.5) * 0.28 * env
    _mix_tone_into(buf, 0.08, 0.34, 415.0, 165.0, 0.38, "soft_sq", 0.012)
    write_wav_mono16(os.path.join(AUDIO_DIR, "SFX_StarBurn.wav"), buf)

    # 11. SFX_Footstep.wav — light adorable soft patter tap
    buf = [0.0] * int(0.095 * SAMPLE_RATE)
    _mix_tone_into(buf, 0.00, 0.075, 220.0, 95.0, 0.34, "sine", 0.003)
    noise = _lcg_noise(1111, len(buf))
    for i in range(len(buf)):
        t = i / SAMPLE_RATE
        env = max(0.0, 1.0 - t / 0.045) ** 2.0
        buf[i] += noise[i] * 0.14 * env
    write_wav_mono16(os.path.join(AUDIO_DIR, "SFX_Footstep.wav"), buf)


# ---------------------------------------------------------------------------
# 5. Built-in Output Integrity Verifier
# ---------------------------------------------------------------------------
def read_png_rgba(filepath):
    with open(filepath, "rb") as f:
        data = f.read()
    assert data[:8] == b"\x89PNG\r\n\x1a\n", f"Invalid PNG header: {filepath}"
    pos = 8
    w = h = None
    idat_chunks = []
    while pos < len(data):
        length = struct.unpack(">I", data[pos: pos + 4])[0]
        ctype = data[pos + 4: pos + 8]
        cdata = data[pos + 8: pos + 8 + length]
        pos += 12 + length
        if ctype == b"IHDR":
            w, h, bd, ct, _, _, _ = struct.unpack(">IIBBBBB", cdata)
            assert bd == 8 and ct == 6, f"Expected 8-bit RGBA PNG: {filepath}"
        elif ctype == b"IDAT":
            idat_chunks.append(cdata)
        elif ctype == b"IEND":
            break
    raw = zlib.decompress(b"".join(idat_chunks))
    stride = w * 4
    rgba = bytearray(w * h * 4)
    for y in range(h):
        src = y * (stride + 1)
        assert raw[src] == 0
        rgba[y * stride: (y + 1) * stride] = raw[src + 1: src + 1 + stride]
    return w, h, rgba


def verify_outputs():
    expected_chars = [
        "T_Firefighter_Idle_0.png", "T_Firefighter_Idle_1.png",
        "T_Firefighter_Walk_0.png", "T_Firefighter_Walk_1.png",
        "T_Firefighter_Walk_2.png", "T_Firefighter_Walk_3.png",
        "T_Firefighter_Scared.png",
        "T_Firefighter_Tool_Idle.png",
        "T_Firefighter_Tool_Walk_0.png", "T_Firefighter_Tool_Walk_1.png",
        "T_Firefighter_Tool_Walk_2.png", "T_Firefighter_Tool_Walk_3.png",
        "T_Firefighter_Spray_0.png", "T_Firefighter_Spray_1.png",
        "T_Firefighter_EscortUninjured_Idle.png",
        "T_Firefighter_EscortUninjured_Walk_0.png", "T_Firefighter_EscortUninjured_Walk_1.png",
        "T_Firefighter_EscortUninjured_Walk_2.png", "T_Firefighter_EscortUninjured_Walk_3.png",
        "T_Firefighter_EscortInjured_Idle.png",
        "T_Firefighter_EscortInjured_Walk_0.png", "T_Firefighter_EscortInjured_Walk_1.png",
        "T_Firefighter_EscortInjured_Walk_2.png", "T_Firefighter_EscortInjured_Walk_3.png",
        "T_Captain_Block_0.png", "T_Captain_Block_1.png",
        "T_Civilian_Uninjured_0.png", "T_Civilian_Uninjured_1.png",
        "T_Civilian_Injured_0.png", "T_Civilian_Injured_1.png",
        "T_Civilian_Dead.png",
    ]
    expected_interact = [
        "T_Extinguisher.png",
        "T_Door_Closed.png", "T_Door_Open.png",
        "T_Fire_Stage1_0.png", "T_Fire_Stage1_1.png", "T_Fire_Stage1_2.png",
        "T_Fire_Stage2_0.png", "T_Fire_Stage2_1.png", "T_Fire_Stage2_2.png",
        "T_Fire_Stage3_0.png", "T_Fire_Stage3_1.png", "T_Fire_Stage3_2.png",
        "T_WaterJet_0.png", "T_WaterJet_1.png",
    ]
    expected_env = [
        "T_Wall_VintageGreen.png",
        "T_Floor_WarmWoven.png",
        "T_Rug_LivingRoom.png",
        "T_NightWindow.png",
    ]
    expected_audio = [
        "SFX_PaperOpen.wav", "SFX_MarkerSwipe.wav", "SFX_ActionPop.wav",
        "SFX_ThoughtBubble.wav", "SFX_DoorOpen.wav", "SFX_PickupTool.wav",
        "SFX_WaterSpray.wav", "SFX_FireOut.wav", "SFX_StarEarn.wav",
        "SFX_StarBurn.wav", "SFX_Footstep.wav",
    ]

    for fn in expected_chars:
        path = os.path.join(CHARS_DIR, fn)
        assert os.path.getsize(path) > 0, f"Empty file: {path}"
        w, h, rgba = read_png_rgba(path)
        assert (w, h) == (512, 512), f"Wrong size {w}x{h} on {fn}"
        alphas = set(rgba[3::4])
        assert alphas.issubset({0, 255}) and 255 in alphas, f"Non-binary or empty alpha on {fn}: {alphas}"
        row511 = rgba[511 * 512 * 4 + 3: 512 * 512 * 4: 4]
        assert any(a == 255 for a in row511), f"Sprite not grounded on y=511: {fn}"

    for fn in expected_interact:
        path = os.path.join(INTERACT_DIR, fn)
        assert os.path.getsize(path) > 0, f"Empty file: {path}"
        w, h, rgba = read_png_rgba(path)
        assert (w, h) == (512, 512), f"Wrong size {w}x{h} on {fn}"
        alphas = set(rgba[3::4])
        assert alphas.issubset({0, 255}) and 255 in alphas, f"Non-binary or empty alpha on {fn}: {alphas}"
        if not fn.startswith("T_WaterJet_"):
            row511 = rgba[511 * 512 * 4 + 3: 512 * 512 * 4: 4]
            assert any(a == 255 for a in row511), f"Prop not grounded on y=511: {fn}"

    for fn in expected_env:
        path = os.path.join(ENV_DIR, fn)
        assert os.path.getsize(path) > 0, f"Empty file: {path}"
        w, h, rgba = read_png_rgba(path)
        assert (w, h) == (256, 256), f"Wrong size {w}x{h} on {fn}"
        alphas = set(rgba[3::4])
        assert alphas == {255}, f"Environment texture must be fully opaque: {fn}: {alphas}"

    for fn in expected_audio:
        path = os.path.join(AUDIO_DIR, fn)
        assert os.path.getsize(path) > 44, f"Empty wav: {path}"
        with wave.open(path, "rb") as wf:
            assert wf.getnchannels() == 1
            assert wf.getsampwidth() == 2
            assert wf.getframerate() == 44100
            assert wf.getnframes() > 0

    print(
        f"VERIFIED: {len(expected_chars)} character PNGs, "
        f"{len(expected_interact)} interaction PNGs, "
        f"{len(expected_env)} environment PNGs, "
        f"{len(expected_audio)} audio WAVs (total 60 assets)."
    )


def main():
    generate_characters()
    generate_interactions()
    generate_environments()
    generate_audio()
    verify_outputs()


if __name__ == "__main__":
    main()
