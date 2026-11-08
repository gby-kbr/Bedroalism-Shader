$input v_clipPosition, v_worldPosition, v_color0, v_ditheringAndMaskTinting, v_fog, v_lightmapUV, v_texcoord0, v_water, v_worldPos

#include <bgfx_shader.sh>
#include <MinecraftRenderer.Materials/DitherUtil.dragonh>
#include <bedroalism/config.h>
#include <bedroalism/functions/lighting.h>
#include <bedroalism/functions/underwater.h>
#include <bedroalism/functions/shadow.h>
#include <bedroalism/functions/torch.h>
#include <bedroalism/functions/rain.h>
#include <bedroalism/functions/fog.h>
#include <bedroalism/functions/glow.h>
#include <bedroalism/functions/caustics.h>
#include <bedroalism/functions/colorcorrection.h>

uniform vec4 FogColor;
uniform vec4 FogAndDistanceControl;
uniform vec4 TimeOfDay;
uniform vec4 DitherParams;
uniform vec4 DitherParams2[3];
uniform vec4 ViewPositionAndTime;
uniform vec4 CameraPosition;

SAMPLER2D_AUTOREG(s_MatTexture);
SAMPLER2D_AUTOREG(s_LightMapTexture);
SAMPLER2D_AUTOREG(s_BrCloudNoise);
SAMPLER2D_AUTOREG(s_BrCausticNoise);
#if defined(SEASONS)
SAMPLER2D_AUTOREG(s_SeasonsTexture);
#endif

void main() {
#if defined(DEPTH_ONLY) || defined(DEPTH_ONLY_OPAQUE) || defined(DEPTH_ONLY_ALPHA_TEST)
    #if defined(ALPHA_TEST)
    vec4 diffuse = texture2D(s_MatTexture, v_texcoord0);
    if (diffuse.a < 0.5) {
        discard;
    }
    #endif
    gl_FragColor = vec4_splat(0.0);
    return;
#else

    vec4 diffuse = texture2D(s_MatTexture, v_texcoord0);

#if defined(ALPHA_TEST)
    bool ditherDiscard = false;
    #if defined(DITHERING)
    if (v_ditheringAndMaskTinting.x > 0.5) {
        ditherDiscard = evaluateDitherDiscard(v_clipPosition, v_worldPosition.xyz, ViewPositionAndTime.xyz, u_view, DitherParams, DitherParams2[2]);
    }
    #endif
    if (ditherDiscard || diffuse.a < 0.5) {
        discard;
    }
#endif

#if defined(SEASONS)
    diffuse.rgb *= mix(vec3_splat(1.0), texture2D(s_SeasonsTexture, v_color0.xy).rgb * 2.0, v_color0.b);
    diffuse.rgb *= v_color0.a;
    diffuse.a = 1.0;
#else
    if (v_ditheringAndMaskTinting.y > 0.5) {
        diffuse.rgb = mix(diffuse.rgb, diffuse.rgb * v_color0.rgb, diffuse.a) * v_color0.a;
        diffuse.a = 1.0;
    } else {
        diffuse.rgb *= v_color0.rgb;
        #if !defined(TRANSPARENT)
            diffuse.a = 1.0;
        #else
            diffuse.a *= v_color0.a;
        #endif
    }
#endif

#if defined(TRANSPARENT) && defined(DITHERING)
    if (v_ditheringAndMaskTinting.x > 0.5) {
        if (evaluateDitherDiscard(v_clipPosition, v_worldPosition.xyz, ViewPositionAndTime.xyz, u_view, DitherParams, DitherParams2[2])) {
            diffuse.a = 0.0;
        }
    }
#endif

    brEnvironment env = brDetectEnvironment(TimeOfDay.x, FogColor.rgb, FogAndDistanceControl.xyz);
    vec3 relPos = v_worldPos - ViewPositionAndTime.xyz; // from the camera to the fragment
    float camDist = length(relPos);
    vec3 viewDir = relPos/max(camDist, 0.0001);

    bool isWater = false;
    // derivatives are taken outside any branch, they are only defined when all pixels of a block run them
    // the normal of a visible face points toward the camera, which also fixes the sign the winding gives
    vec3 geoNormal = normalize(cross(dFdx(v_worldPos), dFdy(v_worldPos)));
    if (dot(geoNormal, viewDir) > 0.0) {
        geoNormal = -geoNormal;
    }

#if defined(TRANSPARENT) && !(defined(RENDER_AS_BILLBOARDS) || defined(SEASONS))
    if (v_water.x > 0.5) {
        isWater = true;
        brSkyColor sky = brSkyColors(env);

        // the top face gets the animated waves, the other faces use the normal of the geometry
        bool top = v_water.y > 0.5;
        vec3 nrm = geoNormal;
        if (top) {
            vec2 wavePos = brWaterCoord(relPos.xz, CameraPosition.xz);
            nrm = brWaterNormal(wavePos, ViewPositionAndTime.w, camDist);
        }

        vec4 water;
        if (top && viewDir.y > 0.0) {
            // the camera is under the surface: the sky through the water, bent by it. Where a roof covers
            // the water the surface turns see-through so the roof stays visible
            float skyVisible = smoothstep(0.55, 0.95, v_lightmapUV.y);
            vec3 seen = brUnderwaterSurface(s_BrCloudNoise, viewDir, -nrm, env, sky, skyVisible, ViewPositionAndTime.w, gl_FragCoord.xy);
            water = vec4(seen, mix(0.55, 1.0, skyVisible));
        } else {
            water = brWaterAbove(s_BrCloudNoise, viewDir, nrm, v_lightmapUV, v_color0.rgb, env, sky, ViewPositionAndTime.w, gl_FragCoord.xy);
        }
        // torch light on the water, in the color of the dimension
        water.rgb += 0.35*brTorchLight(v_lightmapUV.x, water.rgb, env, relPos.xz + CameraPosition.xz, ViewPositionAndTime.w);
        diffuse = water;
    }
#endif

    if (!isWater) {
        // ambient light of the sky (the lightmap without block light) plus our own torch light
        vec3 albedo = diffuse.rgb;
        vec2 skyOnly = vec2(0.0, v_lightmapUV.y);
        vec3 ambient = texture2D(s_LightMapTexture, skyOnly).rgb;
        ambient *= brGetNightAmbient(skyOnly, TimeOfDay.x, FogColor.rgb, FogAndDistanceControl.xyz);
        vec3 torch = brTorchLight(v_lightmapUV.x, ambient, env, relPos.xz + CameraPosition.xz, ViewPositionAndTime.w);
        diffuse.rgb *= ambient + torch;
        diffuse.rgb = brShadow(diffuse.rgb, v_lightmapUV, geoNormal, v_color0.g, env);
#if !defined(ALPHA_TEST) && !defined(SEASONS) && !defined(TRANSPARENT)
        // rain: wet ground, thin puddles with the reflection of the sky and the rings of the drops
        diffuse.rgb = brWetSurface(s_BrCloudNoise, diffuse.rgb, albedo, viewDir, geoNormal, v_lightmapUV, relPos.xz + CameraPosition.xz, camDist, FogAndDistanceControl.z, env, ViewPositionAndTime.w, gl_FragCoord.xy);
#endif
        // under water: the moving light pattern on the bottom (does nothing outside of the overworld)
        diffuse.rgb = brCaustics(s_BrCausticNoise, diffuse.rgb, relPos.xz + CameraPosition.xz, relPos.y + CameraPosition.y, camDist, geoNormal, v_lightmapUV.y, env, ViewPositionAndTime.w);
    }

    if (env.underwater) {
        // clear water haze instead of the thick vanilla fog
        diffuse.rgb = brUnderwaterFog(diffuse.rgb, camDist, env, v_lightmapUV.y);
    } else {
        diffuse.rgb = brApplyFog(diffuse.rgb, relPos, CameraPosition.y, v_lightmapUV.y, FogAndDistanceControl.z, env, FogColor.rgb, v_fog.a);
    }

    // dazzle when looking toward the sun, over the fog so that haze glows too
    diffuse.rgb = brApplyGlare(diffuse.rgb, viewDir, env, v_lightmapUV.y);

    diffuse.rgb = brColorCorrection(diffuse.rgb);
    gl_FragColor = diffuse;
#endif
}
