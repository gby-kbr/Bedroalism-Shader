"""Generates textures/environment/weather.png, the rain and snow sprites of the Weather material.

64x64, the layout the game expects: snow sprites in the top row (8x8 cells), rain sprites below them
(8 columns, 8 px wide). The rest is empty, so the blue splash squares of the game are not drawn.
Rain: a thin streak per column with a soft head and a long fading tail, each column a little different.
Snow: soft round flakes of different sizes.
Usage: python3 script/generate_weather.py   (needs numpy and Pillow)
"""
import math
import os

import numpy as np
from PIL import Image

OUTPUT = os.path.join(os.path.dirname(__file__), "..", "mcpack", "textures", "environment", "weather.png")

# rain: (first row, last row, strength) per column, rows 8 to 39 are the rain tile
RAIN = [(11, 37, 0.80), (13, 38, 0.55), (9, 36, 0.90), (12, 38, 0.65), (10, 38, 0.85), (14, 35, 0.50), (11, 38, 0.75), (9, 38, 0.95)]
# snow: (radius in px, strength) per cell
SNOW = [(2.6, 0.95), (1.6, 0.8), (3.0, 0.7), (2.0, 0.9), (1.3, 0.75), (2.8, 0.85), (1.8, 0.95), (2.3, 0.65)]


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def main():
    a = np.zeros((64, 64), np.float32)

    for k, (y0, y1, strength) in enumerate(RAIN):
        for y in range(y0, y1 + 1):
            t = (y - y0) / (y1 - y0)
            along = smoothstep(0.0, 0.6, t) * (1.0 - 0.4 * smoothstep(0.85, 1.0, t))
            for x, w in ((2, 0.15), (3, 1.0), (4, 1.0), (5, 0.15)):  # two core columns and a faint edge
                a[y, 8 * k + x] = strength * along * w

    for k, (radius, strength) in enumerate(SNOW):
        for y in range(8):
            for x in range(8):
                d = math.hypot(x - 3.5, y - 3.5)
                a[y, 8 * k + x] = strength * (1.0 - smoothstep(radius * 0.3, radius, d))

    rgba = np.empty((64, 64, 4), np.uint8)
    rgba[..., :3] = 255  # white everywhere, so the filtering never blends in dark edges
    rgba[..., 3] = np.round(a * 255).astype(np.uint8)
    Image.fromarray(rgba, "RGBA").save(OUTPUT, optimize=True)
    print("wrote", os.path.normpath(OUTPUT))


if __name__ == "__main__":
    main()
