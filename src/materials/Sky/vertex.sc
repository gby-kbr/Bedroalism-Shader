#ifndef INSTANCING
    $input a_position
    $output v_worldPos
#endif

#include <bgfx_shader.sh>

void main() {
#ifndef INSTANCING
    // the sky is a full screen quad, the view direction is rebuilt per pixel from the inverse view projection
    vec4 pos = vec4(a_position.x, a_position.z, a_position.y, 1.0);
    pos.xy = 2.0*clamp(pos.xy, -0.5, 0.5);

    v_worldPos = mul(u_invViewProj, pos).xyz;
    gl_Position = pos;
#else
    gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
#endif
}
