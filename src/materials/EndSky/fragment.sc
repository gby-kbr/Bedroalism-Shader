$input v_color0, v_texcoord0, v_worldPos

#include <bgfx_shader.sh>
#include <bedroalism/config.h>
#include <bedroalism/functions/colorcorrection.h>

SAMPLER2D_AUTOREG(s_SkyTexture);
uniform vec4 SkyColor;

void main() {
    vec4 color = SkyColor * texture2D(s_SkyTexture, v_texcoord0);
    color.rgb = brColorCorrection(color.rgb);
    gl_FragColor = color;
}
