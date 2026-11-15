"""Hand-drawn Crayon Die-Cut Sticker & Audio Asset Pipeline for Persona (Director's Cut) v3.1.

Run with system Python 3.12 (requires Pillow, numpy, scipy):
    python d:\\antigravity_projects\\gamejam\\DirectorCutJam\\Scripts\\process_handdrawn_assets.py

Reads the 5 official hand-drawn artworks in assets/characters/:
  - firefighter.png  -> Pipo (armed, unarmed, scared, spray), Extinguisher, WaterJet
  - witch.png        -> Chief Witch (T_Captain_Block_0..1), T_UI_WitchHatSticker
  - rescue.png       -> Heart-Boxers Guy (Uninjured/Injured/Dead/Rescued), Piggyback Escorts, Fire Stages 1-4
  - little girl.png  -> MatchGirl (T_MatchGirl_Idle_0..1)
  - dog.png          -> Angel Puppy Coco (Idle/Happy), T_UI_AngelDogSticker, T_Portrait_Coco

Also generates hand-drawn die-cut doors (T_Door_Closed, T_Door_Open), cozy dollhouse textures
(T_SafeZone_Mat, T_Porch_Wood, T_Portrait_Coco), and seamless 16s BGM (BGM_StageLoop.wav).
NEVER modifies or deletes the 5 original artist PNGs or existing SFX_*.wav files.
"""

import math
import os
import struct
import wave
from typing import Optional, Tuple

import numpy as np
from PIL import Image, ImageDraw, ImageFilter
from scipy import ndimage


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))
CHARS_DIR = os.path.join(PROJECT_ROOT, "assets", "characters")
ENV_DIR = os.path.join(PROJECT_ROOT, "assets", "envirments")
INTERACT_DIR = os.path.join(PROJECT_ROOT, "assets", "envirments-character-interactions")
AUDIO_DIR = os.path.join(PROJECT_ROOT, "assets", "audio")

CREAM_PAPER = (255, 253, 246)
CHARCOAL_OUTLINE = (58, 50, 44)
INK_WARM = (43, 39, 36)
SKY_BLUE = (126, 211, 234)
BLUSH_PINK = (244, 169, 160)
HEART_PINK = (224, 72, 136)
BUTTER_GOLD = (255, 215, 94)
SOOT_CHAR = (72, 66, 62)

SAMPLE_RATE = 44100


# ---------------------------------------------------------------------------
# Core Masking, Layer Splitting & Die-Cut Sticker Helpers
# ---------------------------------------------------------------------------
def load_rgb(filename: str) -> np.ndarray:
    path = os.path.join(CHARS_DIR, filename)
    return np.array(Image.open(path).convert("RGB"), dtype=np.uint8)


def extract_foreground_bridge(
    rgb: np.ndarray,
    r_bridge: float = 9.0,
    white_min: int = 243,
    sat_max: int = 15,
    extra_strokes_mask: Optional[np.ndarray] = None,
) -> np.ndarray:
    """Border-seeded bridge flood-fill background removal.

    Seals small sketchy breaks (<= 2 * r_bridge px) in hand-drawn charcoal outlines
    before flood-filling from image borders so interior white areas (Witch's cloak,
    Pipo's face/hat, Little Girl's overalls, Coco's belly, thought bubble) stay 100% opaque.
    """
    rgb_i = rgb.astype(np.int16)
    min_c = rgb_i.min(axis=2)
    max_c = rgb_i.max(axis=2)
    is_white = (min_c >= white_min) & ((max_c - min_c) <= sat_max)
    strokes = ~is_white
    if extra_strokes_mask is not None:
        strokes = strokes & extra_strokes_mask

    if not np.any(strokes):
        return np.zeros(rgb.shape[:2], dtype=bool)

    dist_to_stroke = ndimage.distance_transform_edt(~strokes)
    sealed = dist_to_stroke <= r_bridge

    free = ~sealed
    seed = np.zeros(free.shape, dtype=bool)
    seed[0, :] = True
    seed[-1, :] = True
    seed[:, 0] = True
    seed[:, -1] = True
    seed &= free

    labeled, _ = ndimage.label(free, structure=np.ones((3, 3), dtype=np.int32))
    bg_labels = np.unique(labeled[seed])
    bg_labels = bg_labels[bg_labels > 0]
    outer_free = np.isin(labeled, bg_labels)

    dist_from_outer = ndimage.distance_transform_edt(~outer_free)
    solid = (dist_from_outer > (r_bridge - 0.5)) | strokes
    return solid


def filter_small_components(mask: np.ndarray, min_area: int = 25) -> np.ndarray:
    if min_area <= 1 or not np.any(mask):
        return mask
    labeled, num = ndimage.label(mask, structure=np.ones((3, 3), dtype=np.int32))
    if num == 0:
        return mask
    sizes = np.bincount(labeled.ravel())
    keep = sizes >= min_area
    keep[0] = False
    return keep[labeled]


def make_rgba_layer(rgb: np.ndarray, fg_mask: np.ndarray) -> Image.Image:
    """Convert RGB + boolean foreground mask to clean RGBA PIL Image over cream RGB."""
    out = np.zeros((rgb.shape[0], rgb.shape[1], 4), dtype=np.uint8)
    out[:, :, :3] = CREAM_PAPER
    out[fg_mask, :3] = rgb[fg_mask]
    out[fg_mask, 3] = 255
    return Image.fromarray(out, mode="RGBA")


def crop_tight(rgba_img: Image.Image, pad: int = 4) -> Tuple[Image.Image, Tuple[int, int, int, int]]:
    arr = np.array(rgba_img)
    ys, xs = np.where(arr[:, :, 3] > 127)
    if len(xs) == 0:
        return rgba_img, (0, 0, rgba_img.width, rgba_img.height)
    x0 = max(0, int(xs.min()) - pad)
    y0 = max(0, int(ys.min()) - pad)
    x1 = min(rgba_img.width, int(xs.max()) + 1 + pad)
    y1 = min(rgba_img.height, int(ys.max()) + 1 + pad)
    return rgba_img.crop((x0, y0, x1, y1)), (x0, y0, x1, y1)


def apply_die_cut_sticker(
    rgba_canvas: Image.Image,
    paper_px: float = 10.0,
    outline_px: float = 2.0,
    close_radius: int = 3,
    smooth_sigma: float = 1.6,
    min_fg_area: int = 12,
    fill_small_holes_area: int = 0,
) -> Image.Image:
    """Wrap foreground silhouette in ~10px warm cream paper border + ~2px charcoal outline.

    Enforces STRICT binary alpha {0, 255}. Transparent pixels store charcoal RGB so
    UE5 bilinear texture sampling at BLEND_MASKED Clip=0.333 has zero halo.
    """
    arr = np.array(rgba_canvas.convert("RGBA"), dtype=np.uint8)
    alpha_f = arr[:, :, 3].astype(np.float32) / 255.0
    fg_mask = arr[:, :, 3] >= 96
    if min_fg_area > 1:
        fg_mask = filter_small_components(fg_mask, min_area=min_fg_area)

    if not np.any(fg_mask):
        empty = np.zeros_like(arr)
        empty[:, :, :3] = CHARCOAL_OUTLINE
        return Image.fromarray(empty)

    # Smooth outer contour via morphological closing + Gaussian-smoothed distance field
    if close_radius > 0:
        struct_y, struct_x = np.ogrid[-close_radius : close_radius + 1, -close_radius : close_radius + 1]
        disk = (struct_x * struct_x + struct_y * struct_y) <= (close_radius * close_radius)
        fg_closed = ndimage.binary_closing(fg_mask, structure=disk)
    else:
        fg_closed = fg_mask

    dist_raw = ndimage.distance_transform_edt(~fg_mask)
    dist_closed = ndimage.distance_transform_edt(~fg_closed)
    if smooth_sigma > 0.0:
        dist_smooth = ndimage.gaussian_filter(dist_closed.astype(np.float32), sigma=smooth_sigma)
        # Guarantee every foreground pixel keeps at least paper_px margin around it
        dist_eff = np.minimum(dist_raw, dist_smooth)
    else:
        dist_eff = dist_raw

    in_paper = dist_eff <= paper_px
    if fill_small_holes_area > 0:
        filled = ndimage.binary_fill_holes(in_paper)
        holes = filled & (~in_paper)
        lbl_h, nh = ndimage.label(holes)
        if nh > 0:
            areas_h = np.bincount(lbl_h.ravel())
            small_h = np.isin(lbl_h, np.where((areas_h > 0) & (areas_h <= fill_small_holes_area))[0])
            in_paper |= small_h

    in_outline = (~in_paper) & (dist_eff <= (paper_px + outline_px))
    if fill_small_holes_area > 0:
        in_outline &= ~in_paper
    sticker_mask = in_paper | in_outline

    # Composite semi-transparent anti-aliased artwork over warm cream sticker paper
    fg_rgb = arr[:, :, :3].astype(np.float32)
    paper_rgb = np.array(CREAM_PAPER, dtype=np.float32)[None, None, :]
    blended_rgb = np.clip(fg_rgb * alpha_f[:, :, None] + paper_rgb * (1.0 - alpha_f[:, :, None]), 0, 255).astype(
        np.uint8
    )

    out = np.zeros_like(arr)
    out[:, :, :3] = CHARCOAL_OUTLINE
    out[in_paper, :3] = CREAM_PAPER
    out[fg_mask, :3] = blended_rgb[fg_mask]
    out[in_outline, :3] = CHARCOAL_OUTLINE
    out[sticker_mask, 3] = 255
    return Image.fromarray(out)


def save_sticker_png(
    rgba_canvas: Image.Image,
    out_path: str,
    paper_px: float = 10.0,
    outline_px: float = 2.0,
    close_radius: int = 3,
    smooth_sigma: float = 1.6,
    min_fg_area: int = 12,
    snap_bottom_to_edge: bool = True,
    fill_small_holes_area: int = 0,
) -> None:
    """Applies die-cut sticker border and saves strict {0,255} RGBA PNG."""
    sticker = apply_die_cut_sticker(
        rgba_canvas,
        paper_px=paper_px,
        outline_px=outline_px,
        close_radius=close_radius,
        smooth_sigma=smooth_sigma,
        min_fg_area=min_fg_area,
        fill_small_holes_area=fill_small_holes_area,
    )
    arr = np.array(sticker, dtype=np.uint8)
    if snap_bottom_to_edge:
        # Ensure sticker bottom rests right on bottom row (y = H - 1) without floating or clipping
        ys = np.where(arr[:, :, 3] == 255)[0]
        if len(ys) > 0:
            max_y = int(ys.max())
            shift = (arr.shape[0] - 1) - max_y
            if shift != 0:
                shifted = np.zeros_like(arr)
                shifted[:, :, :3] = CHARCOAL_OUTLINE
                if shift > 0:
                    shifted[shift:, :, :] = arr[:-shift, :, :]
                else:
                    sh = -shift
                    shifted[:-sh, :, :] = arr[sh:, :, :]
                arr = shifted
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    Image.fromarray(arr, mode="RGBA").save(out_path, format="PNG")


def place_fg_grounded(
    fg_rgba: Image.Image,
    canvas_size: int = 512,
    target_height: int = 468,
    max_width: int = 472,
    bottom_y: int = 499,
    pivot_src_x: Optional[float] = None,
    dest_center_x: float = 256.0,
    offset_y: float = 0.0,
    angle_deg: float = 0.0,
    scale_x: float = 1.0,
    scale_y: float = 1.0,
) -> Image.Image:
    """Places RGBA foreground cleanly grounded at bottom_y on a transparent canvas."""
    arr = np.array(fg_rgba.convert("RGBA"))
    ys, xs = np.where(arr[:, :, 3] > 96)
    if len(xs) == 0:
        return Image.new("RGBA", (canvas_size, canvas_size), (*CREAM_PAPER, 0))

    x0, x1 = int(xs.min()), int(xs.max()) + 1
    y0, y1 = int(ys.min()), int(ys.max()) + 1
    src_w = max(1, x1 - x0)
    src_h = max(1, y1 - y0)

    if pivot_src_x is None:
        pivot_src_x = 0.5 * (x0 + x1)

    cropped = fg_rgba.crop((x0, y0, x1, y1))
    rel_pivot_x = pivot_src_x - x0

    base_scale = min(target_height / float(src_h), max_width / float(src_w))
    new_w = max(4, int(round(src_w * base_scale * scale_x)))
    new_h = max(4, int(round(src_h * base_scale * scale_y)))
    resized = cropped.resize((new_w, new_h), Image.Resampling.BILINEAR)

    if abs(angle_deg) > 0.05:
        resized = resized.rotate(angle_deg, resample=Image.Resampling.BILINEAR, expand=True)
        # Recalculate tight foreground bounds after rotation
        rarr = np.array(resized)
        rys, rxs = np.where(rarr[:, :, 3] > 96)
        if len(rxs) > 0:
            rx0, rx1 = int(rxs.min()), int(rxs.max()) + 1
            ry0, ry1 = int(rys.min()), int(rys.max()) + 1
            resized = resized.crop((rx0, ry0, rx1, ry1))
            paste_x = int(round(dest_center_x - 0.5 * resized.width))
        else:
            paste_x = int(round(dest_center_x - 0.5 * resized.width))
    else:
        scaled_pivot = rel_pivot_x * (new_w / float(src_w))
        paste_x = int(round(dest_center_x - scaled_pivot))
        # Clamp safely inside canvas with room for die-cut margin
        if paste_x < 14:
            paste_x = 14
        if paste_x + resized.width > canvas_size - 14:
            paste_x = max(14, (canvas_size - 14) - resized.width)

    paste_y = int(round(bottom_y - resized.height + offset_y))
    paste_y = max(14, min(canvas_size - 13 - resized.height, paste_y))

    canvas = Image.new("RGBA", (canvas_size, canvas_size), (*CREAM_PAPER, 0))
    canvas.alpha_composite(resized, dest=(paste_x, paste_y))
    return canvas


def place_fg_centered(
    fg_rgba: Image.Image,
    canvas_size: int = 512,
    max_w: int = 470,
    max_h: int = 470,
    angle_deg: float = 0.0,
    offset_x: float = 0.0,
    offset_y: float = 0.0,
    scale_x: float = 1.0,
    scale_y: float = 1.0,
) -> Image.Image:
    cropped, _ = crop_tight(fg_rgba, pad=0)
    src_w, src_h = max(1, cropped.width), max(1, cropped.height)
    base_scale = min(max_w / float(src_w), max_h / float(src_h))
    new_w = max(4, int(round(src_w * base_scale * scale_x)))
    new_h = max(4, int(round(src_h * base_scale * scale_y)))
    resized = cropped.resize((new_w, new_h), Image.Resampling.BILINEAR)
    if abs(angle_deg) > 0.05:
        resized = resized.rotate(angle_deg, resample=Image.Resampling.BILINEAR, expand=True)
        resized, _ = crop_tight(resized, pad=0)
    px = int(round(0.5 * (canvas_size - resized.width) + offset_x))
    py = int(round(0.5 * (canvas_size - resized.height) + offset_y))
    px = max(13, min(canvas_size - 13 - resized.width, px))
    py = max(13, min(canvas_size - 13 - resized.height, py))
    canvas = Image.new("RGBA", (canvas_size, canvas_size), (*CREAM_PAPER, 0))
    canvas.alpha_composite(resized, dest=(px, py))
    return canvas


def make_walk_deform(
    grounded_rgba: Image.Image,
    frame_idx: int,
    bottom_y: int = 499,
) -> Image.Image:
    """Applies cheerful 4-frame puppet waddle & boot-stride shift while keeping planted foot grounded."""
    # Frame pattern: left step forward, bounce mid, right step forward, bounce mid
    tilts = [2.6, 0.4, -2.6, -0.4]
    hops = [0, -7, 0, -7]
    boot_shifts = [(-7, 6), (0, 0), (7, -6), (0, 0)]

    arr = np.array(grounded_rgba.convert("RGBA"), dtype=np.uint8)
    h, w = arr.shape[:2]
    # Split legs gently in bottom 16% of character for cute boot stride
    split_y = int(bottom_y - 68)
    upper = Image.fromarray(arr[:split_y, :, :], mode="RGBA")
    lower = arr[split_y:, :, :].copy()

    # Shift left/right halves of lower boot strip subtly on stride frames
    lx_sh, rx_sh = boot_shifts[frame_idx % 4]
    new_lower = np.zeros_like(lower)
    new_lower[:, :, :3] = CREAM_PAPER
    mid_x = w // 2
    # Left foot strip
    l_lift = -5 if lx_sh > 0 else 0
    r_lift = -5 if rx_sh > 0 else 0

    def blit_half(src_slice: np.ndarray, dx: int, dy: int, x_start: int, x_end: int) -> None:
        lh = src_slice.shape[0]
        for yy in range(lh):
            ny = yy + dy
            if 0 <= ny < lh:
                for xx in range(x_start, x_end):
                    nx = xx + dx
                    if 0 <= nx < w and src_slice[yy, xx, 3] > 96:
                        new_lower[ny, nx] = src_slice[yy, xx]

    blit_half(lower, lx_sh, l_lift, 0, mid_x)
    blit_half(lower, rx_sh, r_lift, mid_x, w)

    composed = Image.new("RGBA", (w, h), (*CREAM_PAPER, 0))
    composed.alpha_composite(upper, dest=(0, 0))
    composed.alpha_composite(Image.fromarray(new_lower, mode="RGBA"), dest=(0, split_y))

    # Apply lively hop & tilt around bottom-center
    tilt = tilts[frame_idx % 4]
    hop = hops[frame_idx % 4]
    rot = composed.rotate(tilt, resample=Image.Resampling.BILINEAR, center=(256, bottom_y))

    # Shift upward by hop only on the hop frames, keeping bottom grounded on planted frames
    out = Image.new("RGBA", (w, h), (*CREAM_PAPER, 0))
    out.alpha_composite(rot, dest=(0, hop))
    return out


# ---------------------------------------------------------------------------
# Hand-Drawn Accent Drawing Helpers (Crayon / Sketchbook Style)
# ---------------------------------------------------------------------------
def draw_crayon_sweat_and_shock(canvas: Image.Image) -> Image.Image:
    """Adds comic sky-blue sweat teardrops & charcoal shock ticks for T_Firefighter_Scared."""
    out = canvas.copy()
    draw = ImageDraw.Draw(out)
    # Two teardrops near upper-left & upper-right of hat/cheek
    drops = [(126, 174, 1.0), (104, 218, 0.78)]
    for cx, cy, sc in drops:
        r = 11 * sc
        pts = [
            (cx, cy - 18 * sc),
            (cx + r * 0.9, cy + 2 * sc),
            (cx + r * 0.6, cy + 12 * sc),
            (cx, cy + 15 * sc),
            (cx - r * 0.6, cy + 12 * sc),
            (cx - r * 0.9, cy + 2 * sc),
        ]
        draw.polygon(pts, fill=(*SKY_BLUE, 255), outline=(*CHARCOAL_OUTLINE, 255), width=3)
        draw.ellipse([cx - 3 * sc, cy + 1 * sc, cx + 1 * sc, cy + 7 * sc], fill=(255, 255, 255, 255))

    # Three startled shock-wave ticks radiating top-right of hat (no letters!)
    ticks = [
        [(372, 108), (398, 86)],
        [(386, 134), (418, 126)],
        [(348, 84), (362, 56)],
    ]
    for p0, p1 in ticks:
        draw.line([p0, p1], fill=(*CHARCOAL_OUTLINE, 255), width=5)
    return out


def draw_soot_and_cough(canvas: Image.Image, soot_spots, cough_origin, frame_idx: int) -> Image.Image:
    """Applies soft organic charcoal crayon soot smudges and sketchy cough puffs."""
    out = canvas.copy()
    arr = np.array(out, dtype=np.uint8)
    fg = arr[:, :, 3] > 128

    yy, xx = np.mgrid[0:512, 0:512]
    rng = np.random.default_rng(42 + frame_idx)
    fine_tooth = rng.random((512, 512))
    coarse_cloud = ndimage.gaussian_filter(rng.normal(0.0, 1.0, (512, 512)), sigma=3.5)
    # Soft organic charcoal smudges (no crisp geometric ellipse edge)
    soot_blend = np.zeros((512, 512), dtype=np.float32)
    for cx, cy, rx, ry in soot_spots:
        d = np.sqrt(((xx - cx) / max(1.0, rx)) ** 2 + ((yy - cy) / max(1.0, ry)) ** 2)
        organic_d = d + coarse_cloud * 0.22
        smudge = np.clip((1.05 - organic_d) * 0.85, 0.0, 0.72) * (0.45 + 0.55 * fine_tooth)
        soot_blend = np.maximum(soot_blend, smudge)

    mask = fg & (soot_blend > 0.04)
    soot_col = np.array(SOOT_CHAR, dtype=np.float32)
    arr[mask, :3] = np.clip(
        arr[mask, :3].astype(np.float32) * (1.0 - soot_blend[mask, None]) + soot_col * soot_blend[mask, None],
        0,
        255,
    ).astype(np.uint8)

    out = Image.fromarray(arr)
    draw = ImageDraw.Draw(out)

    # Sketchy crayon-style cough cloud puffs near mouth (only when cough_origin is on-canvas)
    ox, oy = cough_origin
    if 0 <= ox < 512 and 0 <= oy < 512:
        drift = frame_idx * 7
        puffs = [
            (ox + 16 + drift, oy - 10 - drift // 2, 13),
            (ox + 38 + int(drift * 1.3), oy - 24 - drift, 10),
            (ox + 52 + int(drift * 1.5), oy - 13 - drift, 7),
        ]
        for k, (px, py, pr) in enumerate(puffs):
            # Inner soft grey fill + loose double charcoal pencil rings
            draw.ellipse([px - pr + 1, py - pr + 1, px + pr - 1, py + pr - 1], fill=(216, 210, 204, 255))
            draw.ellipse([px - pr, py - pr, px + pr, py + pr], outline=(112, 104, 98, 255), width=2)
            draw.ellipse(
                [px - pr - 1 + (k % 2), py - pr + 1, px + pr + 1, py + pr + (k % 2)],
                outline=(82, 76, 72, 255),
                width=2,
            )
    return out


def draw_happy_sparkles(canvas: Image.Image, spots, hearts=None) -> Image.Image:
    out = canvas.copy()
    draw = ImageDraw.Draw(out)
    for sx, sy, sr, col in spots:
        pts = []
        for k in range(8):
            ang = k * math.pi / 4.0
            rad = sr if k % 2 == 0 else sr * 0.36
            pts.append((sx + math.cos(ang) * rad, sy + math.sin(ang) * rad))
        draw.polygon(pts, fill=(*col, 255), outline=(*CHARCOAL_OUTLINE, 255), width=2)
    if hearts:
        for hx, hy, hs in hearts:
            draw.ellipse([hx - hs, hy - hs * 0.6, hx, hy + hs * 0.4], fill=(*HEART_PINK, 255))
            draw.ellipse([hx, hy - hs * 0.6, hx + hs, hy + hs * 0.4], fill=(*HEART_PINK, 255))
            draw.polygon([(hx - hs, hy), (hx + hs, hy), (hx, hy + hs * 1.2)], fill=(*HEART_PINK, 255))
    return out


def draw_dizzy_swirls(canvas: Image.Image, cx: int, cy: int) -> Image.Image:
    out = canvas.copy()
    draw = ImageDraw.Draw(out)
    for ox, oy, col in [(cx - 28, cy, BUTTER_GOLD), (cx + 22, cy - 12, SKY_BLUE)]:
        pts = []
        for t_i in range(42):
            ang = t_i * 0.35
            r = 2.0 + t_i * 0.42
            pts.append((ox + math.cos(ang) * r, oy + math.sin(ang) * r * 0.65))
        draw.line(pts, fill=(*CHARCOAL_OUTLINE, 255), width=4)
        draw.line([(p[0] + 1, p[1] - 1) for p in pts], fill=(*col, 255), width=2)
    return out


# ---------------------------------------------------------------------------
# 1. Firefighter Pipeline (firefighter.png)
# ---------------------------------------------------------------------------
def _apply_crayon_fill(rgba_img: Image.Image, mask: np.ndarray, base_rgb: Tuple[int, int, int], seed: int = 7) -> Image.Image:
    """Applies organic wax-crayon grain & subtle hatching inside an RGBA region."""
    arr = np.array(rgba_img, dtype=np.uint8)
    h, w = arr.shape[:2]
    yy, xx = np.mgrid[0:h, 0:w]
    rng = np.random.default_rng(seed)
    tooth = rng.normal(0.0, 14.0, (h, w))
    streak = ndimage.gaussian_filter(rng.normal(0.0, 22.0, (h, w)), sigma=(1.2, 4.5))
    hatch = np.sin((xx * 0.45 + yy * 0.65)) * 7.5
    col = np.array(base_rgb, dtype=np.float32)[None, None, :] + (tooth + streak + hatch)[:, :, None]
    arr[mask, :3] = np.clip(col[mask], 0, 255).astype(np.uint8)
    arr[mask, 3] = 255
    return Image.fromarray(arr)


def _draw_crayon_extinguisher(size: int = 512) -> Image.Image:
    """Draws a crisp hand-drawn crayon red fire extinguisher matching firefighter.png's style."""
    cv = Image.new("RGBA", (size, size), (*CREAM_PAPER, 0))
    draw = ImageDraw.Draw(cv)
    rng = np.random.default_rng(314)

    # 1. Rounded tall crayon red cylinder tank (positioned slightly left so curled hose balances center)
    cx = 196
    tx0, ty0, tx1, ty1 = cx - 72, 168, cx + 72, 490
    # Inner cream/red wax base mask
    mask_img = Image.new("L", (size, size), 0)
    mdraw = ImageDraw.Draw(mask_img)
    mdraw.rounded_rectangle([tx0, ty0, tx1, ty1], radius=52, fill=255)
    body_mask = np.array(mask_img) > 128

    arr = np.array(cv, dtype=np.uint8)
    yy, xx = np.mgrid[0:size, 0:size]
    wax_noise = ndimage.gaussian_filter(rng.normal(0.0, 18.0, (size, size)), sigma=(1.6, 3.8))
    tooth = rng.normal(0.0, 11.0, (size, size))
    # Subtle lighter specular highlight stripe on left & warm deep red shadow on right
    shade = np.where((xx >= cx - 44) & (xx <= cx - 18), 32.0, np.where(xx > cx + 22, -24.0, 0.0))
    red_base = np.array([218.0, 74.0, 56.0])[None, None, :] + (wax_noise + tooth + shade)[:, :, None]
    arr[body_mask, :3] = np.clip(red_base[body_mask], 0, 255).astype(np.uint8)
    arr[body_mask, 3] = 255

    # Horizontal glossy crayon scuff bands (matching the 2 horizontal stripes in firefighter.png)
    for by in (292, 378):
        band = body_mask & (np.abs(yy - (by + 4.5 * np.sin((xx - cx) * 0.03))) <= 8.5)
        arr[band, :3] = np.clip(np.array([76.0, 70.0, 68.0])[None, None, :] + tooth[band, None] * 1.4, 0, 255).astype(np.uint8)

    cv = Image.fromarray(arr)
    draw = ImageDraw.Draw(cv)

    # Sketchy double-pass crimson-charcoal contour around canister
    for off_x, off_y, col, wid in [
        (0, 0, (186, 48, 36, 255), 8),
        (2, -2, (82, 54, 48, 255), 4),
    ]:
        draw.rounded_rectangle([tx0 + off_x, ty0 + off_y, tx1 + off_x, ty1 + off_y], radius=52, outline=col, width=wid)

    # 2. Charcoal top valve neck, pressure gauge & squeeze trigger lever
    neck_pts = [(cx - 26, 174), (cx - 22, 128), (cx + 22, 128), (cx + 26, 174)]
    draw.polygon(neck_pts, fill=(92, 88, 84, 255), outline=(58, 52, 48, 255), width=5)
    # Top squeeze handle & lower grip handle (sketchy charcoal double strokes)
    draw.line([(cx - 46, 102), (cx + 18, 92), (cx + 76, 118)], fill=(68, 64, 60, 255), width=11)
    draw.line([(cx + 12, 122), (cx + 68, 132)], fill=(76, 72, 68, 255), width=9)
    # Tiny cream/yellow pressure dial right above neck
    draw.ellipse([cx - 38, 122, cx - 8, 152], fill=(255, 246, 220, 255), outline=(64, 58, 54, 255), width=4)
    draw.line([(cx - 23, 137), (cx - 14, 129)], fill=(214, 68, 52, 255), width=3)

    # 3. Expressive curled dark charcoal crayon hose sweeping right + flared nozzle cone
    hose_pts = [
        (cx + 24, 152),
        (cx + 88, 148),
        (cx + 148, 178),
        (cx + 176, 240),
        (cx + 168, 314),
        (cx + 142, 376),
        (cx + 152, 434),
        (cx + 188, 468),
    ]
    draw.line(hose_pts, fill=(78, 74, 70, 255), width=18, joint="curve")
    draw.line([(x + 2, y - 2) for x, y in hose_pts], fill=(108, 104, 100, 255), width=7, joint="curve")
    # Flared nozzle horn at end of hose
    nozzle = [(cx + 178, 456), (cx + 216, 466), (cx + 214, 492), (cx + 176, 480)]
    draw.polygon(nozzle, fill=(64, 60, 56, 255), outline=(48, 44, 40, 255), width=3)
    return cv


def build_firefighter_assets() -> Tuple[Image.Image, float]:
    rgb = load_rgb("firefighter.png")
    h, w = rgb.shape[:2]
    yy, xx = np.mgrid[0:h, 0:w]
    rgb_i = rgb.astype(np.int16)

    # Split cyan water splash cleanly from Pipo:
    # Cyan splash has high G & B relative to R, located right of nozzle (x >= 302)
    is_cyan_splash = (
        (rgb_i[:, :, 2] - rgb_i[:, :, 0] > 24)
        & (rgb_i[:, :, 1] - rgb_i[:, :, 0] > 16)
        & (rgb_i[:, :, 1] > 165)
        & (rgb_i[:, :, 2] > 180)
        & (xx >= 302)
    )
    is_cyan_splash = filter_small_components(is_cyan_splash, min_area=14)

    # WaterJet extraction: dense core splash + primary spray droplets (excluding tiny far edge dust)
    splash_region = is_cyan_splash & (xx >= 302) & (xx <= 850) & (yy >= 195) & (yy <= 860)
    splash_region = filter_small_components(splash_region, min_area=18)
    water_rgba = make_rgba_layer(rgb, splash_region)
    water_tight, _ = crop_tight(water_rgba, pad=2)

    # Frame 0 & Frame 1 WaterJet (512x512 horizontal blast shooting RIGHT)
    wj0 = place_fg_centered(water_tight, 512, max_w=465, max_h=455, offset_x=4)
    save_sticker_png(
        wj0,
        os.path.join(INTERACT_DIR, "T_WaterJet_0.png"),
        paper_px=9.0,
        outline_px=2.0,
        min_fg_area=10,
        snap_bottom_to_edge=False,
    )
    wj1 = place_fg_centered(water_tight, 512, max_w=474, max_h=466, offset_x=8, offset_y=-4, scale_x=1.05, scale_y=0.96)
    save_sticker_png(
        wj1,
        os.path.join(INTERACT_DIR, "T_WaterJet_1.png"),
        paper_px=9.0,
        outline_px=2.0,
        min_fg_area=10,
        snap_bottom_to_edge=False,
    )

    # Extract Armed Pipo (exclude cyan water splash and ghost gap behind right shoulder x > 306 for y in 402..500)
    shoulder_trim = (yy >= 402) & (yy <= 505) & (xx > 306)
    pipo_stroke_mask = (~is_cyan_splash) & (xx <= 415) & (~shoulder_trim)
    pipo_fg = extract_foreground_bridge(rgb, r_bridge=9.0, extra_strokes_mask=pipo_stroke_mask)
    pipo_fg &= (~is_cyan_splash) & (xx <= 415) & (~shoulder_trim)
    pipo_fg = filter_small_components(pipo_fg, min_area=80)

    # Trim any residual pale cyan transition pixels right next to yellow arm (x >= 298, y in 408..560)
    pale_cyan_edge = (
        (xx >= 298)
        & (yy >= 408)
        & (yy <= 560)
        & (rgb_i[:, :, 2] - rgb_i[:, :, 0] > 12)
        & (rgb_i[:, :, 1] > 180)
    )
    pipo_fg &= ~pale_cyan_edge

    pipo_armed_rgba = make_rgba_layer(rgb, pipo_fg)
    # Boots horizontal midpoint in firefighter.png is around x = 252
    pipo_pivot_x = 252.0

    # 1. T_Firefighter_Tool_Idle.png & Tool_Walk_0..3.png
    armed_idle = place_fg_grounded(
        pipo_armed_rgba, 512, target_height=468, max_width=420, bottom_y=499, pivot_src_x=pipo_pivot_x
    )
    save_sticker_png(armed_idle, os.path.join(CHARS_DIR, "T_Firefighter_Tool_Idle.png"))

    for wi in range(4):
        wf = make_walk_deform(armed_idle, wi, bottom_y=499)
        save_sticker_png(wf, os.path.join(CHARS_DIR, f"T_Firefighter_Tool_Walk_{wi}.png"))

    # 2. T_Firefighter_Spray_0.png & Spray_1.png (recoil lean + pure cyan spray jet at nozzle!)
    nozzle_cyan = is_cyan_splash & (xx >= 305) & (xx <= 465) & (yy >= 340) & (yy <= 575)
    nozzle_cyan = filter_small_components(nozzle_cyan, min_area=28)
    spray_tuft_full = make_rgba_layer(rgb, nozzle_cyan)
    spray_tuft, _ = crop_tight(spray_tuft_full, pad=0)
    for si, (tilt, dx_shift, tuft_scale) in enumerate([(-2.5, -8, 0.44), (-4.2, -12, 0.52)]):
        spr = place_fg_grounded(
            pipo_armed_rgba,
            512,
            target_height=462,
            max_width=400,
            bottom_y=499,
            pivot_src_x=pipo_pivot_x,
            dest_center_x=238.0 + dx_shift,
            angle_deg=tilt,
        )
        tw = max(12, int(round(spray_tuft.width * tuft_scale)))
        th = max(12, int(round(spray_tuft.height * tuft_scale)))
        tuft_rs = spray_tuft.resize((tw, th), Image.Resampling.BILINEAR)
        spr.alpha_composite(tuft_rs, dest=(280 + si * 5, 294 - si * 4))
        save_sticker_png(spr, os.path.join(CHARS_DIR, f"T_Firefighter_Spray_{si}.png"))

    # 3. Hand-Drawn Crayon Red Extinguisher (T_Extinguisher.png)
    ext_canvas = _draw_crayon_extinguisher(512)
    save_sticker_png(ext_canvas, os.path.join(INTERACT_DIR, "T_Extinguisher.png"))

    # 4. Unarmed Pipo (cleanly remove front extinguisher, trailing hose & yellow nozzle/arm!)
    unarmed_rgb = rgb.copy()
    unarmed_fg = pipo_fg.copy()

    # Erase trailing black hose on left (x < 194, y in 490..675)
    hose_erase = (xx < 194) & (yy >= 490) & (yy <= 675)
    unarmed_fg &= ~hose_erase

    # Erase extinguisher red outlines, valve, grey bars, and yellow sleeve across torso
    ext_erase_box = (xx >= 186) & (xx <= 310) & (yy >= 412) & (yy <= 545)
    is_red_or_orange = (rgb_i[:, :, 0] > 145) & (rgb_i[:, :, 0] - rgb_i[:, :, 2] > 28)
    is_yellow = (rgb_i[:, :, 0] > 185) & (rgb_i[:, :, 1] > 170) & (rgb_i[:, :, 2] < 150)
    inner_can = (xx >= 188) & (xx <= 268) & (yy >= 414) & (yy <= 532)
    preserve_navy = (rgb_i[:, :, 2] > rgb_i[:, :, 0] + 18) & (rgb_i[:, :, 0] < 95)
    erase_pixels = ext_erase_box & (~preserve_navy) & (is_red_or_orange | is_yellow | inner_can)
    unarmed_rgb[erase_pixels] = CREAM_PAPER

    # Repaint Pipo's navy overall apron with authentic crayon wax texture + sketchy charcoal arms
    unarmed_rgba = make_rgba_layer(unarmed_rgb, unarmed_fg)
    # Build crayon-textured apron mask over chest bib
    apron_layer = Image.new("L", (w, h), 0)
    adraw = ImageDraw.Draw(apron_layer)
    adraw.polygon([(205, 460), (275, 462), (274, 540), (218, 558), (200, 522)], fill=255)
    adraw.line([(208, 415), (216, 468)], fill=255, width=15)
    adraw.line([(286, 416), (274, 468)], fill=255, width=15)
    apron_mask = np.array(apron_layer) > 128

    # First restore crisp paper-cream upper shirt above apron bib
    udraw = ImageDraw.Draw(unarmed_rgba)
    udraw.polygon([(206, 416), (288, 416), (276, 464), (210, 464)], fill=(*CREAM_PAPER, 255))
    # Now fill apron with textured navy crayon matching Pipo's original blue crayon shorts
    unarmed_rgba = _apply_crayon_fill(unarmed_rgba, apron_mask, (28, 52, 114), seed=77)
    udraw = ImageDraw.Draw(unarmed_rgba)

    # Multi-pass sketchy charcoal arms & round crayon mittens (instead of rigid thin vectors)
    for pts_arm, mx, my in [
        ([(196, 422), (176, 476), (186, 514)], 186, 515),
        ([(296, 422), (314, 478), (304, 514)], 304, 515),
    ]:
        udraw.line(pts_arm, fill=(84, 80, 76, 255), width=9, joint="curve")
        udraw.line([(p[0] + 2, p[1] - 1) for p in pts_arm], fill=(58, 55, 52, 255), width=5, joint="curve")
        udraw.ellipse([mx - 10, my - 10, mx + 10, my + 10], fill=(*CREAM_PAPER, 255), outline=(66, 62, 58, 255), width=5)

    # Clean up mask around newly drawn arms
    u_arr = np.array(unarmed_rgba)
    u_mask = u_arr[:, :, 3] > 128
    u_mask = filter_small_components(u_mask, min_area=80)
    unarmed_rgba = Image.fromarray(np.where(u_mask[:, :, None], u_arr, np.array([*CREAM_PAPER, 0], dtype=np.uint8)))

    unarmed_idle0 = place_fg_grounded(
        unarmed_rgba, 512, target_height=468, max_width=400, bottom_y=499, pivot_src_x=pipo_pivot_x
    )
    save_sticker_png(unarmed_idle0, os.path.join(CHARS_DIR, "T_Firefighter_Idle_0.png"))

    unarmed_idle1 = place_fg_grounded(
        unarmed_rgba, 512, target_height=474, max_width=405, bottom_y=499, pivot_src_x=pipo_pivot_x, scale_x=1.02, scale_y=0.98
    )
    save_sticker_png(unarmed_idle1, os.path.join(CHARS_DIR, "T_Firefighter_Idle_1.png"))

    for wi in range(4):
        wf = make_walk_deform(unarmed_idle0, wi, bottom_y=499)
        save_sticker_png(wf, os.path.join(CHARS_DIR, f"T_Firefighter_Walk_{wi}.png"))

    # Scared Pipo
    scared_base = place_fg_grounded(
        unarmed_rgba, 512, target_height=464, max_width=395, bottom_y=499, pivot_src_x=pipo_pivot_x, angle_deg=-4.0
    )
    scared_full = draw_crayon_sweat_and_shock(scared_base)
    save_sticker_png(scared_full, os.path.join(CHARS_DIR, "T_Firefighter_Scared.png"))

    return unarmed_rgba, pipo_pivot_x


# ---------------------------------------------------------------------------
# 2. Witch Captain Pipeline (witch.png)
# ---------------------------------------------------------------------------
def build_witch_assets() -> None:
    rgb = load_rgb("witch.png")
    fg = extract_foreground_bridge(rgb, r_bridge=11.0)
    fg = filter_small_components(fg, min_area=60)
    witch_rgba = make_rgba_layer(rgb, fg)

    # Center horizontally around her two stick legs / hat (x ~ 370)
    witch_pivot_x = 372.0
    cap0 = place_fg_grounded(witch_rgba, 512, target_height=470, max_width=440, bottom_y=499, pivot_src_x=witch_pivot_x)
    save_sticker_png(cap0, os.path.join(CHARS_DIR, "T_Captain_Block_0.png"))

    cap1_base = place_fg_grounded(
        witch_rgba,
        512,
        target_height=476,
        max_width=444,
        bottom_y=499,
        pivot_src_x=witch_pivot_x,
        angle_deg=1.4,
        scale_x=1.015,
        scale_y=0.985,
    )
    cap1 = draw_happy_sparkles(cap1_base, [(74, 184, 13, BUTTER_GOLD), (112, 152, 8, SKY_BLUE)])
    save_sticker_png(cap1, os.path.join(CHARS_DIR, "T_Captain_Block_1.png"))

    # T_UI_WitchHatSticker.png (256x256 centered die-cut purple pointed witch hat ONLY — no braid/bangs/cloak/face)
    h, w = rgb.shape[:2]
    yy, xx = np.mgrid[0:h, 0:w]
    rgb_i = rgb.astype(np.int16)
    is_purple_hat = (
        (rgb_i[:, :, 2] - rgb_i[:, :, 1] > 14)
        & (rgb_i[:, :, 2] >= rgb_i[:, :, 0] - 8)
        & (rgb_i[:, :, 0] > 85)
        & (rgb_i[:, :, 2] > 110)
        & (xx >= 145)
        & (xx <= 430)
        & (yy >= 140)
        & (yy <= 462)
    )
    # Strictly exclude black bangs/eyes/face ink, head circle grey strokes on right, yellow braid, & green collar
    is_yellow_hair = (rgb_i[:, :, 0] - rgb_i[:, :, 2] > 24) & (rgb_i[:, :, 0] > 180)
    is_green_collar = (rgb_i[:, :, 1] - rgb_i[:, :, 0] > 10) & (rgb_i[:, :, 1] > 140)
    is_dark_ink = (rgb_i.min(axis=2) < 115) & ((rgb_i[:, :, 2] - rgb_i[:, :, 1]) < 22)
    in_face_or_bangs = (xx >= 265) & (yy >= 242)
    in_head_circle_right = (xx >= 355) & (yy >= 165) & (~is_purple_hat)
    is_hat_grey = (rgb_i.min(axis=2) < 230) & (rgb_i.min(axis=2) >= 115) & (xx <= 425) & (yy <= 456)
    near_purple = ndimage.binary_dilation(is_purple_hat, iterations=14)

    hat_strokes = is_purple_hat | (
        is_hat_grey & near_purple & (~in_face_or_bangs) & (~in_head_circle_right)
    )
    hat_strokes &= (~is_yellow_hair) & (~is_green_collar) & (~is_dark_ink)

    # Bridge small pencil crown gap & fill hat interior paper cleanly
    hat_bridge = ndimage.binary_closing(hat_strokes, structure=np.ones((7, 7), dtype=bool))
    hat_mask = ndimage.binary_fill_holes(hat_bridge) | hat_strokes
    hat_mask &= (~is_yellow_hair) & (~is_green_collar) & (~is_dark_ink) & (~(in_face_or_bangs & (~is_purple_hat)))
    # Keep only the main connected witch hat silhouette (drops isolated stray specks)
    lbl_h, nh = ndimage.label(hat_mask)
    if nh > 0:
        areas_h = np.bincount(lbl_h.ravel())
        hat_mask = lbl_h == (1 + int(np.argmax(areas_h[1:])))
    hat_rgba = make_rgba_layer(rgb, hat_mask)
    hat_sticker_in = place_fg_centered(hat_rgba, 256, max_w=212, max_h=212, angle_deg=-10.0)
    save_sticker_png(
        hat_sticker_in,
        os.path.join(INTERACT_DIR, "T_UI_WitchHatSticker.png"),
        paper_px=6.5,
        outline_px=2.0,
        snap_bottom_to_edge=False,
    )


# ---------------------------------------------------------------------------
# 3. Rescue Civilian, Flames & Combined Escorts (rescue.png)
# ---------------------------------------------------------------------------
def build_rescue_assets(unarmed_pipo_rgba: Image.Image, pipo_pivot_x: float) -> None:
    rgb = load_rgb("rescue.png")
    h, w = rgb.shape[:2]
    yy, xx = np.mgrid[0:h, 0:w]
    rgb_i = rgb.astype(np.int16)

    # Identify left & right flames vs center Heart-Boxers Guy cleanly:
    # Left flame cluster: x <= 367, y=[365..770] (strictly excludes guy's shorts/feet)
    # Right flame cluster: x >= 530, y=[415..760] (strictly stops above y=770..828 yellow shoe!)
    is_warm = (rgb_i[:, :, 0] - rgb_i[:, :, 2] > 18) & (rgb_i[:, :, 0] > 130)
    is_shorts_region = (xx >= 336) & (xx <= 545) & (yy >= 462) & (yy <= 565)
    left_flame_raw = is_warm & (xx <= 367) & (yy >= 365) & (yy <= 770) & (~is_shorts_region)
    right_flame_raw = is_warm & (xx >= 530) & (yy >= 415) & (yy <= 760) & (~is_shorts_region)

    # Close crayon speckles and enforce large area (>= 5000px) so ZERO stray toes/dots survive
    left_flame_fg = ndimage.binary_closing(left_flame_raw, structure=np.ones((5, 5), dtype=bool))
    left_flame_fg = filter_small_components(left_flame_fg, min_area=5000)
    right_flame_fg = ndimage.binary_closing(right_flame_raw, structure=np.ones((5, 5), dtype=bool))
    right_flame_fg = filter_small_components(right_flame_fg, min_area=5000)

    left_flame_rgba = make_rgba_layer(rgb, left_flame_fg)
    right_flame_rgba = make_rgba_layer(rgb, right_flame_fg)

    # --- Fire Stages 1..4 (3 frames each = 12 PNGs) ---
    left_crop, _ = crop_tight(left_flame_rgba, pad=2)
    right_crop, _ = crop_tight(right_flame_rgba, pad=2)
    right_flip = right_crop.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    left_flip = left_crop.transpose(Image.Transpose.FLIP_LEFT_RIGHT)

    def compose_flame(target_h: int, mode: str, frame: int) -> Image.Image:
        cv = Image.new("RGBA", (512, 512), (*CREAM_PAPER, 0))
        wobble_x = [-10, 0, 10][frame % 3]
        tilt = [-4.0, 0.5, 4.0][frame % 3]
        scale_var = [0.97, 1.03, 0.99][frame % 3]
        eff_h = int(round(target_h * scale_var))

        if mode == "stage1":
            src = [right_crop, left_crop, right_flip][frame % 3]
            placed = place_fg_grounded(src, 512, target_height=eff_h, max_width=320, bottom_y=499, offset_y=0, angle_deg=tilt)
            cv.alpha_composite(placed)
        elif mode == "stage2":
            p_back = place_fg_grounded(
                right_crop if frame != 1 else right_flip,
                512,
                target_height=int(eff_h * 0.86),
                max_width=340,
                bottom_y=499,
                dest_center_x=290 + wobble_x,
                angle_deg=tilt * 0.8,
            )
            p_front = place_fg_grounded(
                left_crop if frame != 2 else left_flip,
                512,
                target_height=eff_h,
                max_width=330,
                bottom_y=499,
                dest_center_x=226 - wobble_x // 2,
                angle_deg=-tilt * 0.7,
            )
            cv.alpha_composite(p_back)
            cv.alpha_composite(p_front)
        elif mode == "stage3":
            p_l = place_fg_grounded(
                left_crop, 512, target_height=int(eff_h * 0.95), max_width=310, bottom_y=499, dest_center_x=185 + wobble_x, angle_deg=-5.0 + tilt
            )
            p_r = place_fg_grounded(
                right_crop, 512, target_height=int(eff_h * 0.90), max_width=310, bottom_y=499, dest_center_x=330 - wobble_x, angle_deg=5.0 - tilt
            )
            p_mid = place_fg_grounded(
                left_flip if frame % 2 == 0 else right_flip,
                512,
                target_height=eff_h,
                max_width=320,
                bottom_y=486,
                dest_center_x=256 + wobble_x // 2,
                angle_deg=tilt,
            )
            cv.alpha_composite(p_l)
            cv.alpha_composite(p_r)
            cv.alpha_composite(p_mid)
            cv = draw_happy_sparkles(
                cv,
                [(155 + frame * 18, 92 - frame * 8, 11, BUTTER_GOLD), (355 - frame * 16, 112 + frame * 6, 9, (255, 115, 50))],
            )
        else:  # stage4 mega-inferno
            p_l = place_fg_grounded(
                left_crop, 512, target_height=int(eff_h * 0.96), max_width=320, bottom_y=499, dest_center_x=160 + wobble_x, angle_deg=-6.0 + tilt
            )
            p_r = place_fg_grounded(
                right_flip, 512, target_height=int(eff_h * 0.95), max_width=320, bottom_y=499, dest_center_x=352 - wobble_x, angle_deg=6.0 - tilt
            )
            p_c1 = place_fg_grounded(
                left_flip, 512, target_height=eff_h, max_width=360, bottom_y=499, dest_center_x=256, angle_deg=tilt
            )
            p_c2 = place_fg_grounded(
                right_crop, 512, target_height=int(eff_h * 0.80), max_width=380, bottom_y=499, dest_center_x=256 - wobble_x, angle_deg=-tilt
            )
            cv.alpha_composite(p_l)
            cv.alpha_composite(p_r)
            cv.alpha_composite(p_c1)
            cv.alpha_composite(p_c2)
            cv = draw_happy_sparkles(
                cv,
                [
                    (125 + frame * 15, 62, 13, BUTTER_GOLD),
                    (258 - frame * 12, 42 + frame * 8, 12, (255, 120, 48)),
                    (392 - frame * 15, 74, 11, BUTTER_GOLD),
                ],
            )
        return cv

    stage_specs = [(1, 290, "stage1"), (2, 368, "stage2"), (3, 435, "stage3"), (4, 470, "stage4")]
    for st_num, th, mode in stage_specs:
        for fr in range(3):
            fl_canvas = compose_flame(th, mode, fr)
            save_sticker_png(
                fl_canvas,
                os.path.join(INTERACT_DIR, f"T_Fire_Stage{st_num}_{fr}.png"),
                fill_small_holes_area=2600,
            )

    # --- Extract Center Heart-Boxers Guy ---
    # Scrub overlapping warm orange-red left-flame edge from his shorts hip (x=[325..372], y=[470..565])
    # Notice blue shorts have B > R and pink hearts have B >= 145, while orange flame has low B < 135
    hip_flame = (
        (xx >= 325)
        & (xx <= 372)
        & (yy >= 470)
        & (yy <= 565)
        & (rgb_i[:, :, 0] - rgb_i[:, :, 2] > 28)
        & (rgb_i[:, :, 2] < 135)
    )
    guy_rgb = rgb.copy()
    guy_rgb[hip_flame] = CREAM_PAPER

    guy_stroke_mask = (~left_flame_fg) & (~right_flame_fg) & (~hip_flame) & (xx >= 260) & (xx <= 585)
    guy_fg = extract_foreground_bridge(guy_rgb, r_bridge=10.0, extra_strokes_mask=guy_stroke_mask)
    guy_fg &= (~left_flame_fg) & (~right_flame_fg) & (~hip_flame) & (xx >= 260) & (xx <= 585)
    # Remove disconnected tiny stray spots (such as the faint oval near his left ankle)
    guy_fg = filter_small_components(guy_fg, min_area=500)
    guy_rgba = make_rgba_layer(guy_rgb, guy_fg)
    guy_pivot_x = 442.0  # midpoint between bare foot and yellow flip-flop

    # T_Civilian_Uninjured_0..1.png
    civ_u0 = place_fg_grounded(guy_rgba, 512, target_height=468, max_width=400, bottom_y=499, pivot_src_x=guy_pivot_x)
    save_sticker_png(civ_u0, os.path.join(CHARS_DIR, "T_Civilian_Uninjured_0.png"))

    civ_u1 = place_fg_grounded(
        guy_rgba, 512, target_height=468, max_width=400, bottom_y=499, pivot_src_x=guy_pivot_x, offset_y=-6, angle_deg=2.8
    )
    save_sticker_png(civ_u1, os.path.join(CHARS_DIR, "T_Civilian_Uninjured_1.png"))

    # T_Civilian_Injured_0..1.png (slumped + soft charcoal soot + cough puffs)
    soot_spots = [
        (232, 165, 22, 13),
        (268, 255, 28, 16),
        (238, 288, 20, 12),
        (266, 338, 18, 11),
    ]
    for ii, (tilt, dy) in enumerate([(-5.0, 0), (-7.2, 4)]):
        inj_base = place_fg_grounded(
            guy_rgba, 512, target_height=452, max_width=395, bottom_y=499, pivot_src_x=guy_pivot_x, offset_y=dy, angle_deg=tilt
        )
        inj_soot = draw_soot_and_cough(inj_base, soot_spots, (282, 148), ii)
        save_sticker_png(inj_soot, os.path.join(CHARS_DIR, f"T_Civilian_Injured_{ii}.png"))

    # T_Civilian_Dead.png (horizontal fainted sticker along bottom with dizzy swirls)
    dead_base = place_fg_grounded(
        guy_rgba, 512, target_height=420, max_width=460, bottom_y=499, dest_center_x=256.0, angle_deg=-86.0
    )
    dead_clean_soot = draw_soot_and_cough(
        dead_base, [(180, 428, 24, 15), (270, 412, 26, 16), (345, 422, 20, 14)], (999, 999), 0
    )
    dead_final = draw_dizzy_swirls(dead_clean_soot, 388, 338)
    save_sticker_png(dead_final, os.path.join(CHARS_DIR, "T_Civilian_Dead.png"))

    # T_Civilian_Rescued_0..1.png (happy relieved blush + hearts & sparkles!)
    for ri, (tilt, dy) in enumerate([(1.5, 0), (-2.0, -5)]):
        res_base = place_fg_grounded(
            guy_rgba, 512, target_height=462, max_width=400, bottom_y=499, pivot_src_x=guy_pivot_x, offset_y=dy, angle_deg=tilt
        )
        res_happy = draw_happy_sparkles(
            res_base,
            [(156, 118 + ri * 8, 12, BUTTER_GOLD), (346, 134 - ri * 6, 10, SKY_BLUE)],
            hearts=[(175, 166 - ri * 6, 9), (332, 180 + ri * 5, 8)],
        )
        save_sticker_png(res_happy, os.path.join(CHARS_DIR, f"T_Civilian_Rescued_{ri}.png"))

    # --- Combined Piggyback Escort Stickers (Pipo front-right + Heart-Boxers Guy on his back!) ---
    def compose_escort(injured: bool) -> Image.Image:
        cv = Image.new("RGBA", (512, 512), (*CREAM_PAPER, 0))
        # Heart-Boxers Guy perched cleanly higher on Pipo's back so both faces are unobstructed
        guy_piggy = place_fg_grounded(
            guy_rgba,
            512,
            target_height=332,
            max_width=270,
            bottom_y=404,
            pivot_src_x=guy_pivot_x,
            dest_center_x=176.0,
            angle_deg=-15.0,
        )
        if injured:
            guy_piggy = draw_soot_and_cough(
                guy_piggy, [(160, 165, 18, 11), (188, 232, 22, 14)], (118, 142), 0
            )
        cv.alpha_composite(guy_piggy)

        # Pipo in front-right carrying him securely
        pipo_front = place_fg_grounded(
            unarmed_pipo_rgba,
            512,
            target_height=436,
            max_width=340,
            bottom_y=499,
            pivot_src_x=pipo_pivot_x,
            dest_center_x=312.0,
            angle_deg=-2.5,
        )
        cv.alpha_composite(pipo_front)
        return cv

    for injured, prefix in [(False, "T_Firefighter_EscortUninjured"), (True, "T_Firefighter_EscortInjured")]:
        esc_idle = compose_escort(injured)
        save_sticker_png(esc_idle, os.path.join(CHARS_DIR, f"{prefix}_Idle.png"), fill_small_holes_area=1200)
        for wi in range(4):
            wf = make_walk_deform(esc_idle, wi, bottom_y=499)
            save_sticker_png(wf, os.path.join(CHARS_DIR, f"{prefix}_Walk_{wi}.png"), fill_small_holes_area=1200)


# ---------------------------------------------------------------------------
# 4. Little Match Girl Pipeline (little girl.png)
# ---------------------------------------------------------------------------
def build_matchgirl_assets() -> None:
    rgb = load_rgb("little girl.png")
    fg = extract_foreground_bridge(rgb, r_bridge=10.0)
    fg = filter_small_components(fg, min_area=40)
    girl_rgba = make_rgba_layer(rgb, fg)

    # Nudge the lowest burnt matchstick (around y=[975..1015], x=[430..520]) slightly up
    # so her crouched knees rest cleanly near the grounding edge without a 25px bottom gap!
    g_arr = np.array(girl_rgba, dtype=np.uint8)
    low_match = g_arr[972:1018, 425:525, :].copy()
    g_arr[972:1018, 425:525, :3] = CREAM_PAPER
    g_arr[972:1018, 425:525, 3] = 0
    # Paste matchstick slightly higher right in front of her crouched knees (y=935..981)
    mask_lm = low_match[:, :, 3] > 128
    dst_slice = g_arr[934:980, 425:525, :]
    dst_slice[mask_lm] = low_match[mask_lm]
    girl_clean = Image.fromarray(g_arr, mode="RGBA")

    # Center horizontally around her face/body (x ~ 460) while keeping top-left chicken bubble visible
    mg0 = place_fg_grounded(girl_clean, 512, target_height=468, max_width=455, bottom_y=499, pivot_src_x=455.0)
    save_sticker_png(mg0, os.path.join(CHARS_DIR, "T_MatchGirl_Idle_0.png"))

    mg1_base = place_fg_grounded(
        girl_clean,
        512,
        target_height=474,
        max_width=460,
        bottom_y=499,
        pivot_src_x=455.0,
        scale_x=1.015,
        scale_y=0.985,
        angle_deg=1.0,
    )
    # Add tiny golden roast-chicken aroma sparkle over her thought bubble
    mg1 = draw_happy_sparkles(mg1_base, [(126, 62, 9, BUTTER_GOLD)])
    save_sticker_png(mg1, os.path.join(CHARS_DIR, "T_MatchGirl_Idle_1.png"))


# ---------------------------------------------------------------------------
# 5. Angel Puppy Coco Pipeline (dog.png)
# ---------------------------------------------------------------------------
def build_angeldog_assets() -> Image.Image:
    rgb = load_rgb("dog.png")
    fg = extract_foreground_bridge(rgb, r_bridge=7.0)
    fg = filter_small_components(fg, min_area=20)
    dog_rgba = make_rgba_layer(rgb, fg)

    dog_pivot_x = 262.0
    d_idle0 = place_fg_grounded(dog_rgba, 512, target_height=450, max_width=440, bottom_y=499, pivot_src_x=dog_pivot_x)
    save_sticker_png(d_idle0, os.path.join(CHARS_DIR, "T_AngelDog_Idle_0.png"))

    d_idle1 = place_fg_grounded(
        dog_rgba,
        512,
        target_height=458,
        max_width=446,
        bottom_y=499,
        pivot_src_x=dog_pivot_x,
        scale_x=1.02,
        scale_y=0.98,
        angle_deg=-1.5,
    )
    save_sticker_png(d_idle1, os.path.join(CHARS_DIR, "T_AngelDog_Idle_1.png"))

    # Happy jumping / celebratory Coco frames
    dh0_base = place_fg_grounded(
        dog_rgba, 512, target_height=442, max_width=430, bottom_y=499, pivot_src_x=dog_pivot_x, angle_deg=4.5
    )
    dh0 = draw_happy_sparkles(
        dh0_base,
        [(102, 112, 13, BUTTER_GOLD), (412, 98, 12, SKY_BLUE), (424, 240, 10, BUTTER_GOLD)],
        hearts=[(392, 165, 11)],
    )
    save_sticker_png(dh0, os.path.join(CHARS_DIR, "T_AngelDog_Happy_0.png"))

    dh1_base = place_fg_grounded(
        dog_rgba,
        512,
        target_height=442,
        max_width=430,
        bottom_y=486,
        pivot_src_x=dog_pivot_x,
        angle_deg=-5.0,
    )
    dh1 = draw_happy_sparkles(
        dh1_base,
        [(88, 95, 14, SKY_BLUE), (428, 122, 13, BUTTER_GOLD), (398, 224, 10, BUTTER_GOLD)],
        hearts=[(114, 185, 10), (402, 178, 12)],
    )
    save_sticker_png(dh1, os.path.join(CHARS_DIR, "T_AngelDog_Happy_1.png"))

    # T_UI_AngelDogSticker.png (256x256 centered UI badge)
    ui_dog = place_fg_centered(dog_rgba, 256, max_w=218, max_h=218)
    save_sticker_png(
        ui_dog,
        os.path.join(INTERACT_DIR, "T_UI_AngelDogSticker.png"),
        paper_px=6.5,
        outline_px=2.0,
        snap_bottom_to_edge=False,
    )
    return dog_rgba


# ---------------------------------------------------------------------------
# 6. Hand-Drawn Sketchbook Doors & Cozy Dollhouse Environment Textures
# ---------------------------------------------------------------------------
def _wobbly_rect_pts(x0: float, y0: float, x1: float, y1: float, amp: float = 2.0, seed: int = 1):
    rng = np.random.default_rng(seed)
    pts = []
    for t in np.linspace(0, 1, 14, endpoint=False):
        pts.append((x0 + (x1 - x0) * t, y0 + rng.uniform(-amp, amp)))
    for t in np.linspace(0, 1, 18, endpoint=False):
        pts.append((x1 + rng.uniform(-amp, amp), y0 + (y1 - y0) * t))
    for t in np.linspace(0, 1, 14, endpoint=False):
        pts.append((x1 - (x1 - x0) * t, y1 + rng.uniform(-amp, amp)))
    for t in np.linspace(0, 1, 18, endpoint=False):
        pts.append((x0 + rng.uniform(-amp, amp), y1 - (y1 - y0) * t))
    return pts


def _apply_crayon_grain(img: Image.Image, strength: float = 10.0, seed: int = 7) -> Image.Image:
    rng = np.random.default_rng(seed)
    arr = np.array(img, dtype=np.int16)
    noise = rng.normal(0.0, strength, size=arr.shape[:2]).astype(np.int16)
    mask = arr[:, :, 3] > 128 if arr.shape[2] == 4 else np.ones(arr.shape[:2], dtype=bool)
    for c in range(3):
        arr[:, :, c] = np.where(mask, np.clip(arr[:, :, c] + noise, 0, 255), arr[:, :, c])
    return Image.fromarray(arr.astype(np.uint8), mode=img.mode)


def build_doors_and_environments(dog_rgba: Image.Image) -> None:
    # 1. T_Door_Closed.png (512x512 hand-drawn oak door die-cut sticker grounded bottom-center)
    dc = Image.new("RGBA", (512, 512), (*CREAM_PAPER, 0))
    draw = ImageDraw.Draw(dc)
    frame_pts = _wobbly_rect_pts(126, 22, 386, 499, amp=2.2, seed=11)
    draw.polygon(frame_pts, fill=(116, 68, 36, 255), outline=(*INK_WARM, 255), width=5)
    leaf_pts = _wobbly_rect_pts(144, 40, 368, 499, amp=2.0, seed=12)
    draw.polygon(leaf_pts, fill=(182, 118, 66, 255), outline=(*INK_WARM, 255), width=4)
    # Recessed hand-drawn oak panels
    top_panel = _wobbly_rect_pts(172, 72, 340, 240, amp=2.0, seed=13)
    bot_panel = _wobbly_rect_pts(172, 272, 340, 464, amp=2.0, seed=14)
    draw.polygon(top_panel, fill=(152, 92, 48, 255), outline=(*INK_WARM, 255), width=4)
    draw.polygon(bot_panel, fill=(152, 92, 48, 255), outline=(*INK_WARM, 255), width=4)
    # Crayon wood-grain strokes
    for gx, gy0, gy1 in [(194, 84, 224), (255, 78, 232), (316, 90, 220), (202, 286, 450), (298, 294, 444)]:
        draw.line([(gx, gy0), (gx + 3, (gy0 + gy1) // 2), (gx - 2, gy1)], fill=(122, 72, 38, 255), width=3)
    # Cute brass doorknob + keyhole plate
    draw.rounded_rectangle([326, 242, 354, 296], radius=8, fill=(214, 166, 66, 255), outline=(*INK_WARM, 255), width=3)
    draw.ellipse([322, 248, 358, 284], fill=(*BUTTER_GOLD, 255), outline=(*INK_WARM, 255), width=4)
    draw.ellipse([330, 256, 340, 266], fill=(255, 248, 220, 255))
    dc = _apply_crayon_grain(dc, strength=7.5, seed=15)
    save_sticker_png(dc, os.path.join(INTERACT_DIR, "T_Door_Closed.png"))

    # 2. T_Door_Open.png (512x512 hand-drawn swung-open oak door leaf & sunlit opening)
    do = Image.new("RGBA", (512, 512), (*CREAM_PAPER, 0))
    draw = ImageDraw.Draw(do)
    frame_pts = _wobbly_rect_pts(126, 22, 386, 499, amp=2.2, seed=21)
    draw.polygon(frame_pts, fill=(116, 68, 36, 255), outline=(*INK_WARM, 255), width=5)
    interior_pts = _wobbly_rect_pts(144, 40, 368, 499, amp=1.8, seed=22)
    draw.polygon(interior_pts, fill=(248, 236, 206, 255), outline=(*INK_WARM, 255), width=3)
    # Swung-open oak door leaf hinged on right jamb (x=368) swinging inward
    leaf_poly = [(368, 40), (262, 68), (262, 480), (368, 499)]
    draw.polygon(leaf_poly, fill=(182, 118, 66, 255), outline=(*INK_WARM, 255), width=4)
    draw.polygon([(350, 76), (282, 94), (282, 246), (350, 238)], fill=(152, 92, 48, 255), outline=(*INK_WARM, 255), width=3)
    draw.polygon([(350, 266), (282, 270), (282, 444), (350, 460)], fill=(152, 92, 48, 255), outline=(*INK_WARM, 255), width=3)
    draw.ellipse([272, 244, 298, 270], fill=(*BUTTER_GOLD, 255), outline=(*INK_WARM, 255), width=3)
    do = _apply_crayon_grain(do, strength=7.5, seed=25)
    save_sticker_png(do, os.path.join(INTERACT_DIR, "T_Door_Open.png"))

    # 3. T_SafeZone_Mat.png (512x512 opaque cozy stitched pastel-mint & cream rescue mat with paw-print/star emblem)
    mat = Image.new("RGBA", (512, 512), (128, 198, 168, 255))
    mdraw = ImageDraw.Draw(mat)
    # Outer soft border
    mdraw.rounded_rectangle([18, 18, 494, 494], radius=46, fill=(168, 224, 198, 255), outline=(74, 128, 102, 255), width=6)
    mdraw.rounded_rectangle([42, 42, 470, 470], radius=34, fill=(255, 248, 232, 255), outline=(92, 148, 122, 255), width=5)
    # Dashed hand-stitched inner quilt line
    for i in range(62, 450, 26):
        mdraw.line([(i, 58), (i + 14, 58)], fill=(110, 170, 142, 255), width=4)
        mdraw.line([(i, 454), (i + 14, 454)], fill=(110, 170, 142, 255), width=4)
        mdraw.line([(58, i), (58, i + 14)], fill=(110, 170, 142, 255), width=4)
        mdraw.line([(454, i), (454, i + 14)], fill=(110, 170, 142, 255), width=4)
    # Center pastel-mint medallion + cute paw print + stars
    mdraw.ellipse([128, 128, 384, 384], fill=(176, 228, 204, 255), outline=(88, 145, 118, 255), width=5)
    # Cute paw print in center
    mdraw.ellipse([212, 246, 300, 322], fill=(255, 248, 232, 255), outline=(*INK_WARM, 255), width=4)
    for tx, ty, tr in [(194, 214, 19), (232, 188, 20), (280, 188, 20), (318, 214, 19)]:
        mdraw.ellipse([tx - tr, ty - tr * 1.15, tx + tr, ty + tr * 1.15], fill=(255, 248, 232, 255), outline=(*INK_WARM, 255), width=4)
    # Corner stars
    mat = draw_happy_sparkles(
        mat,
        [(98, 98, 16, BUTTER_GOLD), (414, 98, 16, BUTTER_GOLD), (98, 414, 16, BUTTER_GOLD), (414, 414, 16, BUTTER_GOLD)],
    )
    mat = _apply_crayon_grain(mat, strength=5.0, seed=31)
    mat_arr = np.array(mat)
    mat_arr[:, :, 3] = 255
    Image.fromarray(mat_arr, mode="RGBA").save(os.path.join(ENV_DIR, "T_SafeZone_Mat.png"), format="PNG")

    # 4. T_Porch_Wood.png (512x512 seamless tileable warm porch wooden floorboards, alpha=255)
    pw = Image.new("RGBA", (512, 512), (144, 102, 68, 255))
    pdraw = ImageDraw.Draw(pw)
    plank_h = 64  # divides 512 into 8 exact horizontal planks
    rng = np.random.default_rng(77)
    plank_tones = [(146, 104, 70), (136, 96, 64), (154, 112, 76), (140, 99, 67)]
    for row in range(8):
        y0 = row * plank_h
        y1 = y0 + plank_h
        tone = plank_tones[row % len(plank_tones)]
        pdraw.rectangle([0, y0, 512, y1], fill=(*tone, 255))
        # Subtle wood-grain horizontal lines wrapped seamlessly across x=0..512
        for g in range(3):
            gy = y0 + 14 + g * 18 + int(rng.integers(-2, 3))
            pdraw.line([(0, gy), (512, gy)], fill=(tone[0] - 14, tone[1] - 11, tone[2] - 8, 255), width=2)
        # Plank horizontal seam
        pdraw.line([(0, y0), (512, y0)], fill=(96, 66, 42, 255), width=3)
        # Staggered vertical plank joints + nail dots
        jx = (row * 160 + 96) % 512
        pdraw.line([(jx, y0), (jx, y1)], fill=(96, 66, 42, 255), width=3)
        for nx in [(jx + 12) % 512, (jx - 12) % 512]:
            pdraw.ellipse([nx - 3, y0 + 12, nx + 3, y0 + 18], fill=(78, 54, 36, 255))
            pdraw.ellipse([nx - 3, y1 - 18, nx + 3, y1 - 12], fill=(78, 54, 36, 255))
    pw = _apply_crayon_grain(pw, strength=5.5, seed=32)
    pw_arr = np.array(pw)
    pw_arr[:, :, 3] = 255
    Image.fromarray(pw_arr, mode="RGBA").save(os.path.join(ENV_DIR, "T_Porch_Wood.png"), format="PNG")

    # 5. T_Portrait_Coco.png (512x512 framed hand-drawn wall portrait featuring Angel Puppy Coco!)
    pt = Image.new("RGBA", (512, 512), (124, 76, 42, 255))
    pt_draw = ImageDraw.Draw(pt)
    # Cozy wooden frame + cream matboard + sky-blue pastel background
    pt_draw.rectangle([28, 28, 484, 484], fill=(166, 108, 62, 255), outline=(*INK_WARM, 255), width=6)
    pt_draw.rectangle([52, 52, 460, 460], fill=(255, 248, 232, 255), outline=(*INK_WARM, 255), width=5)
    pt_draw.rectangle([82, 76, 430, 404], fill=(208, 238, 248, 255), outline=(*INK_WARM, 255), width=4)
    # Paste actual extracted Coco inside portrait
    coco_crop, _ = crop_tight(dog_rgba, pad=2)
    c_scale = min(310.0 / coco_crop.width, 300.0 / coco_crop.height)
    cw = max(8, int(round(coco_crop.width * c_scale)))
    ch = max(8, int(round(coco_crop.height * c_scale)))
    coco_rs = coco_crop.resize((cw, ch), Image.Resampling.BILINEAR)
    pt.alpha_composite(coco_rs, dest=(256 - cw // 2, 240 - ch // 2))
    # Cute little brass frame plaque (no letters!) at bottom of matboard
    pt_draw = ImageDraw.Draw(pt)
    pt_draw.rounded_rectangle([188, 418, 324, 446], radius=6, fill=(*BUTTER_GOLD, 255), outline=(*INK_WARM, 255), width=3)
    pt = draw_happy_sparkles(pt, [(118, 114, 11, BUTTER_GOLD), (394, 114, 11, BUTTER_GOLD)])
    pt = _apply_crayon_grain(pt, strength=4.5, seed=33)
    pt_arr = np.array(pt)
    pt_arr[:, :, 3] = 255
    Image.fromarray(pt_arr, mode="RGBA").save(os.path.join(ENV_DIR, "T_Portrait_Coco.png"), format="PNG")


# ---------------------------------------------------------------------------
# 7. Seamless 16-Second Cozy Sketchbook Comedy BGM (BGM_StageLoop.wav)
# ---------------------------------------------------------------------------
def build_bgm_wav() -> None:
    """Synthesizes a seamless 16.0-second 44.1kHz 16-bit PCM marimba/pizzicato/toy-piano loop."""
    duration_sec = 16.0
    num_samples = int(round(duration_sec * SAMPLE_RATE))  # 705,600 samples
    buf = np.zeros(num_samples, dtype=np.float64)

    def add_wrapped_note(start_sec: float, dur_sec: float, freq: float, amp: float, timbre: str = "marimba") -> None:
        n = int(round(dur_sec * SAMPLE_RATE))
        if n <= 0:
            return
        t = np.arange(n, dtype=np.float64) / SAMPLE_RATE
        atk = 0.008
        env = np.minimum(1.0, t / atk) * np.exp(-t / max(0.05, dur_sec * 0.36))
        # Fade out cleanly at end of note buffer
        tail = min(n, int(0.02 * SAMPLE_RATE))
        if tail > 0:
            env[-tail:] *= np.linspace(1.0, 0.0, tail)

        if timbre == "marimba":
            sig = np.sin(2.0 * np.pi * freq * t) + 0.22 * np.sin(2.0 * np.pi * (4.0 * freq) * t) * np.exp(-t * 28.0)
        elif timbre == "toypiano":
            sig = (
                0.78 * np.sin(2.0 * np.pi * freq * t)
                + 0.26 * np.sin(2.0 * np.pi * 2.0 * freq * t)
                + 0.12 * np.sin(2.0 * np.pi * 3.0 * freq * t) * np.exp(-t * 18.0)
            )
        else:  # pizzicato bass
            sig = (
                0.85 * np.sin(2.0 * np.pi * freq * t)
                + 0.30 * np.sin(2.0 * np.pi * 2.0 * freq * t) * np.exp(-t * 14.0)
            )

        wave_samples = sig * env * amp
        start_idx = int(round(start_sec * SAMPLE_RATE)) % num_samples
        idxs = (start_idx + np.arange(n)) % num_samples
        np.add.at(buf, idxs, wave_samples)

    # 120 BPM -> 1 beat = 0.50s, 8 bars = 32 beats = 16.00s
    beat = 0.50
    notes_hz = {
        "C3": 130.81, "D3": 146.83, "E3": 164.81, "F3": 174.61, "G3": 196.00, "A3": 220.00, "B3": 246.94,
        "C4": 261.63, "D4": 293.66, "E4": 329.63, "F4": 349.23, "G4": 392.00, "A4": 440.00, "B4": 493.88,
        "C5": 523.25, "D5": 587.33, "E5": 659.25, "F5": 698.46, "G5": 783.99, "A5": 880.00,
    }

    # Playful 8-bar progression: C | G | Am | F | Dm | G7 | C | G7
    chords = [
        ("C3", "G3", ["E4", "G4", "C5"]),
        ("G3", "D3", ["D4", "G4", "B4"]),
        ("A3", "E3", ["C4", "E4", "A4"]),
        ("F3", "C3", ["C4", "F4", "A4"]),
        ("D3", "A3", ["D4", "F4", "A4"]),
        ("G3", "D3", ["F4", "G4", "B4"]),
        ("C3", "G3", ["E4", "G4", "C5"]),
        ("G3", "D3", ["D4", "F4", "B4"]),
    ]

    for bar, (bass1, bass2, triad) in enumerate(chords):
        b0 = bar * 4.0 * beat
        # Bouncy pizzicato bass on beats 1 & 3, light pluck on 2 & 4
        add_wrapped_note(b0 + 0.0 * beat, 0.42, notes_hz[bass1], 0.28, "pizz")
        add_wrapped_note(b0 + 1.0 * beat, 0.25, notes_hz[triad[0]], 0.10, "marimba")
        add_wrapped_note(b0 + 2.0 * beat, 0.42, notes_hz[bass2], 0.24, "pizz")
        add_wrapped_note(b0 + 3.0 * beat, 0.25, notes_hz[triad[1]], 0.10, "marimba")

    # Charming quirky marimba + toy-piano melody across 8 bars
    melody = [
        # Bar 0 (C)
        (0.0, 0.45, "E5", "marimba"), (0.5, 0.35, "G5", "marimba"), (1.0, 0.45, "E5", "marimba"),
        (1.5, 0.35, "C5", "toypiano"), (2.0, 0.55, "D5", "marimba"), (3.0, 0.45, "E5", "marimba"),
        # Bar 1 (G)
        (4.0, 0.45, "D5", "marimba"), (4.5, 0.35, "B4", "marimba"), (5.0, 0.65, "G4", "toypiano"),
        (6.0, 0.45, "A4", "marimba"), (7.0, 0.45, "B4", "marimba"),
        # Bar 2 (Am)
        (8.0, 0.45, "C5", "marimba"), (8.5, 0.35, "E5", "marimba"), (9.0, 0.45, "A5", "toypiano"),
        (10.0, 0.45, "G5", "marimba"), (10.5, 0.35, "E5", "marimba"), (11.0, 0.55, "C5", "marimba"),
        # Bar 3 (F)
        (12.0, 0.45, "F5", "marimba"), (12.5, 0.35, "E5", "marimba"), (13.0, 0.45, "D5", "marimba"),
        (14.0, 0.75, "C5", "toypiano"), (15.0, 0.40, "A4", "marimba"),
        # Bar 4 (Dm)
        (16.0, 0.45, "D5", "marimba"), (16.5, 0.35, "F5", "marimba"), (17.0, 0.45, "E5", "marimba"),
        (18.0, 0.45, "D5", "marimba"), (19.0, 0.45, "C5", "toypiano"),
        # Bar 5 (G7)
        (20.0, 0.45, "B4", "marimba"), (20.5, 0.35, "D5", "marimba"), (21.0, 0.55, "G5", "toypiano"),
        (22.0, 0.45, "F5", "marimba"), (23.0, 0.45, "D5", "marimba"),
        # Bar 6 (C)
        (24.0, 0.45, "E5", "marimba"), (24.5, 0.35, "G5", "marimba"), (25.0, 0.50, "A5", "toypiano"),
        (26.0, 0.45, "G5", "marimba"), (26.5, 0.35, "E5", "marimba"), (27.0, 0.55, "C5", "marimba"),
        # Bar 7 (G7 turnaround back to C)
        (28.0, 0.45, "D5", "marimba"), (29.0, 0.45, "E5", "marimba"), (30.0, 0.45, "F5", "toypiano"),
        (31.0, 0.45, "G5", "marimba"),
    ]
    for beat_pos, dur_beats, note_name, timbre in melody:
        amp = 0.24 if timbre == "marimba" else 0.18
        add_wrapped_note(beat_pos * beat, dur_beats * beat * 1.4, notes_hz[note_name], amp, timbre)

    # Gentle warm brushed shaker pulse on off-beats
    rng = np.random.default_rng(31415)
    shaker_len = int(0.045 * SAMPLE_RATE)
    shaker_env = np.sin(np.pi * np.arange(shaker_len) / shaker_len) ** 1.6
    for b in range(64):  # eighth notes
        accent = 0.032 if (b % 2 == 1) else 0.014
        noise = rng.uniform(-1.0, 1.0, shaker_len)
        # simple high-pass shimmer
        hp = np.diff(noise, prepend=0.0) * shaker_env * accent
        s0 = int(round((b * 0.25) * SAMPLE_RATE)) % num_samples
        idxs = (s0 + np.arange(shaker_len)) % num_samples
        np.add.at(buf, idxs, hp)

    peak = float(np.max(np.abs(buf)))
    if peak > 1e-6:
        buf = buf * (0.68 / peak)

    pcm = np.clip(np.round(buf * 32767.0), -32767, 32767).astype(np.int16)
    os.makedirs(AUDIO_DIR, exist_ok=True)
    out_wav = os.path.join(AUDIO_DIR, "BGM_StageLoop.wav")
    with wave.open(out_wav, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(SAMPLE_RATE)
        wf.writeframes(pcm.tobytes())


# ---------------------------------------------------------------------------
# 8. Output Integrity Verifier
# ---------------------------------------------------------------------------
def verify_all_outputs() -> None:
    expected_raw = ["firefighter.png", "witch.png", "rescue.png", "little girl.png", "dog.png"]
    for fn in expected_raw:
        p = os.path.join(CHARS_DIR, fn)
        assert os.path.isfile(p) and os.path.getsize(p) > 0, f"Original artwork missing: {p}"

    expected_chars = [
        "T_Firefighter_Idle_0.png", "T_Firefighter_Idle_1.png",
        "T_Firefighter_Walk_0.png", "T_Firefighter_Walk_1.png", "T_Firefighter_Walk_2.png", "T_Firefighter_Walk_3.png",
        "T_Firefighter_Scared.png",
        "T_Firefighter_Tool_Idle.png",
        "T_Firefighter_Tool_Walk_0.png", "T_Firefighter_Tool_Walk_1.png", "T_Firefighter_Tool_Walk_2.png", "T_Firefighter_Tool_Walk_3.png",
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
        "T_Civilian_Rescued_0.png", "T_Civilian_Rescued_1.png",
        "T_MatchGirl_Idle_0.png", "T_MatchGirl_Idle_1.png",
        "T_AngelDog_Idle_0.png", "T_AngelDog_Idle_1.png",
        "T_AngelDog_Happy_0.png", "T_AngelDog_Happy_1.png",
    ]
    expected_interact_512 = [
        "T_Door_Closed.png", "T_Door_Open.png", "T_Extinguisher.png",
        "T_WaterJet_0.png", "T_WaterJet_1.png",
        "T_Fire_Stage1_0.png", "T_Fire_Stage1_1.png", "T_Fire_Stage1_2.png",
        "T_Fire_Stage2_0.png", "T_Fire_Stage2_1.png", "T_Fire_Stage2_2.png",
        "T_Fire_Stage3_0.png", "T_Fire_Stage3_1.png", "T_Fire_Stage3_2.png",
        "T_Fire_Stage4_0.png", "T_Fire_Stage4_1.png", "T_Fire_Stage4_2.png",
    ]
    expected_interact_256 = ["T_UI_AngelDogSticker.png", "T_UI_WitchHatSticker.png"]
    expected_env_new = ["T_SafeZone_Mat.png", "T_Porch_Wood.png", "T_Portrait_Coco.png"]
    expected_sfx = [
        "SFX_PaperOpen.wav", "SFX_MarkerSwipe.wav", "SFX_ActionPop.wav", "SFX_ThoughtBubble.wav",
        "SFX_DoorOpen.wav", "SFX_PickupTool.wav", "SFX_WaterSpray.wav", "SFX_FireOut.wav",
        "SFX_StarEarn.wav", "SFX_StarBurn.wav", "SFX_Footstep.wav", "BGM_StageLoop.wav",
    ]

    for fn in expected_chars:
        im = Image.open(os.path.join(CHARS_DIR, fn))
        assert im.mode == "RGBA" and im.size == (512, 512), f"Invalid char PNG {fn}: {im.mode} {im.size}"
        arr = np.array(im)
        alphas = set(np.unique(arr[:, :, 3]).tolist())
        assert alphas == {0, 255}, f"Non-binary alpha on {fn}: {alphas}"
        assert np.any(arr[511, :, 3] == 255), f"Not grounded on y=511: {fn}"

    for fn in expected_interact_512:
        im = Image.open(os.path.join(INTERACT_DIR, fn))
        assert im.mode == "RGBA" and im.size == (512, 512), f"Invalid interact PNG {fn}: {im.mode} {im.size}"
        arr = np.array(im)
        alphas = set(np.unique(arr[:, :, 3]).tolist())
        assert alphas == {0, 255}, f"Non-binary alpha on {fn}: {alphas}"
        if not fn.startswith("T_WaterJet_"):
            assert np.any(arr[511, :, 3] == 255), f"Not grounded on y=511: {fn}"

    for fn in expected_interact_256:
        im = Image.open(os.path.join(INTERACT_DIR, fn))
        assert im.mode == "RGBA" and im.size == (256, 256), f"Invalid UI sticker {fn}: {im.mode} {im.size}"
        arr = np.array(im)
        alphas = set(np.unique(arr[:, :, 3]).tolist())
        assert alphas == {0, 255}, f"Non-binary alpha on {fn}: {alphas}"

    for fn in expected_env_new:
        im = Image.open(os.path.join(ENV_DIR, fn))
        assert im.mode == "RGBA" and im.size == (512, 512), f"Invalid env PNG {fn}: {im.mode} {im.size}"
        arr = np.array(im)
        assert set(np.unique(arr[:, :, 3]).tolist()) == {255}, f"Non-opaque env texture {fn}"

    for fn in expected_sfx:
        p = os.path.join(AUDIO_DIR, fn)
        assert os.path.isfile(p) and os.path.getsize(p) > 44, f"Missing audio {p}"
        with wave.open(p, "rb") as wf:
            assert wf.getnchannels() == 1 and wf.getsampwidth() == 2 and wf.getframerate() == SAMPLE_RATE
            if fn == "BGM_StageLoop.wav":
                assert wf.getnframes() == int(16.0 * SAMPLE_RATE)

    print(
        f"VERIFIED HAND-DRAWN ASSETS: {len(expected_chars)} char PNGs, "
        f"{len(expected_interact_512) + len(expected_interact_256)} interaction/UI PNGs, "
        f"{len(expected_env_new)} new env PNGs (+4 preserved), "
        f"{len(expected_sfx)} audio WAVs (incl. 16s BGM_StageLoop.wav)."
    )


def main() -> None:
    unarmed_pipo_rgba, pipo_pivot_x = build_firefighter_assets()
    build_witch_assets()
    build_rescue_assets(unarmed_pipo_rgba, pipo_pivot_x)
    build_matchgirl_assets()
    dog_rgba = build_angeldog_assets()
    build_doors_and_environments(dog_rgba)
    build_bgm_wav()
    verify_all_outputs()


if __name__ == "__main__":
    main()
