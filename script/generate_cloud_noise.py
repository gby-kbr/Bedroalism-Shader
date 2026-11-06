"""Generates the cloud noise texture used by the Sky material.

The texture is 256x256 RGBA and tiles seamlessly. R, G, B, A hold 4 octaves of noise, from low to high
frequency (3, 6, 12 and 24 cycles per tile), so a single texture fetch returns 4 octaves.

Each octave is white noise low-pass filtered in frequency space with a gaussian. Filtering in frequency
space makes the result periodic, so it tiles without a seam.

Usage: python3 tools/generate_cloud_noise.py
Needs: numpy, Pillow
"""
import os

import numpy as np
from PIL import Image

SIZE = 256
OCTAVES = ((3, 1), (6, 2), (12, 3), (24, 4))  # (cycles per tile, seed)
OUTPUT = os.path.join(os.path.dirname(__file__), "..", "mcpack", "textures", "bedroalism", "cloud_noise.png")


def tileable_noise(cycles, seed):
    rng = np.random.default_rng(seed)
    spectrum = np.fft.fft2(rng.standard_normal((SIZE, SIZE)))
    freq = np.fft.fftfreq(SIZE) * SIZE
    radius = np.sqrt(freq[:, None] ** 2 + freq[None, :] ** 2)
    spectrum *= np.exp(-((radius / cycles) ** 2))
    noise = np.real(np.fft.ifft2(spectrum))
    noise = (noise - noise.mean()) / noise.std()
    return np.clip(0.5 + 0.28 * noise, 0.0, 1.0)


def main():
    channels = [tileable_noise(cycles, seed) for cycles, seed in OCTAVES]
    pixels = (np.stack(channels, axis=-1) * 255.0 + 0.5).astype(np.uint8)
    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    Image.fromarray(pixels, "RGBA").save(OUTPUT, optimize=False)
    print("wrote", os.path.normpath(OUTPUT))


if __name__ == "__main__":
    main()
