"""Generates the noise texture used by the cloud layers of the Sky material.

256x256 RGBA, tiles seamlessly. Every channel is histogram-equalized (uniform 0..1), so a threshold in the
shader is also the fraction of the sky that stays clear.
  R  coverage: large soft blobs, decides where the clouds are.
  G  puffs: inverted Worley noise (3 octaves), the rounded cumulus bulges.
  B  fine detail: small ragged edges.

Usage: python3 script/generate_cloud_noise.py
Needs: numpy, Pillow
"""
import os

import numpy as np
from PIL import Image

N = 256
OUTPUT = os.path.join(os.path.dirname(__file__), "..", "mcpack", "textures", "environment", "cloud_noise.png")


def equalize(a):
    r = np.argsort(np.argsort(a.ravel()))
    return (r / (r.size - 1.0)).reshape(a.shape)


def spectral(rng, beta, kmin, kmax, stretch=1.0):
    """Tileable noise with a power law spectrum k^-beta. stretch > 1 elongates the features along x."""
    f = np.fft.fftfreq(N) * N
    fx, fy = np.meshgrid(f, f)
    k = np.sqrt((fx * stretch) ** 2 + fy ** 2)
    amp = np.where((k >= kmin) & (k <= kmax), np.maximum(k, 1e-3) ** (-beta / 2.0), 0.0)
    spec = amp * (rng.standard_normal((N, N)) + 1j * rng.standard_normal((N, N)))
    return np.real(np.fft.ifft2(spec))


def worley(rng, cells):
    """Tileable Worley F1 distance (0 at a feature point), normalized by the cell size."""
    pts = rng.random((cells, cells, 2))
    ys, xs = np.mgrid[0:N, 0:N] / N * cells
    cy, cx = np.floor(ys).astype(int), np.floor(xs).astype(int)
    best = np.full((N, N), 9.0)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            iy, ix = cy + dy, cx + dx
            p = pts[iy % cells, ix % cells]
            d = np.hypot(ix + p[..., 0] - xs, iy + p[..., 1] - ys)
            best = np.minimum(best, d)
    return best


def main():
    rng = np.random.default_rng(7)
    coverage = equalize(spectral(rng, 2.6, 1, 10))

    puffs = np.zeros((N, N))
    for i, cells in enumerate((6, 12, 24)):
        puffs += 0.5 ** i * (1.0 - np.clip(worley(rng, cells), 0, 1))
    puffs = equalize(puffs)

    detail = equalize(spectral(rng, 1.2, 14, 110))

    out = np.stack([coverage, puffs, detail, np.ones((N, N))], axis=-1)
    Image.fromarray((out * 255 + 0.5).astype(np.uint8), "RGBA").save(OUTPUT)
    print("wrote", os.path.relpath(OUTPUT))


if __name__ == "__main__":
    main()
