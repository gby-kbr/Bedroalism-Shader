"""Generates the caustic texture used by the RenderChunk material (underwater caustics).

The texture is 256x256 RGBA, covers 10x10 blocks and tiles seamlessly. It is not drawn by hand or made of
plain noise, it is simulated: a random sea surface is made from waves of wavelengths between about 0.8 and 10
blocks, and a grid of sun rays is sent straight down through it. Every ray is refracted at the surface with
Snell's law (n = 1.333), travels down to a given depth and lands on the bottom. Counting how many rays land on
each pixel gives the light density: thin bright lines where the surface focuses the light, dark where it
spreads it. All the rays are splatted with wrap-around, so the result tiles.

The four channels hold the same surface seen at four depths of water, 1.5, 3.5, 7 and 12 blocks: shallow
caustics are soft and wide, deep ones are sharp and folded.
Pixel = clamp(light density / 8), the shader multiplies by 8 again. The mean light density is 1.0.

Usage: python3 script/generate_caustic_noise.py
Needs: numpy, Pillow
"""
import os

import numpy as np
from PIL import Image

SIZE = 256            # texture size
SRC = 768             # rays per side: 9 rays per pixel
TILE_BLOCKS = 10.0    # size of the tile in blocks
DEPTHS = (1.5, 3.5, 7.0, 12.0)
INDEX = 1.333
RMS_SLOPE = 0.17      # steepness of the surface
OUTPUT = os.path.join(os.path.dirname(__file__), "..", "mcpack", "textures", "environment", "caustic_noise.png")


def surface(seed):
    """Height (blocks) of a tileable random sea surface on a SRC x SRC grid, and its slopes dh/dx, dh/dy."""
    rng = np.random.default_rng(seed)
    freq = np.fft.fftfreq(SRC) * SRC                       # cycles per tile
    fx, fy = np.meshgrid(freq, freq)
    k = np.sqrt(fx * fx + fy * fy)
    amp = np.where((k >= 1.0) & (k <= 13.0), np.maximum(k, 1.0) ** -1.3, 0.0)   # wavelengths 10 .. 0.8 blocks
    amp *= np.exp(-((k / 11.0) ** 4))
    spec = amp * (rng.standard_normal((SRC, SRC)) + 1j * rng.standard_normal((SRC, SRC)))
    kx = 2.0 * np.pi * fx / TILE_BLOCKS
    ky = 2.0 * np.pi * fy / TILE_BLOCKS
    h = np.real(np.fft.ifft2(spec))
    hx = np.real(np.fft.ifft2(1j * kx * spec))
    hy = np.real(np.fft.ifft2(1j * ky * spec))
    scale = RMS_SLOPE / np.sqrt(np.mean(hx * hx + hy * hy))
    return h * scale, hx * scale, hy * scale


def bottom_light(hx, hy, depth):
    """Light density on the bottom, SIZE x SIZE, mean 1.0."""
    nz = 1.0 / np.sqrt(1.0 + hx * hx + hy * hy)
    nx, ny = -hx * nz, -hy * nz
    eta = 1.0 / INDEX
    cosi = nz                                              # incident ray (0, 0, -1)
    kk = np.sqrt(1.0 - eta * eta * (1.0 - cosi * cosi))
    a = eta * cosi - kk
    rx, ry, rz = a * nx, a * ny, -eta + a * nz              # refracted direction, rz < 0
    off_x = depth * rx / -rz                                # where it lands, blocks
    off_y = depth * ry / -rz
    px = (np.arange(SRC)[None, :] + 0.5) / SRC * SIZE + off_x / TILE_BLOCKS * SIZE
    py = (np.arange(SRC)[:, None] + 0.5) / SRC * SIZE + off_y / TILE_BLOCKS * SIZE
    px = px - 0.5
    py = py - 0.5
    x0 = np.floor(px).astype(int)
    y0 = np.floor(py).astype(int)
    fx = px - x0
    fy = py - y0
    acc = np.zeros(SIZE * SIZE)
    for dy, wy in ((0, 1.0 - fy), (1, fy)):
        for dx, wx in ((0, 1.0 - fx), (1, fx)):
            idx = ((y0 + dy) % SIZE) * SIZE + ((x0 + dx) % SIZE)
            acc += np.bincount(idx.ravel(), weights=(wx * wy).ravel(), minlength=SIZE * SIZE)
    acc = acc.reshape(SIZE, SIZE)
    # tiny blur, tileable: removes the counting noise but not the thin lines
    f = np.fft.fftfreq(SIZE) * SIZE
    r2 = f[:, None] ** 2 + f[None, :] ** 2
    acc = np.real(np.fft.ifft2(np.fft.fft2(acc) * np.exp(-r2 * (2.0 * np.pi * 0.7 / SIZE) ** 2 / 2.0)))
    return acc / acc.mean()


def main():
    _, hx, hy = surface(7)
    channels = [np.clip(bottom_light(hx, hy, d) / 8.0, 0.0, 1.0) for d in DEPTHS]
    pixels = (np.stack(channels, axis=-1) * 255.0 + 0.5).astype(np.uint8)
    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    Image.fromarray(pixels, "RGBA").save(OUTPUT, optimize=False)
    print("wrote", os.path.normpath(OUTPUT))
    for i, d in enumerate(DEPTHS):
        c = channels[i] * 8.0
        print("depth", d, "mean %.2f p99 %.2f max %.2f" % (c.mean(), np.percentile(c, 99), c.max()))


if __name__ == "__main__":
    main()
