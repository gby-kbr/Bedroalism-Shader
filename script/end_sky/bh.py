"""Black hole by tracing light rays through the Schwarzschild metric (G = c = M = 1).

An observer at radius R0 looks at the black hole. Every pixel is a ray traced backwards along its orbit:
  u = 1/r,  d2u/dphi2 = -u + 3u^2   (light orbit equation)
phi is the angle swept in the orbital plane, which contains the observer, the black hole and the ray.
- ray falls into the horizon      -> black (the shadow, radius about 5.2 M seen from far away)
- ray crosses the disk plane      -> adds light of the accretion disk at that radius (several times for rays
                                     that bend around, which gives the thin inner rings and the arc over the shadow)
- ray escapes                     -> looks up the star field in the bent direction
"""
import numpy as np
from scipy import ndimage
from common import *

R0 = 18.0                      # observer radius: the shadow covers about 17 degrees of the sky
INCL = np.radians(4.5)         # elevation of the observer above the disk plane
R_IN, R_OUT = 6.0, 14.0        # accretion disk from the innermost stable orbit outwards
DPHI, NSTEP = 0.0225, 440      # integration step and count, phi up to ~ 3 pi
DISK_OPACITY = 0.86

def frame():
    D0 = np.array([np.cos(BH_EL)*np.sin(BH_AZ), np.sin(BH_EL), np.cos(BH_EL)*np.cos(BH_AZ)], np.float64)
    right = np.cross([0, 1, 0], D0); right /= np.linalg.norm(right)
    up = np.cross(D0, right)
    return D0, right, up

def disk_emission(r, ang, noise_tile):
    """Brightness and colour of the disk at radius r and angle ang (position in the disk plane)."""
    base = 7.5*(R_IN/r)**2.7*smoothstep(R_IN, R_IN*1.07, r)*(1 - smoothstep(0.80*R_OUT, R_OUT, r))
    # spiral streaks: noise in (angle + shear*log r, log r), periodic in the angle
    lr = np.log(r)
    n = noise_tile.shape[0]
    def tex(freq, shear, off):
        u = ((ang + shear*lr)/(2*np.pi)*freq) % 1.0*n
        v = (lr*freq*0.55 + off) % 1.0*n
        return ndimage.map_coordinates(noise_tile, np.stack([v, u]), order=1, mode='grid-wrap')
    streak = 0.7*tex(18, 2.6, 0.13) + 0.3*tex(52, 3.4, 0.61)
    base = base*(0.35 + 1.5*streak)
    t = np.clip((r - R_IN)/(R_OUT - R_IN), 0, 1)**0.55
    white, blue = np.array([1.0, 1.0, 1.0]), np.array([0.40, 0.68, 1.0])
    col = white*(1 - t)[..., None] + blue*t[..., None]
    return base[..., None]*col

def trace(dirs, bg, noise_tile):
    """dirs: (N,3) world directions of the rays (unit). bg: space image (H,W,3) for the bent star lookup.
    Returns rgb of black hole + stars seen through it and the emission of the disk alone."""
    D0, right, up = frame()
    N = len(dirs)
    x, y, z = dirs @ right, dirs @ up, dirs @ D0
    psi0 = np.arccos(np.clip(z, -1, 1))
    gam = np.arctan2(y, x)
    b = R0*np.sin(psi0)/np.sqrt(1 - 2/R0)
    u0 = 1.0/R0
    w0 = np.sqrt(np.maximum(1/b**2 - u0**2*(1 - 2*u0), 0))
    st, ct = np.sin(INCL), np.cos(INCL)

    # per ray state
    idx = np.arange(N)
    u = np.full(N, u0); w = w0.copy(); phi = np.zeros(N)
    g, sg, cg = gam, np.sin(gam), np.cos(gam)
    zn_prev = np.full(N, st)                          # z numerator at phi = 0 (observer above the plane)
    T = np.ones(N); emit = np.zeros((N, 3)); status = np.zeros(N, np.int8)   # 0 running, 1 escaped, 2 captured
    phi_final = np.zeros(N)
    act = idx.copy()
    for step in range(NSTEP):
        if len(act) == 0: break
        uu, ww, ph = u[act], w[act], phi[act]
        # RK4 on (u, w)
        h = DPHI
        k1u, k1w = ww, -uu + 3*uu*uu
        u2, w2 = uu + 0.5*h*k1u, ww + 0.5*h*k1w; k2u, k2w = w2, -u2 + 3*u2*u2
        u3, w3 = uu + 0.5*h*k2u, ww + 0.5*h*k2w; k3u, k3w = w3, -u3 + 3*u3*u3
        u4, w4 = uu + h*k3u, ww + h*k3w; k4u, k4w = w4, -u4 + 3*u4*u4
        un = uu + h/6*(k1u + 2*k2u + 2*k3u + k4u)
        wn = ww + h/6*(k1w + 2*k2w + 2*k3w + k4w)
        phn = ph + h
        gi = g[act]
        zn = np.cos(phn)*st + np.sin(phn)*np.sin(gi)*ct
        zp = zn_prev[act]
        # disk crossing
        cross = (zn*zp < 0) & (un > 0)
        if cross.any():
            ci = np.nonzero(cross)[0]
            f = zp[ci]/(zp[ci] - zn[ci])
            phc = ph[ci] + f*h
            uc = uu[ci] + f*(un[ci] - uu[ci])
            wc = ww[ci] + f*(wn[ci] - ww[ci])
            rc = 1.0/np.maximum(uc, 1e-6)
            inside = (rc > R_IN) & (rc < R_OUT)
            if inside.any():
                ii = ci[inside]; rr = rc[inside]; pc = phc[inside]; wcc = wc[inside]
                gg = gi[ii]
                xd = rr*(np.cos(pc)*ct - np.sin(pc)*np.sin(gg)*st)
                yd = rr*np.sin(pc)*np.cos(gg)
                ang = np.arctan2(yd, xd)
                # light direction to the observer (flat space local frame): minus the direction the traced ray moves
                e1 = np.array([ct, 0, st]); 
                tvec = np.stack([-np.sin(gg)*st, np.cos(gg), np.sin(gg)*ct], -1)
                phat = np.cos(pc)[:, None]*e1 + np.sin(pc)[:, None]*tvec
                dphat = -np.sin(pc)[:, None]*e1 + np.cos(pc)[:, None]*tvec
                drdphi = -wcc/np.maximum(uc[inside], 1e-6)**2
                V = drdphi[:, None]*phat + rr[:, None]*dphat
                V /= np.linalg.norm(V, axis=1)[:, None]
                nobs = -V
                vk = 1/np.sqrt(rr - 2)
                vel = vk[:, None]*np.stack([-yd, xd, np.zeros_like(xd)], -1)/rr[:, None]
                vdotn = (vel*nobs).sum(1)
                gam_l = 1/np.sqrt(1 - vk**2)
                dopp = 1/(gam_l*(1 - vdotn))
                gred = np.sqrt(1 - 2/rr)
                E = disk_emission(rr, ang, noise_tile)*(dopp*gred)[:, None]**2.0
                pid = act[ii]
                emit[pid] += E*(T[pid]*DISK_OPACITY)[:, None]
                T[pid] *= (1 - DISK_OPACITY)
        # termination
        cap = un > 0.5
        esc = (wn < 0) & (un <= 0.0)
        # linear interpolation of the angle where u reaches 0
        fe = uu/np.maximum(uu - un, 1e-9)
        pid = act[esc]
        phi_final[pid] = (ph + fe*h)[esc]; status[pid] = 1
        status[act[cap & ~esc]] = 2
        u[act], w[act], phi[act], zn_prev[act] = un, wn, phn, zn
        keep = ~(cap | esc)
        act = act[keep]
    # rays that never finished are on (almost) closed orbits around the photon sphere: a thin bright ring
    ring = status == 0
    # lookup of the bent direction for the escaped rays
    out = np.zeros((N, 3))
    e = status == 1
    if e.any():
        pf = phi_final[e]
        sd = -np.cos(pf)[:, None]*D0 + np.sin(pf)[:, None]*(np.cos(gam[e])[:, None]*right + np.sin(gam[e])[:, None]*up)
        out[e] = lookup(bg, sd)
    out = out*T[:, None]
    # mix back to straight lookup for rays looking away (psi0 > 90): they were not integrated
    return out + emit, emit, ring, psi0, status

def lookup(bg, d):
    """Bilinear lookup of an equirectangular image, wrapping around in longitude."""
    H, W = bg.shape[:2]
    pad = 4
    key = id(bg)
    if getattr(lookup, 'key', None) != key:
        lookup.padded = np.pad(bg, ((0, 0), (pad, pad), (0, 0)), mode='wrap'); lookup.key = key
    lon = np.arctan2(d[:, 0], d[:, 2]); lat = np.arcsin(np.clip(d[:, 1], -1, 1))
    px = (lon/(2*np.pi) + 0.5)*W - 0.5 + pad
    py = np.clip((0.5 - lat/np.pi)*H - 0.5, 0, H - 1)
    c = np.stack([py, px])
    return np.stack([ndimage.map_coordinates(lookup.padded[..., k], c, order=1, mode='nearest') for k in range(3)], -1)
