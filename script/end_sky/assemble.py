import numpy as np, time, sys, os
from PIL import Image
from scipy import ndimage
from common import *
import space, clouds, bh

W, H = int(sys.argv[1]) if len(sys.argv) > 1 else 4032, None
H = W//2
t0 = time.time()
def log(m): print(f'[{time.time()-t0:6.1f}s] {m}', flush=True)

# 1. deep sky, stars rendered at twice the resolution so lensed stars keep their brightness
d, la, lo = grid(W, H)
smooth = space.sky_gradient(la)
neb, band = space.nebula(W, H); smooth = (smooth + neb).astype(np.float32)
bg2 = space.up2(smooth) + space.stars(2*W, 2*H, scale=2.0)
bg = bg2.reshape(H, 2, W, 2, 3).mean((1, 3)).astype(np.float32); log('space')

# 2. black hole window: every pixel within 60 degrees of the black hole is traced
noise = tile_noise(512, 1.2, 9)
az0, el0 = np.degrees(BH_AZ), np.degrees(BH_EL)
i0 = int(((az0 - 62)/360 + 0.5)*W); i1 = int(((az0 + 62)/360 + 0.5)*W)
j0 = int((0.5 - (el0 + 52)/180)*H); j1 = int((0.5 - (el0 - 52)/180)*H)
ii, jj = np.meshgrid(np.arange(i0, i1), np.arange(j0, j1))
def dirs_at(ox, oy):
    lon = ((ii + ox)/W - 0.5)*2*np.pi; lat = (0.5 - (jj + oy)/H)*np.pi
    return np.stack([np.cos(lat)*np.sin(lon), np.sin(lat), np.cos(lat)*np.cos(lon)], -1)
D0, right, up = bh.frame()
d0 = dirs_at(0.5, 0.5)
psi = np.degrees(np.arccos(np.clip(d0 @ D0, -1, 1)))
core = psi < 50.0
inner = psi < 30.0
res = np.zeros(ii.shape + (3,), np.float32); emit = np.zeros(ii.shape + (3,), np.float32)
# outer ring: one sample per pixel, inner part: 2 x 2 samples
dd = dirs_at(0.5, 0.5)[core & ~inner]
col, em, ring, p0, st = bh.trace(dd, bg2, noise)
res[core & ~inner] = col; emit[core & ~inner] = em; log('bh outer')
SS = 3
offs = [((a + 0.5)/SS, (b + 0.5)/SS) for a in range(SS) for b in range(SS)]
for ox, oy in offs:
    dd = dirs_at(ox, oy)[inner]
    col, em, ring, p0, st = bh.trace(dd, bg2, noise)
    res[inner] += (col/len(offs)).astype(np.float32); emit[inner] += (em/len(offs)).astype(np.float32)
    log(f'bh inner sample {ox:.2f},{oy:.2f}')
wblend = (1 - smoothstep(40.0, 50.0, psi))[..., None].astype(np.float32)
win = bg[j0:j1, i0:i1]
space_img = bg.copy()
space_img[j0:j1, i0:i1] = win*(1 - wblend) + np.where(core[..., None], res, win)*wblend
emit_full = np.zeros_like(bg); emit_full[j0:j1, i0:i1] = emit*wblend
log('bh composed')

# 3. atmosphere limb above the horizon
el = np.degrees(la)
cb = np.cos(np.arccos(np.clip(d @ D0, -1, 1)))
ang_bh = np.degrees(np.arccos(np.clip(d @ D0, -1, 1)))
near = np.exp(-(ang_bh/38.0)**2)
above = (el > 0)
limb = (np.array([0.20, 0.46, 0.95], np.float32)*(0.95*np.exp(-np.maximum(el, 0)/1.3))[..., None]
        + np.array([0.035, 0.085, 0.22], np.float32)*(np.exp(-np.maximum(el, 0)/8.0))[..., None])
limb *= (1 + 3.5*near)[..., None]
space_img = space_img + np.where(above[..., None], limb, 0)
log('limb')

# 4. clouds below the horizon
tL = tile_noise(512, 1.5, 1); tD = tile_noise(512, 1.3, 2)
tiles = (mip_chain(tL, 8), mip_chain(tD, 8))
out = space_img
CH = 144
for r0 in range(H//2, H, CH):
    r1 = min(r0 + CH, H)
    rgb, below = clouds.cloud_layer(W, H, (r0, r1), tiles)
    out[r0:r1] = np.where(below[..., None] > 0, rgb, out[r0:r1])
    log(f'clouds rows {r0}-{r1}')
np.save('linear_nobloom.npy', out); np.save('emit.npy', emit_full); np.save('win.npy', np.array([j0, j1, i0, i1]))
log('saved')
