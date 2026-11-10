$input a_color0, a_position, a_texcoord0
#ifdef INSTANCING
    $input i_data1, i_data2, i_data3
#endif

$output v_color0, v_texcoord0, v_worldPos

#include <bgfx_shader.sh>
#include <bedroalism/config.h>
#include <bedroalism/functions/utils.h>
#include <bedroalism/functions/sunmoon.h>

uniform vec4 TimeOfDay;

void main() {
    vec3 worldPosition;
#ifdef INSTANCING
    mat4 model = mtxFromCols(i_data1, i_data2, i_data3, vec4(0.0, 0.0, 0.0, 1.0));
    worldPosition = instMul(model, vec4(a_position, 1.0)).xyz;
#else
    // the sun and the moon are two draws of the same quad, told apart by where the game puts them
    float angle = 2.0*BR_PI*TimeOfDay.x;
    float st = angle + 0.5*BR_PI;
    vec2 gamePos = mul(u_model[0], vec4(0.0, 0.0, 0.0, 1.0)).xy;
    bool isSun = dot(vec2(cos(st), sin(st)), gamePos) > 0.0;
    worldPosition = brSunMoonPosition(a_position, angle, isSun);
#endif

    v_color0 = a_color0;
    v_texcoord0 = a_texcoord0;
    v_worldPos = worldPosition;
    gl_Position = mul(u_viewProj, vec4(worldPosition, 1.0));
}
