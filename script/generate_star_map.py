"""Generates the star map texture used by the Sky material.

2048x1024 RGBA, equirectangular in the equatorial frame: x is right ascension (0..360 deg), y is declination
(-90..90 deg). The sky shader reads it once per pixel, and the rotation of the sky is only an offset on x.
  R  stars: small squares, all the same size, brightness from the star magnitude.
  G  constellation lines (great-circle arcs between the figure stars).
Star positions and figures are real (Hipparcos, via d3-celestial), see script/data/constellations.json.

Usage: python3 script/generate_star_map.py
Needs: numpy, Pillow
"""
import json
import math
import os

import numpy as np
from PIL import Image, ImageDraw

W, H = 2048, 1024
SS = 4            # supersampling, for smooth edges
STAR_SIZE = 1.4   # star edge in texels (square on the sphere, widened by 1/cos(dec) in x)
LINE_WIDTH = 1.1  # line width in texels
LINE_VALUE = 95   # line brightness in the G channel

HERE = os.path.dirname(__file__)
DATA = os.path.join(HERE, "data", "constellations.json")
OUTPUT = os.path.join(HERE, "..", "mcpack", "textures", "environment", "star_map.png")


def unit(ra, dec):
    ra, dec = math.radians(ra), math.radians(dec)
    return np.array([math.cos(dec) * math.cos(ra), math.cos(dec) * math.sin(ra), math.sin(dec)])


def to_px(ra, dec):
    return ra / 360.0 * W * SS, (dec / 180.0 + 0.5) * H * SS


def draw_lines(draw, constellations):
    width = max(1, round(LINE_WIDTH * SS))
    for con in constellations:
        for poly in con["lines"]:
            for (ra0, de0), (ra1, de1) in zip(poly, poly[1:]):
                a, b = unit(ra0, de0), unit(ra1, de1)
                w = math.acos(min(1.0, float(a @ b)))
                if w < 1e-4:
                    continue
                n = max(2, math.ceil(w / math.radians(2.0)))
                pts, prev = [], None
                for k in range(n + 1):
                    t = k / n
                    p = (a * math.sin((1 - t) * w) + b * math.sin(t * w)) / math.sin(w)
                    ra = math.atan2(p[1], p[0])
                    if prev is not None:  # keep the arc continuous across the 0h seam
                        while ra - prev > math.pi:
                            ra -= 2 * math.pi
                        while ra - prev < -math.pi:
                            ra += 2 * math.pi
                    prev = ra
                    pts.append((ra / (2 * math.pi) * W * SS, (math.asin(p[2]) / math.pi + 0.5) * H * SS))
                for shift in (-1, 0, 1, 2):
                    draw.line([(x + shift * W * SS, y) for x, y in pts], fill=LINE_VALUE, width=width)


def draw_stars(draw, stars):
    for ra, dec, mag in stars:
        v = max(0.55, min(1.0, 1.05 - 0.12 * max(mag - 1.0, 0.0)))
        w = STAR_SIZE / max(math.cos(math.radians(dec)), 0.12) * SS
        h = STAR_SIZE * SS
        x, y = to_px(ra, dec)
        for shift in (-W * SS, 0, W * SS):
            draw.rectangle([x + shift - w / 2, y - h / 2, x + shift + w / 2 - 1, y + h / 2 - 1], fill=round(255 * v))


def main():
    with open(DATA) as f:
        data = json.load(f)
    stars = Image.new("L", (W * SS, H * SS), 0)
    lines = Image.new("L", (W * SS, H * SS), 0)
    draw_lines(ImageDraw.Draw(lines), data["constellations"])
    draw_stars(ImageDraw.Draw(stars), data["stars"])
    r = np.asarray(stars.resize((W, H), Image.BOX))
    g = np.asarray(lines.resize((W, H), Image.BOX))
    rgba = np.zeros((H, W, 4), np.uint8)
    rgba[..., 0], rgba[..., 1], rgba[..., 3] = r, g, 255
    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    Image.fromarray(rgba, "RGBA").save(OUTPUT, optimize=True)
    print("wrote", os.path.normpath(OUTPUT), len(data["stars"]), "stars")


if __name__ == "__main__":
    main()
