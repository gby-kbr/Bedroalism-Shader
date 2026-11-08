"""Generates mcpack/textures/environment/weather.png (64x64).

Layout (same regions the game uses):
  y 0..7   snow: 8 sprites of 8x8 px
  y 9..38  rain: 8 sprites of 8 px wide, 30 px tall
Everything else stays transparent. The color is white everywhere (also under the transparent pixels, so
bilinear filtering never pulls dark edges in); the shader tints it with the light of the scene.

Rain: a thin, motion-blurred drop, one pixel wide at the core, brightest in the middle and fading
at both ends. The 8 variants differ in length and brightness so the rain does not look like a pattern.
There is no speck/dot sprite.
Snow: round soft flakes of different sizes, no hard edges.
"""
import numpy as np
from PIL import Image

img = np.zeros((64, 64, 4), np.float32)
img[..., :3] = 1.0

# --- snow ---
yy, xx = np.mgrid[0:8, 0:8]
for i in range(8):
    r = 1.15 + 0.30*i                                  # flake radius in px, 1.15 .. 3.25
    d2 = ((xx - 3.5)**2 + (yy - 3.5)**2)/r**2
    a = np.exp(-2.2*d2)*(0.95 - 0.04*i)                # soft gaussian, no hard edge
    a[a < 0.01] = 0.0
    img[0:8, 8*i:8*i + 8, 3] = a

# --- rain ---
H = 30
lengths = [27, 21, 29, 17, 24, 30, 19, 25]
peaks = [0.85, 0.65, 0.95, 0.55, 0.78, 1.0, 0.60, 0.72]
shift = [0, 5, 0, 8, 3, 0, 6, 2]                       # start row inside the 30 rows
xs = np.arange(8)
for i in range(8):
    core = np.exp(-0.5*((xs - 3.5)/0.48)**2)           # about one pixel wide (pixels 3 and 4 each ~0.58)
    core /= core.max()
    L = lengths[i]
    t = (np.arange(H) - shift[i] + 0.5)/L
    prof = np.where((t > 0) & (t < 1), np.sin(np.pi*np.clip(t, 0, 1))**1.4, 0.0)
    img[9:9 + H, 8*i:8*i + 8, 3] = prof[:, None]*core[None, :]*peaks[i]*0.92

out = (np.clip(img, 0, 1)*255 + 0.5).astype(np.uint8)
Image.fromarray(out, 'RGBA').save('mcpack/textures/environment/weather.png')
print('saved', out.shape)
