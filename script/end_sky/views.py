"""360 degree checks: perspective views and a sphere rendered from the equirectangular PNG, seam shift."""
import numpy as np, sys
from PIL import Image
from scipy import ndimage

def load(path): return np.asarray(Image.open(path).convert('RGB'))

def persp(img, yaw, pitch, fov=90, w=900, h=600):
    H, W = img.shape[:2]
    f = 0.5*w/np.tan(np.radians(fov)/2)
    x, y = np.meshgrid(np.arange(w) - w/2 + 0.5, np.arange(h) - h/2 + 0.5)
    d = np.stack([x, -y, np.full_like(x, f)], -1); d /= np.linalg.norm(d, axis=-1, keepdims=True)
    cy, sy, cp, sp = np.cos(np.radians(yaw)), np.sin(np.radians(yaw)), np.cos(np.radians(pitch)), np.sin(np.radians(pitch))
    # pitch about x, then yaw about y
    d1 = np.stack([d[..., 0], d[..., 1]*cp + d[..., 2]*sp, -d[..., 1]*sp + d[..., 2]*cp], -1)
    d2 = np.stack([d1[..., 0]*cy + d1[..., 2]*sy, d1[..., 1], -d1[..., 0]*sy + d1[..., 2]*cy], -1)
    lon = np.arctan2(d2[..., 0], d2[..., 2]); lat = np.arcsin(np.clip(d2[..., 1], -1, 1))
    px = (lon/(2*np.pi) + 0.5)*W - 0.5; py = (0.5 - lat/np.pi)*H - 0.5
    pad = 6
    p = np.pad(img, ((0, 0), (pad, pad), (0, 0)), mode='wrap').astype(np.float32)
    c = np.stack([np.clip(py, 0, H - 1), px + pad])
    out = np.stack([ndimage.map_coordinates(p[..., k], c, order=1, mode='nearest') for k in range(3)], -1)
    return np.clip(out + 0.5, 0, 255).astype(np.uint8)

def seam_shift(img):
    """Roll by half a width so the left and right edge meet in the middle of the picture."""
    W = img.shape[1]
    return np.roll(img, W//2, axis=1)

def seam_stats(img):
    a = img[:, 0, :].astype(np.float32); b = img[:, -1, :].astype(np.float32)
    inner = img[:, 1, :].astype(np.float32); inner_b = img[:, -2, :].astype(np.float32)
    seam = np.abs(a - b).mean()
    nb = np.abs(img[:, 1:, :].astype(np.float32) - img[:, :-1, :].astype(np.float32)).mean()
    return seam, nb
