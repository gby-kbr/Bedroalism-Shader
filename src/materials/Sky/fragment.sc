$input v_color0, v_texcoord0, v_time, v_worldPos

#include <bgfx_shader.sh>
#include <bedroalism/config.h>
#include <bedroalism/functions/colorcorrection.h>
#include <bedroalism/functions/clouds.h>
#include <bedroalism/functions/stars.h>

SAMPLER2D_AUTOREG(s_BrCloudNoise);
SAMPLER2D_AUTOREG(s_BrStarMap);

uniform vec4 FogColor;
uniform vec4 FogAndDistanceControl;
uniform vec4 TimeOfDay;

void main() {
    brEnvironment env = brDetectEnvironment(TimeOfDay.x, FogColor.rgb, FogAndDistanceControl.xyz);
    vec3 dir = normalize(v_worldPos);
    brSkyColor skyColors = brSkyColors(env);
    vec3 sky = brRenderSky(skyColors, dir);
    // the quad covers the whole screen and the terrain is drawn over most of it, so the stars, the glow and
    // the clouds only run above the horizon. The clouds have faded to nothing a little before it, so the
    // edge of this test cannot be seen
    if (env.overworld && dir.y > -0.04) {
        if (env.night > 0.0) {
            sky += brStars(s_BrStarMap, dir, env); // under the clouds
        }
        sky += brSunMoonGlow(dir, env);
        vec4 clouds = brClouds(s_BrCloudNoise, dir, env, skyColors, v_time.x);
        sky = mix(sky, clouds.rgb, clouds.a);
    }
    gl_FragColor = vec4(brColorCorrection(sky), v_color0.a);
}
