"""Cloud deck seen from above, lit by the light of the black hole and the sky. Everything is a function of the
view direction, so the left and right edge of the panorama match by construction."""
import numpy as np
from common import *

import os
BUMP = float(os.environ.get("BR_BUMP", "8"))
COLDET = float(os.environ.get("BR_COLDET", "0.6"))
EDGEAMP = float(os.environ.get("BR_EDGE", "0.35"))
OPAC = float(os.environ.get("BR_OPAC", "28"))
SHK = float(os.environ.get("BR_SHK", "1"))

def cloud_layer(W, H, rows, tiles, quality=1.0):
    """Returns (rgb, alpha) for the rows given, directions below the horizon only (others get alpha 0)."""
    d, la, lo = grid(W, H, rows)
    s = np.maximum(-d[..., 1], 1e-4)                       # how steeply the ray goes down
    below = d[..., 1] < -1e-5
    A, TH = 1.0, 0.30                                       # depth of the cloud tops below the camera, thickness
    dpx = 2*np.pi/W                                         # angle of one pixel
    steps = int(36*quality)
    chainL, chainD = tiles
    th, k = 0.30, float(os.environ.get('BR_K', '0.46'))

    # direction of the light from the black hole (horizontal part used for the shadow offset)
    L = np.array([np.cos(BH_EL)*np.sin(BH_AZ), np.sin(BH_EL), np.cos(BH_EL)*np.cos(BH_AZ)], np.float32)
    Lh, Ly = np.cos(BH_EL), np.sin(BH_EL)
    Lxz = L[[0, 2]]/np.linalg.norm(L[[0, 2]])
    SHADOW_DIST = (0.04, 0.09, 0.16, 0.26, 0.40, 0.58, 0.80)
    TH_SHADOW = 0.80                                        # thickness used for the shadow rays: long, soft shadows
    facing = np.clip(d @ L, 0, 1)                           # view toward the black hole

    rgb = np.zeros(d.shape, np.float32); T = np.ones(s.shape, np.float32)
    ij = np.indices(s.shape)
    # white-noise jitter of the start of the ray, hides the steps without a visible pattern
    jitter = np.random.default_rng(1000 + rows[0]).random(s.shape).astype(np.float32)
    dist_top = A/s

    c37, s37 = np.cos(0.65), np.sin(0.65)
    def height(xz, lod_shift=0.0, low_only=False):
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
        if low_only: return low, None
        return low, detail(xz, fp, lod_shift)

    def detail(xz, fp, lod_shift=0.0):
        x, z = xz[0], xz[1]
        xr, zr = c37*x - s37*z, s37*x + c37*z
        def lod(sc): return np.log2(np.maximum(fp*sc, 1e-3)) + lod_shift
        det = sample_tile(chainD, xr*160.0 + 123.0, zr*160.0 - 77.0, lod(160.0))
        # two finer octaves: keep the cloud surface crisp at the pixel scale instead of magnifying the coarse tile
        det2 = sample_tile(chainD, x*480.0 + 51.0, z*480.0 + 301.0, lod(480.0))
        det3 = sample_tile(chainD, xr*1100.0 - 220.0, zr*1100.0 + 17.0, lod(1100.0))
        return np.clip(0.5 + 1.15*(0.62*(det - 0.5) + 0.30*(det2 - 0.5) + 0.20*(det3 - 0.5)), 0, 1)

    for i in range(steps):
        hf = (i + jitter)/steps                            # 0 base .. 1 top; march from the top down
        hf = 1.0 - hf
        depth = A + (1 - hf)*TH                            # distance below the camera plane
        xz = np.stack([d[..., 0]/s*depth, d[..., 2]/s*depth])
        low, det = height(xz)
        Hc = np.clip((low - th)/k, 0, 1)**0.8
        edge = Hc + EDGEAMP*(det - 0.5)*smoothstep(0.0, 0.35, Hc)
        occ = smoothstep(0.0, 0.10, edge - hf)
        depthin = np.clip((edge - hf)/0.45, 0, 1)
        ds = (TH/steps)/s
        a = occ*(1 - np.exp(-ds*OPAC))
        a = np.where(below, a, 0)
        # shadow: march from this point toward the black hole; the ray climbs by Ly/Lh per unit of horizontal distance
        # and is blocked where the cloud height field is above it (soft, several distances)
        climb = Ly/Lh/TH_SHADOW                                    # height fraction gained per unit of horizontal distance
        blocked = np.zeros(s.shape, np.float32)
        hs0 = 0.5*hf + 0.5*np.minimum(Hc, hf + 0.15)               # smooth height of the shaded point (no layer noise)
        for sd_ in SHADOW_DIST:
            lowS, _ = height(xz + Lxz[:, None, None]*sd_, low_only=True)
            Hs = 1.5*SHK*np.clip((lowS - th)/k, 0, 1)**0.8
            blocked = np.maximum(blocked, np.clip((Hs - (hs0 + sd_*climb))*4.0 + 0.05, 0, 1)*np.clip(Hs*3.0, 0, 1))
        # big cloud masses shadow their neighbours far away (low octave only, long distances)
        for sd_ in (1.2, 2.4, 4.0):
            lowB, _ = height(xz + Lxz[:, None, None]*sd_, low_only=True)
            HB = np.clip((lowB - th)/k, 0, 1)**0.8
            blocked = np.maximum(blocked, 0.8*np.clip((HB - (hs0 + sd_*climb*0.25))*3.0 + 0.05, 0, 1)*np.clip(HB*3.0, 0, 1))
        shadow = 1 - 0.98*blocked
        lowN, _ = height(xz + Lxz[:, None, None]*0.045, low_only=True)
        HN = np.clip((lowN - th)/k, 0, 1)**0.8
        slope = np.clip(0.60 + (Hc - HN)*12.0, 0.05, 1.7)    # surface facing the light is brighter
        # fine surface relief: the small bumps facing the light are bright, the ones facing away are dark
        fp_ = dist_top*dpx/s
        d1 = lambda p: sample_tile(chainD, (c37*p[0] - s37*p[1])*160.0 + 123.0, (s37*p[0] + c37*p[1])*160.0 - 77.0,
                                   np.log2(np.maximum(fp_*160.0, 1e-3)) + 1.0)
        bump = np.clip(1.0 + (d1(xz) - d1(xz + Lxz[:, None, None]*0.006))*BUMP, 0.35, 1.6)
        lit = shadow*slope*bump
        top = np.clip(0.35*hf + 0.65*Hc, 0, 1)**0.7           # follows the smooth cloud height, not the noisy hit layer
        amb = np.array([0.34, 0.45, 0.70], np.float32)*(0.55 + 0.45*top)[..., None]
        skyl = np.array([1.00, 0.94, 0.84], np.float32)*(0.18 + 0.82*top**1.6)[..., None]*(1 - 0.75*depthin)[..., None]
        skyl = skyl*(0.12 + 0.88*shadow)[..., None]
        bh = np.array([1.00, 1.15, 1.55], np.float32)*((0.85 + 2.4*facing**2.0)*lit*(0.35 + 0.65*top))[..., None]
        cdet = 0.5 + (det - 0.5)*COLDET*(0.35 + 0.65*smoothstep(0.2, 0.75, s))   # fine tone of the surface, calmer at grazing angles
        col = (amb*1.05*(0.75 + 0.25*bump)[..., None] + skyl*1.6 + bh*1.15)*(0.50 + 0.95*cdet)[..., None]
        rgb += col*(T*a)[..., None]
        T *= (1 - a)

    alpha = 1 - T
    col = rgb/np.maximum(alpha, 1e-4)[..., None]
    # aerial perspective: far cloud fades into the blue haze of the atmosphere
    haze = np.array([0.16, 0.30, 0.58], np.float32)
    w = (1 - np.exp(-0.022*dist_top))[..., None]
    col = col*(1 - w) + haze*w
    # the gaps show the dark, hazy air of the planet below
    ground = np.array([0.032, 0.056, 0.110], np.float32)*(1 - w) + haze*0.55*w
    full = col*alpha[..., None] + ground*(1 - alpha)[..., None]
    return np.where(below[..., None], full, 0).astype(np.float32), below.astype(np.float32)
