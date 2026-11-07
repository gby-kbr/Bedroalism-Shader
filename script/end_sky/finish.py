"""Glow of the black hole, tone mapping, dithering and the PNG."""
import numpy as np, sys
from PIL import Image
from scipy import ndimage
from common import *
import bh

# (sigma in degrees, weight, tint): tight white core, then a blue halo that spreads over the sky and the cloud tops
GLOW = ((0.35, 0.22, (1.00, 1.00, 1.00)),
        (1.2,  0.50, (0.86, 0.93, 1.00)),
        (4.0,  2.00, (0.60, 0.78, 1.00)),
        (11.0, 0.95, (0.40, 0.62, 1.00)),
        (28.0, 0.42, (0.30, 0.50, 0.95)))

def blur_wrap(img, sx, sy):
    """Gaussian blur of an equirectangular layer, wrapping in longitude. Wide blurs run on a reduced copy."""
    f = max(k for k in range(1, max(2, int(min(sx, sy)/8)) + 1) if img.shape[0] % k == 0 and img.shape[1] % k == 0)
    if f > 1:
        small = img.reshape(img.shape[0]//f, f, img.shape[1]//f, f).mean((1, 3))
        small = ndimage.gaussian_filter(small, (sy/f, sx/f), mode=('nearest', 'wrap'))
        return ndimage.zoom(small, f, order=1, mode='grid-wrap')
    return ndimage.gaussian_filter(img, (sy, sx), mode=('nearest', 'wrap'))

def finish(lin, emit, win, W, H, out_png, bloom=1.0, reflect=0.0):
    j0, j1, i0, i1 = win
    ppd = W/360.0
    e = emit.mean(-1).astype(np.float32)
    kx = 1.0/np.cos(BH_EL)                       # longitude is stretched at the elevation of the black hole
    glow = np.zeros_like(lin)
    for sigma_deg, wgt, tint in GLOW:
        g = blur_wrap(e, sigma_deg*ppd*kx, sigma_deg*ppd)
        glow += (g*wgt*bloom)[..., None]*np.array(tint, np.float32)
    # the shadow of the black hole stays dark: the glow lives outside of it
    D0 = bh.frame()[0]
    d, la, lo = grid(W, H)
    psi = np.arccos(np.clip(d @ D0, -1, 1))
    psi_s = np.arcsin(5.196*np.sqrt(1 - 2/bh.R0)/bh.R0)          # angular radius of the shadow
    keep = smoothstep(0.80*psi_s, 1.02*psi_s, psi)
    glow *= (0.04 + 0.96*keep)[..., None]
    x = lin + glow
    # tone: soft shoulder, keeps the colour of the highlights
    white = 5.0
    lum = x.max(-1, keepdims=True)
    y = np.clip(x*(1 + lum/(white*white))/(1 + lum), 0, 1)
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
