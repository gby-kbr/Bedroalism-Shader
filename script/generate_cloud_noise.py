"""Generates the cloud noise texture used by the Sky and RenderChunk materials.

The texture is 256x256 RGBA and tiles seamlessly. R, G, B, A hold 4 octaves of noise, from low to high
frequency (about 3, 6, 12 and 24 cycles per tile), so a single texture fetch returns 4 octaves.

Real cumulus is billowy: round, cauliflower-like bulges that meet in creases. Smooth gaussian noise only gives
shapeless blobs. So every octave is built from Worley (cellular) noise: the distance F1 to the nearest of
jittered feature points, inverted, is 1.0 at the center of a cell and drops to the creases between the cells.
Each octave stacks the cell noise at 1x, 2x and 4x with falling weight, which adds smaller bulges on the
big ones, and the lowest octave is blended with smooth gaussian noise (Perlin-Worley) so the big shapes do
not read as a regular pattern of cells.

The cells are on a grid whose size divides the texture and the feature points wrap around, so it tiles
without a seam. At the end every channel is rank-matched to the distribution of the previous gaussian noise
(mean 0.5, spread 0.28), so the thresholds and the coverage of the shader keep working.

Usage: python3 script/generate_cloud_noise.py
Needs: numpy, Pillow
"""
import os

import numpy as np
from PIL import Image

SIZE = 256
OCTAVES = ((4, 1), (9, 2), (18, 3), (36, 4))  # (cells per tile side, seed)
SMOOTH_CYCLES = 3
OUTPUT = os.path.join(os.path.dirname(__file__), "..", "mcpack", "textures", "environment", "cloud_noise.png")


def worley_billow(cells, rng):
    """1 - F1 on a tileable grid of jittered points: round bulges separated by creases. Range 0 ~ 1."""
    pts = rng.random((cells, cells, 2))
    size = SIZE / cells
    ys, xs = np.mgrid[0:SIZE, 0:SIZE].astype(np.float32) + 0.5
    cy = (ys // size).astype(int)
    cx = (xs // size).astype(int)
    best = np.full((SIZE, SIZE), 1e9, np.float32)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            gy, gx = cy + dy, cx + dx
            py = (gy + pts[gy % cells, gx % cells, 0]) * size
            px = (gx + pts[gy % cells, gx % cells, 1]) * size
            best = np.minimum(best, (ys - py) ** 2 + (xs - px) ** 2)
    f1 = np.sqrt(best) / size
    return np.clip(1.0 - f1/0.8, 0.0, 1.0)


def smooth_noise(cycles, rng):
    spectrum = np.fft.fft2(rng.standard_normal((SIZE, SIZE)))
    freq = np.fft.fftfreq(SIZE) * SIZE
    radius = np.sqrt(freq[:, None] ** 2 + freq[None, :] ** 2)
    spectrum *= np.exp(-((radius / cycles) ** 2))
    noise = np.real(np.fft.ifft2(spectrum))
    return (noise - noise.mean()) / noise.std()


def rank_match(values):
    """Gives the values the distribution clip(0.5 + 0.28*z) of the former gaussian noise, keeping their order."""
    order = np.argsort(values.ravel(), kind="stable")
    ranks = np.empty(order.size)
    ranks[order] = (np.arange(order.size) + 0.5) / order.size
    # inverse normal CDF through erfinv
    from math import sqrt
    z = np.sqrt(2.0) * erfinv(2.0 * ranks - 1.0)
    return np.clip(0.5 + 0.28 * z, 0.0, 1.0).reshape(values.shape)


def erfinv(y):
    """Winitzki approximation refined by two Newton steps, enough for a texture."""
    a = 0.147
    ln = np.log(1.0 - y * y)
    t = 2.0 / (np.pi * a) + ln / 2.0
    x = np.sign(y) * np.sqrt(np.sqrt(t * t - ln / a) - t)
    from math import pi
    import math
    erf = np.vectorize(math.erf)
    for _ in range(2):
        x = x - (erf(x) - y) / (2.0 / np.sqrt(pi) * np.exp(-x * x))
    return x


def octave(cells, seed, smooth_mix):
    rng = np.random.default_rng(seed)
    v = np.zeros((SIZE, SIZE))
    total = 0.0
    for mult, weight in ((1, 0.62), (2, 0.28), (4, 0.10)):
        n = cells * mult
        if n > SIZE // 2:
            continue
        v += weight * worley_billow(n, rng)
        total += weight
    v /= total
    v = (v - v.mean()) / v.std()
    if smooth_mix > 0.0:
        v = (1.0 - smooth_mix) * v + smooth_mix * smooth_noise(SMOOTH_CYCLES, rng)
        v = (v - v.mean()) / v.std()
    return rank_match(v)


def main():
    channels = [octave(cells, seed, 0.35 if i == 0 else 0.0) for i, (cells, seed) in enumerate(OCTAVES)]
    pixels = (np.stack(channels, axis=-1) * 255.0 + 0.5).astype(np.uint8)
    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    Image.fromarray(pixels, "RGBA").save(OUTPUT, optimize=False)
    print("wrote", os.path.normpath(OUTPUT))


if __name__ == "__main__":
    main()
