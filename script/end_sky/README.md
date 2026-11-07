# End sky generator

Procedural 360° equirectangular End sky (4032x2016, seamless left/right).

    pip install numpy scipy pillow
    python3 assemble.py 4032        # ~2 min, writes linear_nobloom.npy / emit.npy / win.npy
    python3 finish.py 4032 end_sky.png
    python3 views.py                # helpers: persp(), seam_shift(), seam_stats()

Copy the result to `mcpack/textures/environment/end_sky.png`.
