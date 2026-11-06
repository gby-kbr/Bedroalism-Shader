$input v_color0, v_texcoord0, v_worldPos

#include <bgfx_shader.sh>
#include <bedroalism/config.h>
#include <bedroalism/functions/colorcorrection.h>

SAMPLER2D_AUTOREG(s_SunMoonTexture);
uniform vec4 SunMoonColor;

void main() {
    vec4 color = SunMoonColor * texture2D(s_SunMoonTexture, v_texcoord0);
    color.rgb = brColorCorrection(color.rgb);
    gl_FragColor = color;
}
