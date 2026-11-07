"""Cloud deck seen from above, lit by the light of the black hole and the sky. Everything is a function of the
view direction, so the left and right edge of the panorama match by construction."""
import numpy as np
from common import *

def cloud_layer(W, H, rows, tiles, quality=1.0):
    """Returns (rgb, alpha) for the rows given, directions below the horizon only (others get alpha 0)."""
    d, la, lo = grid(W, H, rows)
    s = np.maximum(-d[..., 1], 1e-4)                       # how steeply the ray goes down
    below = d[..., 1] < -1e-5
    A, TH = 1.0, 0.30                                       # depth of the cloud tops below the camera, thickness
    dpx = 2*np.pi/W                                         # angle of one pixel
    steps = int(20*quality)
    chainL, chainD = tiles
    th, k = 0.30, 0.34

    # direction of the light from the black hole (horizontal part used for the shadow offset)
    L = np.array([np.cos(BH_EL)*np.sin(BH_AZ), np.sin(BH_EL), np.cos(BH_EL)*np.cos(BH_AZ)], np.float32)
    Lxz = L[[0, 2]]/np.linalg.norm(L[[0, 2]])
    facing = np.clip(d @ L, 0, 1)                           # view toward the black hole

    rgb = np.zeros(d.shape, np.float32); T = np.ones(s.shape, np.float32)
    ij = np.indices(s.shape)
    # white-noise jitter of the start of the ray, hides the steps without a visible pattern
    jitter = np.random.default_rng(1000 + rows[0]).random(s.shape).astype(np.float32)
    dist_top = A/s

    c37, s37 = np.cos(0.65), np.sin(0.65)
    def height(xz, lod_shift=0.0):
        # lumps from three scales of the low octave tile (rotated and offset so the tile does not repeat),
        # billows from the detail tile. scale: texels per unit
        fp = dist_top*dpx/s                                # footprint on the cloud plane (units)
        x, z = xz[0], xz[1]
        xr, zr = c37*x - s37*z, s37*x + c37*z
        def lod(sc): return np.log2(np.maximum(fp*sc, 1e-3)) + lod_shift
        l1 = sample_tile(chainL, x*10.0, z*10.0, lod(10.0))
        l2 = sample_tile(chainL, xr*28.0 + 200.0, zr*28.0 + 90.0, lod(28.0))
        l3 = sample_tile(chainL, x*70.0 - 310.0, z*70.0 + 40.0, lod(70.0))
        low = 0.40*l1 + 0.35*l2 + 0.25*l3
        det = sample_tile(chainD, xr*160.0 + 123.0, zr*160.0 - 77.0, lod(160.0))
        return low, det

    for i in range(steps):
        hf = (i + jitter)/steps                            # 0 base .. 1 top; march from the top down
        hf = 1.0 - hf
        depth = A + (1 - hf)*TH                            # distance below the camera plane
        xz = np.stack([d[..., 0]/s*depth, d[..., 2]/s*depth])
        low, det = height(xz)
        Hc = np.clip((low - th)/k, 0, 1)**0.8
        edge = Hc + 0.55*(det - 0.5)*smoothstep(0.0, 0.35, Hc)
        occ = smoothstep(0.0, 0.18, edge - hf)
        depthin = np.clip((edge - hf)/0.45, 0, 1)
        ds = (TH/steps)/s
        a = occ*(1 - np.exp(-ds*7.0))
        a = np.where(below, a, 0)
        # shadow: is the neighbour toward the light taller than this point
        lowS, _ = height(xz + Lxz[:, None, None]*0.07)
        Hs = np.clip((lowS - th)/k, 0, 1)**0.8
        shadow = np.clip(1 - (Hs - hf)*2.6, 0.08, 1)
        top = hf**0.7
        amb = np.array([0.34, 0.45, 0.70], np.float32)*(0.55 + 0.45*top)[..., None]
        skyl = np.array([1.00, 0.94, 0.84], np.float32)*(0.18 + 0.82*top**1.6)[..., None]*(1 - 0.75*depthin)[..., None]
        bh = np.array([0.55, 0.78, 1.25], np.float32)*((0.10 + 2.4*facing**2.0)*shadow*(0.30 + 0.70*top))[..., None]
        col = (amb*0.70 + skyl*2.7 + bh)*(0.55 + 0.85*det)[..., None]
        rgb += col*(T*a)[..., None]
        T *= (1 - a)

    alpha = 1 - T
    col = rgb/np.maximum(alpha, 1e-4)[..., None]
    # aerial perspective: far cloud fades into the blue haze of the atmosphere
    haze = np.array([0.16, 0.30, 0.58], np.float32)
    w = (1 - np.exp(-0.022*dist_top))[..., None]
    col = col*(1 - w) + haze*w
    # the gaps show the dark, hazy air of the planet below
    ground = np.array([0.020, 0.035, 0.075], np.float32)*(1 - w) + haze*0.55*w
    full = col*alpha[..., None] + ground*(1 - alpha)[..., None]
    return np.where(below[..., None], full, 0).astype(np.float32), below.astype(np.float32)
