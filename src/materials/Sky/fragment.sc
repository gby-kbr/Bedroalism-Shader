#ifndef INSTANCING
    $input v_worldPos
#endif

#include <bgfx_shader.sh>

#ifndef INSTANCING
    #include <bedroalism/config.h>
    #include <bedroalism/functions/detection.h>
    #include <bedroalism/functions/sky.h>
    #include <bedroalism/functions/glow.h>
    #include <bedroalism/functions/clouds.h>
    #include <bedroalism/functions/underwater.h>
    #include <bedroalism/functions/colorcorrection.h>

    uniform vec4 TimeOfDay;
    uniform vec4 FogColor;
    uniform vec4 FogAndDistanceControl;
    uniform vec4 ViewPositionAndTime;
    uniform vec4 CameraPosition;

    SAMPLER2D_AUTOREG(s_BrCloudNoise);
#endif

void main() {
#ifndef INSTANCING
    // w of the unprojected position is negative, so the direction comes out reversed
    vec3 viewDir = -normalize(v_worldPos);

    brEnvironment env = brDetectEnvironment(TimeOfDay.x, FogColor.rgb, FogAndDistanceControl.xyz);
    brSkyColor skyCol = brSkyColors(env);

    vec3 color;
    if (env.underwater) {
        // the sky as seen through the water surface above, with the ripples of the surface bending it
        float depth = 2.5; // assumed distance to the surface
        vec2 hit = brWaterCoord(viewDir.xz/max(viewDir.y, 0.1)*depth, CameraPosition.xz);
        vec3 up = brWaterNormal(hit, ViewPositionAndTime.w, depth);
        vec3 nrm = vec3(-up.x, -up.y, -up.z);
        if (viewDir.y > 0.0) {
            color = brUnderwaterSurface(s_BrCloudNoise, viewDir, nrm, env, skyCol, 1.0, ViewPositionAndTime.w, gl_FragCoord.xy);
        } else {
            color = brWaterScatterColor(env, 0.9)*mix(1.0, 0.55, smoothstep(0.0, 0.5, -viewDir.y));
        }
    } else {
        color = brSkyGradient(viewDir, skyCol);
        color += brSunMoonGlow(viewDir, env);

        vec4 clouds = brClouds(s_BrCloudNoise, viewDir, env, skyCol, ViewPositionAndTime.w, gl_FragCoord.xy);
        color = mix(color, clouds.rgb, clouds.a);
    }

    gl_FragColor = vec4(brColorCorrection(color), 1.0);
#else
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
#endif
}
