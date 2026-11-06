#ifndef INSTANCING
    $input v_worldPos
#endif

#include <bgfx_shader.sh>

#ifndef INSTANCING
    #include <bedroalism/config.h>
    #include <bedroalism/functions/detection.h>
    #include <bedroalism/functions/sky.h>
    #include <bedroalism/functions/clouds.h>
    #include <bedroalism/functions/colorcorrection.h>

    uniform vec4 TimeOfDay;
    uniform vec4 FogColor;
    uniform vec4 FogAndDistanceControl;
    uniform vec4 ViewPositionAndTime;

    SAMPLER2D_AUTOREG(s_BrCloudNoise);
#endif

void main() {
#ifndef INSTANCING
    // w of the unprojected position is negative, so the direction comes out reversed
    vec3 viewDir = -normalize(v_worldPos);

    brEnvironment env = brDetectEnvironment(TimeOfDay.x, FogColor.rgb, FogAndDistanceControl.xyz);
    brSkyColor skyCol = brSkyColors(env);

    vec3 color = brSkyGradient(viewDir, skyCol);

    if (!env.underwater) {
        vec4 clouds = brClouds(s_BrCloudNoise, viewDir, env, skyCol, ViewPositionAndTime.w, gl_FragCoord.xy);
        color = mix(color, clouds.rgb, clouds.a);
    }

    gl_FragColor = vec4(brColorCorrection(color), 1.0);
#else
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
#endif
}
