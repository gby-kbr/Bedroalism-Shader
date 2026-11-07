"""Shared pieces of the End sky generator: the equirectangular grid, tileable noise and sampling helpers."""
import numpy as np
from scipy import ndimage

BH_AZ = np.radians(-72.0)    # azimuth of the black hole, reference: 30% from the left edge
BH_EL = np.radians(27.0)     # elevation of its centre: the whole black hole stays above the horizon

def grid(W, H, rows=None):
    """Directions of the pixel centres. x east, y up, z north, longitude 0 = +z. Shape (rows, W, 3)."""
    j0, j1 = rows if rows else (0, H)
    lon = ((np.arange(W) + 0.5)/W - 0.5)*2*np.pi
    lat = (0.5 - (np.arange(j0, j1) + 0.5)/H)*np.pi
    lo, la = np.meshgrid(lon, lat)
    d = np.stack([np.cos(la)*np.sin(lo), np.sin(la), np.cos(la)*np.cos(lo)], -1).astype(np.float32)
    return d, la.astype(np.float32), lo.astype(np.float32)

def tile_noise(n, beta, seed):
    """Periodic noise with a power-law spectrum (amplitude ~ f^-beta), turned into a uniform distribution by rank."""
    rng = np.random.default_rng(seed)
    f = np.fft.fftfreq(n)
    fx, fy = np.meshgrid(f, f)
    r = np.sqrt(fx*fx + fy*fy); r[0, 0] = 1.0
    spec = np.fft.fft2(rng.standard_normal((n, n)))*r**(-beta)
    spec[0, 0] = 0
    a = np.fft.ifft2(spec).real
    rank = np.argsort(np.argsort(a.ravel())).reshape(n, n)
    return (rank/(n*n - 1)).astype(np.float32)

def mip_chain(t, levels):
    out = [t]
    for _ in range(levels):
        t = 0.25*(t[0::2, 0::2] + t[1::2, 0::2] + t[0::2, 1::2] + t[1::2, 1::2])
        out.append(t)
    return out

def sample_tile(chain, u, v, lod):
    """Trilinear sample of a periodic mip chain. u, v in texels of level 0, lod per sample (float)."""
    lod = np.clip(lod, 0, len(chain) - 1.001)
    l0 = np.floor(lod).astype(np.int32); fr = lod - l0
    out = np.zeros(u.shape, np.float32)
    for L in np.unique(l0):
        m = l0 == L
        for lev, w in ((L, 1 - fr[m]), (L + 1, fr[m])):
            if lev >= len(chain): continue
            s = 2.0**lev
            c = np.stack([v[m]/s - 0.5, u[m]/s - 0.5])
            out[m] += w*ndimage.map_coordinates(chain[lev], c, order=1, mode='grid-wrap').astype(np.float32)
    return out

def smoothstep(a, b, x):
    t = np.clip((x - a)/(b - a), 0, 1)
    return t*t*(3 - 2*t)
