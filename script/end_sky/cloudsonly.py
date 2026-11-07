import numpy as np, time, sys
from common import *
import clouds
W = int(sys.argv[1]); H = W//2
t0 = time.time()
tL = tile_noise(512, 1.5, 1); tD = tile_noise(512, 1.3, 2)
tiles = (mip_chain(tL, 8), mip_chain(tD, 8))
out = np.zeros((H//2, W, 3), np.float32); msk = np.zeros((H//2, W), np.float32)
for r0 in range(H//2, H, 144):
    r1 = min(r0 + 144, H)
    rgb, below = clouds.cloud_layer(W, H, (r0, r1), tiles)
    out[r0 - H//2:r1 - H//2] = rgb; msk[r0 - H//2:r1 - H//2] = below
    print(f'[{time.time()-t0:6.1f}s] rows {r0}-{r1}', flush=True)
np.save('clouds_rgb.npy', out); np.save('clouds_mask.npy', msk)
print('saved', flush=True)
