$input a_color0, a_position, a_texcoord0
#ifdef INSTANCING
    $input i_data1, i_data2, i_data3
#endif

$output v_color0, v_texcoord0, v_time, v_worldPos

#include <bgfx_shader.sh>

uniform vec4 ViewPositionAndTime;

void main() {
    v_color0 = vec4(1.0, 1.0, 1.0, 1.0);
    v_texcoord0 = a_texcoord0;
    v_time = vec4(ViewPositionAndTime.w, 0.0, 0.0, 0.0);
#ifdef INSTANCING
    // the dome of the game stops above the horizon and leaves a hard line, so it is hidden
    v_worldPos = vec3(0.0, 1.0, 0.0);
    gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
#else
    // one quad over the whole screen, the sky is drawn per pixel from the view direction
    vec4 pos = vec4(a_position.xzy, 1.0);
    pos.xy = 2.0*clamp(pos.xy, -0.5, 0.5);
    v_worldPos = mul(u_invViewProj, pos).xyz;
    gl_Position = pos;
#endif
}
