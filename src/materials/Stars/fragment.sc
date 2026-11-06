$input v_color0, v_texcoord0, v_worldPos

#include <bgfx_shader.sh>
#include <bedroalism/config.h>
#include <bedroalism/functions/colorcorrection.h>

uniform vec4 StarsColor;

void main() {
    vec4 color = vec4(v_color0.rgb * StarsColor.rgb * v_color0.a, v_color0.a);
    color.rgb = brColorCorrection(color.rgb);
    gl_FragColor = color;
}
