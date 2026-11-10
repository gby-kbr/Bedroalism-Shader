$input v_color0, v_texcoord0, v_worldPos

#include <bgfx_shader.sh>

// The clouds are drawn in the Sky material (see bedroalism/functions/clouds.h), so the blocky ones are dropped
void main() {
    discard;
}
