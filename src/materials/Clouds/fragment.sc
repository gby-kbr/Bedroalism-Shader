$input v_color0, v_texcoord0, v_worldPos

#include <bgfx_shader.sh>
#include <bedroalism/config.h>
#include <bedroalism/functions/colorcorrection.h>

void main() {
    vec4 color = v_color0;
    color.rgb = brColorCorrection(color.rgb);
    gl_FragColor = color;
}
