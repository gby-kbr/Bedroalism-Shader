$input v_color0, v_texcoord0, v_worldPos

#include <bgfx_shader.sh>
#include <bedroalism/config.h>
#include <bedroalism/functions/colorcorrection.h>

SAMPLER2D_AUTOREG(s_SkyTexture);
uniform vec4 SkyColor;

// End sky is a 360 degree equirectangular panorama (textures/environment/end_sky.png),
// looked up by view direction instead of the vanilla cube-face UVs.
void main() {
    vec3 dir = normalize(v_worldPos);
    float lon = atan2(dir.x, dir.z);                 // -pi..pi, 0 = +z
    float lat = asin(clamp(dir.y, -1.0, 1.0));       // -pi/2..pi/2
    vec2 uv = vec2(lon * 0.15915494 + 0.5, 0.5 - lat * 0.31830989);
    // Level 0 only: mip selection from atan2 derivatives would draw a line at the seam.
    vec4 color = vec4(texture2DLod(s_SkyTexture, uv, 0.0).rgb, 1.0);
    color.rgb = brColorCorrection(color.rgb);
    gl_FragColor = color;
}
