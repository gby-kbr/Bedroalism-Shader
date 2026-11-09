$input v_color0, v_texcoord0, v_worldPos

#include <bgfx_shader.sh>
#include <bedroalism/config.h>
#include <bedroalism/functions/colorcorrection.h>

uniform vec4 StarsColor;

void main() {
    vec3 stars = v_color0.rgb * StarsColor.rgb * v_color0.a;
    gl_FragColor = vec4(brColorCorrection(stars), v_color0.a);
}
