"""Deep sky: dark gradient, a faint Milky Way with dust, stars of all sizes. No clouds, no black hole."""
import numpy as np
from scipy import ndimage
from common import *

def noise3(chain3, p, octaves, gain=0.5, lac=2.0, f0=1.0):
    """Fractal sum of periodic 3D value noise. p: (..., 3) float array."""
    out = np.zeros(p.shape[:-1], np.float32); amp, tot, f = 1.0, 0.0, f0
    for o in range(octaves):
        q = (p*f + 7.31*o).reshape(-1, 3).T
        out += amp*ndimage.map_coordinates(chain3, q, order=1, mode='grid-wrap').reshape(p.shape[:-1]).astype(np.float32)
        tot += amp; amp *= gain; f *= lac
    return out/tot

def lattice(n, seed):
    return np.random.default_rng(seed).random((n, n, n)).astype(np.float32)

def sky_gradient(la):
    el = np.degrees(la)
    up = np.clip(el, 0, 90)
    zen = np.array([0.0006, 0.0011, 0.0036], np.float32)
    hor = np.array([0.0040, 0.0100, 0.0360], np.float32)
    t = (1 - np.exp(-up/14.0))[..., None]
    return zen*(1 - t) + hor*t

def nebula(W, H, seed=11):
    """Faint Milky Way: a band of soft clouds with dark dust lanes, computed at half resolution."""
    w, h = W//2, H//2
    d, la, lo = grid(w, h)
    lat = lattice(64, seed)
    # the band is a great circle tilted against the horizon
    n_g = np.array([0.30, 0.78, -0.55], np.float32); n_g /= np.linalg.norm(n_g)
    sd = d @ n_g
    warp = noise3(lat, d*2.2 + 3.0, 3)
    band_c = np.arcsin(np.clip(sd + 0.10*(warp - 0.5), -1, 1))
    band = np.exp(-(band_c/0.30)**2)
    cloudy = noise3(lat, d*3.4 + 1.7*warp[..., None], 6)
    fine = noise3(lat, d*11.0 + 5.0, 4)
    dens = np.clip((cloudy - 0.42)*2.6, 0, 1)*band
    dust = np.clip((noise3(lat, d*6.5 + 21.0, 5) - 0.50)*3.0, 0, 1)*band
    col1 = np.array([0.30, 0.38, 0.85], np.float32)   # blue
    col2 = np.array([0.62, 0.30, 0.58], np.float32)   # violet-pink
    col3 = np.array([0.58, 0.34, 0.20], np.float32)   # warm
    mix = noise3(lat, d*2.0 + 40.0, 3)
    c = col1*(1 - mix)[..., None] + col2*mix[..., None]
    c = c*(1 - 0.5*smoothstep(0.55, 0.85, mix))[..., None] + col3*smoothstep(0.55, 0.85, mix)[..., None]*0.5
    neb = c*(dens*(0.55 + 0.9*fine)*0.045)[..., None]
    neb += np.array([0.50, 0.55, 0.75], np.float32)*(band**2*0.0045*(0.5 + fine))[..., None]   # unresolved starlight
    neb *= (1 - 0.7*dust)[..., None]
    pad = 4
    padded = np.pad(neb, ((0, 0), (pad, pad), (0, 0)), mode='wrap')
    yy, xx = np.meshgrid(np.clip((np.arange(H) + 0.5)/2 - 0.5, 0, h - 1), (np.arange(W) + 0.5)/2 - 0.5 + pad, indexing='ij')
    c = np.stack([yy, xx])
    out = np.stack([ndimage.map_coordinates(padded[..., k], c, order=1, mode='nearest') for k in range(3)], -1)
    return out.astype(np.float32), band

def stars(W, H, seed=5, density=1.0, scale=1.0):
    rng = np.random.default_rng(seed)
    img = np.zeros((H, W, 3), np.float32)
    n_faint = int(21000*density)
    # magnitudes: more faint stars than bright ones, N ~ 10^(0.45 m)
    u = rng.random(n_faint)
    m = np.log10(10**(0.45*1.5) + u*(10**(0.45*6.8) - 10**(0.45*1.5)))/0.45
    m = np.concatenate([m, rng.uniform(-0.2, 1.6, 90)])
    n = len(m)
    sinlat = rng.uniform(-1, 1, n)
    lat = np.arcsin(sinlat); lon = rng.uniform(-np.pi, np.pi, n)
    keep = lat > np.radians(-1.0)
    m, lat, lon = m[keep], lat[keep], lon[keep]
    n = len(m)
    peak = 0.85*(10**(-0.4*m))**0.45
    sig = (0.60 + 0.55*np.clip((2.4 - m)/2.4, 0, 1)**1.5)*scale
    # colors from blue-white to orange
    temp = rng.random(n)
    pal = np.array([[0.70, 0.80, 1.00], [0.90, 0.93, 1.00], [1.00, 0.98, 0.92], [1.00, 0.88, 0.70], [1.00, 0.74, 0.55]], np.float32)
    idx = np.clip((temp**1.3*4.0), 0, 3.999)
    i0 = idx.astype(int); fr = (idx - i0)[:, None]
    col = pal[i0]*(1 - fr) + pal[np.minimum(i0 + 1, 4)]*fr
    px = (lon/(2*np.pi) + 0.5)*W - 0.5
    py = (0.5 - lat/np.pi)*H - 0.5
    for k in range(n):
        cl = max(np.cos(lat[k]), 0.07)
        sx, sy = sig[k]/cl, sig[k]
        big = m[k] < 1.2
        hx, hy = int(min(np.ceil((14 if big else 4)*sx), 800)), int(np.ceil((14 if big else 4)*sy))
        x0, y0 = int(round(px[k])), int(round(py[k]))
        ys = np.arange(max(y0 - hy, 0), min(y0 + hy + 1, H))
        if len(ys) == 0: continue
        xs = np.arange(x0 - hx, x0 + hx + 1)
        g = np.exp(-0.5*(((xs - px[k])/sx)**2)[None, :] - 0.5*(((ys - py[k])/sy)**2)[:, None])
        img[ys[:, None], (xs % W)[None, :]] += (peak[k]*g)[..., None]*col[k]
        if m[k] < 1.2:                                          # the brightest: a soft halo and short diffraction spikes
            a = 0.10*(1.2 - m[k])
            hs = hx*3
            gx = np.exp(-np.abs(xs - px[k])/(4.0*sx))
            gy = np.exp(-np.abs(ys - py[k])/(4.0*sy))
            halo = np.exp(-0.5*(((xs - px[k])/(3.2*sx))**2)[None, :] - 0.5*(((ys - py[k])/(3.2*sy))**2)[:, None])
            spike = (np.exp(-np.abs(xs - px[k])/(7.0*sx))[None, :]*np.exp(-0.5*((ys - py[k])/(0.45*sy))**2)[:, None]
                     + np.exp(-np.abs(ys - py[k])/(7.0*sy))[:, None]*np.exp(-0.5*((xs - px[k])/(0.45*sx))**2)[None, :])
            img[ys[:, None], (xs % W)[None, :]] += (a*(halo + 0.35*spike))[..., None]*col[k]
    return img

def up2(img):
    """Bilinear 2x upsample that wraps around in longitude."""
    h, w = img.shape[:2]
    pad = 4
    padded = np.pad(img, ((0, 0), (pad, pad), (0, 0)), mode='wrap')
    yy, xx = np.meshgrid(np.clip((np.arange(2*h) + 0.5)/2 - 0.5, 0, h - 1), (np.arange(2*w) + 0.5)/2 - 0.5 + pad, indexing='ij')
    c = np.stack([yy, xx])
    return np.stack([ndimage.map_coordinates(padded[..., k], c, order=1, mode='nearest') for k in range(3)], -1).astype(np.float32)
