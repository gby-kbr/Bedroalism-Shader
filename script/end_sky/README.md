# End sky generator

Procedural 360° equirectangular End sky (4032x2016, seamless left/right).

    pip install numpy scipy pillow
    python3 assemble.py 4032        # ~2 min, writes linear_nobloom.npy / emit.npy / win.npy
    NOCLOUDS=1 python3 assemble.py 4032   # space + black hole only
    BR_K=0.46 BR_OPAC=28 python3 cloudsonly.py 4032   # ~15 min, clouds_rgb.npy / clouds_mask.npy (lower half)
    python3 finish.py 4032 end_sky.png
    python3 views.py                # helpers: persp(), seam_shift(), seam_stats()

Copy the result to `mcpack/textures/environment/end_sky.png`.
