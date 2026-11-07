"""Bloom of the black hole, tone mapping, dithering and the PNG."""
import numpy as np, sys
from PIL import Image
from scipy import ndimage
from common import *

def finish(lin, emit, win, W, H, out_png, bloom=1.0, reflect=0.0):
    j0, j1, i0, i1 = win
    ppd = W/360.0
    e = emit[j0:j1, i0:i1].mean(-1)
    glow = np.zeros_like(lin)
    # the glow of the lens and the air around the very bright disk: three soft widths
    pad = 260
    big = np.zeros((e.shape[0] + 2*pad, e.shape[1] + 2*pad), np.float32); big[pad:-pad, pad:-pad] = e
    tint = np.array([0.62, 0.80, 1.0], np.float32)
    for sigma_deg, wgt in ((0.5, 0.12), (1.8, 0.20), (6.0, 0.18), (14.0, 0.12)):
        g = ndimage.gaussian_filter(big, sigma_deg*ppd, mode='constant')[pad:-pad, pad:-pad]
        glow[j0:j1, i0:i1] += (g*wgt*bloom)[..., None]*tint
    # faint mirror image of the black hole on the cloud tops, as on a wet surface: much weaker and softer
    full = np.zeros((H, W), np.float32); full[j0:j1, i0:i1] = e
    mirror = full[::-1].copy(); mirror[:H//2] = 0
    mirror = ndimage.gaussian_filter(mirror, (0.30*ppd, 0.22*ppd))
    cl = lin[H//2:].mean(-1)
    glow[H//2:] += (mirror[H//2:]*reflect*np.clip(cl, 0.15, 1.2))[..., None]*tint
    # the shadow of the black hole stays black: the glow only lives outside of it
    import bh
    d = np.stack([np.cos(np.pi*(0.5 - (np.arange(j0, j1) + 0.5)/H))[:, None]*np.sin(((np.arange(i0, i1) + 0.5)/W - 0.5)*2*np.pi)[None, :],
                  np.sin(np.pi*(0.5 - (np.arange(j0, j1) + 0.5)/H))[:, None]*np.ones((1, i1 - i0)),
                  np.cos(np.pi*(0.5 - (np.arange(j0, j1) + 0.5)/H))[:, None]*np.cos(((np.arange(i0, i1) + 0.5)/W - 0.5)*2*np.pi)[None, :]], -1)
    D0 = bh.frame()[0]
    psi = np.arccos(np.clip(d @ D0, -1, 1))
    psi_s = np.arcsin(5.196*np.sqrt(1 - 2/bh.R0)/bh.R0)                  # angular radius of the shadow
    keep = smoothstep(0.80*psi_s, 1.02*psi_s, psi)
    glow[j0:j1, i0:i1] *= (0.06 + 0.94*keep)[..., None]
    x = lin + glow
    x = x*1.0
    # tone: soft shoulder, keeps the colour of the highlights
    white = 5.0
    lum = x.max(-1, keepdims=True)
    scale = (1 + lum/(white*white))/(1 + lum)
    y = x*scale
    y = np.clip(y, 0, 1)
    srgb = np.where(y <= 0.0031308, 12.92*y, 1.055*np.power(y, 1/2.4) - 0.055)
    rng = np.random.default_rng(3)
    srgb = srgb + (rng.random(srgb.shape, dtype=np.float32) - 0.5)/255.0       # dither, hides banding in the dark sky
    img = (np.clip(srgb, 0, 1)*255 + 0.5).astype(np.uint8)
    Image.fromarray(img, 'RGB').save(out_png, optimize=False)
    return img

if __name__ == '__main__':
    W = int(sys.argv[1]); H = W//2
    lin = np.load('linear_nobloom.npy'); emit = np.load('emit.npy'); win = np.load('win.npy')
    finish(lin, emit, win, W, H, sys.argv[2])
