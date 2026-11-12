$input v_color0, v_texcoord0, v_time, v_worldPos

#include <bgfx_shader.sh>
#include <bedroalism/config.h>
#include <bedroalism/functions/colorcorrection.h>
#include <bedroalism/functions/clouds.h>

SAMPLER2D_AUTOREG(s_SunMoonTexture);
SAMPLER2D_AUTOREG(s_BrCloudNoise);
uniform vec4 SunMoonColor;
uniform vec4 FogColor;
uniform vec4 FogAndDistanceControl;
uniform vec4 TimeOfDay;

void main() {
    vec4 color = SunMoonColor * texture2D(s_SunMoonTexture, v_texcoord0);

    // the sun and the moon are far behind the clouds, so the clouds in that direction hide them
    brEnvironment env = brDetectEnvironment(TimeOfDay.x, FogColor.rgb, FogAndDistanceControl.xyz);
    float hidden = brCloudCover(s_BrCloudNoise, normalize(v_worldPos), env, v_time.x);
    // both are scaled: the blend of the game may add the color (the alpha is ignored) or mix it by the alpha
    color *= 1.0 - hidden;

    gl_FragColor = vec4(brColorCorrection(color.rgb), color.a);
}
