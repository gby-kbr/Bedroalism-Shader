$input v_color0, v_texcoord0, v_worldPos

#include <bgfx_shader.sh>
#include <bedroalism/config.h>
#include <bedroalism/functions/colorcorrection.h>
#include <bedroalism/functions/sky.h>

uniform vec4 FogColor;
uniform vec4 FogAndDistanceControl;
uniform vec4 TimeOfDay;

void main() {
    brEnvironment env = brDetectEnvironment(TimeOfDay.x, FogColor.rgb, FogAndDistanceControl.xyz);
    vec3 sky = brRenderSky(brSkyColors(env), normalize(v_worldPos));
    gl_FragColor = vec4(brColorCorrection(sky), v_color0.a);
}
