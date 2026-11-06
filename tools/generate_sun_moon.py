"""Generates the sun and moon textures of the pack.

mcpack/textures/environment/sun.png          256x256 RGBA, round sun
mcpack/textures/environment/moon_phases.png  2160x1080 RGBA, 4x2 tiles of 540x540, the 8 phases

Sun: a disc with limb darkening (centre white, rim warmer) and a soft glow around it.

Moon: the real surface comes from a NASA photograph of the near side (tools/sources/moon_nasa.jpg),
which is almost a full moon, so it works as the albedo. Every phase relights that albedo on a sphere:
  - the sphere normal comes from the disc (orthographic view, x right, y up, z toward the viewer)
  - the light direction is rotated by the phase angle, waxing phases are lit from the right and waning
    phases from the left (northern hemisphere view)
  - brightness follows the Lommel-Seeliger law, 2*mu0/(mu0 + mu), which is why a full moon is flat
    and why the lit limb stays bright, instead of the Lambert law that would darken it
  - the terminator is softened a little and the dark side gets a faint earthshine
Tile order is the same as the vanilla moon_phases.png:
  full, waning gibbous, last quarter, waning crescent / new, waxing crescent, first quarter, waxing gibbous

Usage: python3 tools/generate_sun_moon.py
Needs: numpy, scipy, Pillow
"""
import os

import numpy as np
from PIL import Image, ImageFilter
from scipy import ndimage

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.normpath(os.path.join(HERE, "..", "mcpack", "textures", "environment"))
MOON_SOURCE = os.path.join(HERE, "sources", "moon_nasa.jpg")

SUN_SIZE = 256
SUN_DISC = 0.60      # disc diameter as a fraction of the tile, the rest is glow
MOON_TILE = 540
MOON_DISC = 0.90     # disc diameter as a fraction of the tile

# (phase angle in degrees, waxing). 0 degrees is a full moon, 180 degrees a new moon
PHASES = [(0, False), (45, False), (90, False), (135, False), (180, False), (135, True), (90, True), (45, True)]


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def make_sun():
    n = SUN_SIZE
    c = n / 2.0
    ys, xs = np.mgrid[0:n, 0:n]
    r = np.hypot(xs + 0.5 - c, ys + 0.5 - c)
    radius = SUN_DISC * n / 2.0

    # disc: limb darkening I(mu) = 1 - u*(1 - mu) with u = 0.3, mu = cos of the angle to the view direction
    mu = np.sqrt(np.clip(1.0 - (r / radius) ** 2, 0.0, 1.0))
    limb = 1.0 - 0.30 * (1.0 - mu)
    centre = np.array([1.00, 0.98, 0.94])
    rim = np.array([1.00, 0.88, 0.70])
    disc_col = (rim[None, None, :] * (1.0 - mu[..., None]) + centre[None, None, :] * mu[..., None]) * limb[..., None]
    disc_a = np.clip(radius + 0.5 - r, 0.0, 1.0)

    # glow: falls off from the rim, reaches zero before the tile edge
    glow_a = 0.55 * np.exp(-np.clip(r - radius, 0.0, None) / (0.07 * n)) * (1.0 - smoothstep(0.34 * n, 0.5 * n, r))
    glow_col = np.array([1.00, 0.93, 0.80])

    alpha = disc_a + glow_a * (1.0 - disc_a)
    # color is premultiplied outside the disc so additive and alpha blending both give the same glow
    rgb = disc_col * disc_a[..., None] + glow_col[None, None, :] * (glow_a * (1.0 - disc_a))[..., None]
    out = np.dstack([np.clip(rgb, 0, 1), np.clip(alpha, 0, 1)])
    return Image.fromarray((out * 255.0 + 0.5).astype(np.uint8), "RGBA")


def fit_circle(gray):
    mask = gray > 30
    edge = mask & ~ndimage.binary_erosion(mask, iterations=2)
    ys, xs = np.nonzero(edge)
    A = np.c_[2 * xs, 2 * ys, np.ones_like(xs)]
    b = xs ** 2 + ys ** 2
    sol = np.linalg.lstsq(A, b, rcond=None)[0]
    cx, cy = sol[0], sol[1]
    radius = np.sqrt(sol[2] + cx ** 2 + cy ** 2)
    for _ in range(3):  # drop the terminator side and refit
        d = np.hypot(xs - cx, ys - cy)
        keep = np.abs(d - radius) < 0.012 * radius
        sol = np.linalg.lstsq(A[keep], b[keep], rcond=None)[0]
        cx, cy = sol[0], sol[1]
        radius = np.sqrt(sol[2] + cx ** 2 + cy ** 2)
    return cx, cy, radius


def moon_albedo_tile():
    src = Image.open(MOON_SOURCE).convert("RGB")
    gray = np.asarray(src.convert("L").filter(ImageFilter.GaussianBlur(2)), dtype=float)
    cx, cy, radius = fit_circle(gray)
    target_r = MOON_DISC * MOON_TILE / 2.0
    scale = radius / target_r                       # source pixels per tile pixel
    half = MOON_TILE / 2.0 * scale
    box = (cx - half, cy - half, cx + half, cy + half)
    tile = src.resize((MOON_TILE, MOON_TILE), Image.LANCZOS, box=box)
    return np.asarray(tile, dtype=float) / 255.0, target_r


def make_moon():
    albedo, radius = moon_albedo_tile()
    n = MOON_TILE
    c = n / 2.0
    ys, xs = np.mgrid[0:n, 0:n]
    x = (xs + 0.5 - c) / radius
    y = -(ys + 0.5 - c) / radius
    r2 = x * x + y * y
    z = np.sqrt(np.clip(1.0 - r2, 0.0, 1.0))
    alpha = np.clip(radius + 0.5 - np.sqrt(r2) * radius, 0.0, 1.0)

    sheet = np.zeros((2 * n, 4 * n, 4))
    for index, (angle_deg, waxing) in enumerate(PHASES):
        a = np.radians(angle_deg)
        lx = np.sin(a) * (1.0 if waxing else -1.0)
        lz = np.cos(a)
        mu0 = x * lx + z * lz                       # cosine of the angle to the sun
        mu = z                                      # cosine of the angle to the viewer
        lit = np.clip(mu0, 0.0, None)
        ls = np.minimum(2.0 * lit / (lit + mu + 1e-3), 1.25)       # Lommel-Seeliger, 1.0 for a full moon
        term = smoothstep(0.0, 0.08, mu0)                          # soft terminator
        gain = 1.0 - 0.35 * (angle_deg / 180.0)                    # a partial moon is dimmer than its lit area suggests
        illum = ls * term * gain
        earthshine = 0.04 * (angle_deg / 180.0)                    # faint light reflected by the Earth on the dark side
        shade = illum + earthshine * (1.0 - term)
        color = albedo * shade[..., None]
        color = color * alpha[..., None]            # premultiplied, background stays black
        tile = np.dstack([np.clip(color, 0, 1), alpha])
        row, col = divmod(index, 4)
        sheet[row * n:(row + 1) * n, col * n:(col + 1) * n] = tile
    return Image.fromarray((sheet * 255.0 + 0.5).astype(np.uint8), "RGBA")


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    sun = make_sun()
    sun.save(os.path.join(OUT_DIR, "sun.png"), optimize=True)
    moon = make_moon()
    moon.save(os.path.join(OUT_DIR, "moon_phases.png"), optimize=True)
    print("sun", sun.size, "moon", moon.size, "->", OUT_DIR)


if __name__ == "__main__":
    main()
