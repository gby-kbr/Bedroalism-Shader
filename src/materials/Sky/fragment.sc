$input v_color0, v_texcoord0, v_time, v_worldPos

#include <bgfx_shader.sh>
#include <bedroalism/config.h>
#include <bedroalism/functions/colorcorrection.h>
#include <bedroalism/functions/clouds.h>

SAMPLER2D_AUTOREG(s_BrCloudNoise);

uniform vec4 FogColor;
uniform vec4 FogAndDistanceControl;
uniform vec4 TimeOfDay;

void main() {
    brEnvironment env = brDetectEnvironment(TimeOfDay.x, FogColor.rgb, FogAndDistanceControl.xyz);
    vec3 dir = normalize(v_worldPos);
    brSkyColor skyColors = brSkyColors(env);
    vec3 sky = brRenderSky(skyColors, dir);
    if (env.overworld && !env.underwater) {
        vec4 clouds = brClouds(s_BrCloudNoise, dir, env, skyColors, v_time.x);
        sky = mix(sky, clouds.rgb, clouds.a);
    }
    gl_FragColor = vec4(brColorCorrection(sky), v_color0.a);
}
