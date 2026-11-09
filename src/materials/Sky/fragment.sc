$input v_color0, v_texcoord0, v_worldPos

#include <bgfx_shader.sh>
#include <bedroalism/config.h>
#include <bedroalism/functions/colorcorrection.h>

void main() {
    gl_FragColor = vec4(brColorCorrection(v_color0.rgb), v_color0.a);
}
