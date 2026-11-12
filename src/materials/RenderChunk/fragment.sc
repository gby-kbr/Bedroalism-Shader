$input v_clipPosition, v_worldPosition, v_color0, v_ditheringAndMaskTinting, v_fog, v_fogAdd, v_fogMul, v_lightmapUV, v_sunLight, v_texcoord0, v_water, v_worldPos

#include <bgfx_shader.sh>
#include <bedroalism/config.h>
#include <bedroalism/functions/colorcorrection.h>
#include <bedroalism/functions/fog.h>
#include <bedroalism/functions/lighting.h>
#include <bedroalism/functions/caustics.h>
#include <bedroalism/functions/puddles.h>
#include <MinecraftRenderer.Materials/DitherUtil.dragonh>

uniform vec4 FogColor;
uniform vec4 DitherParams;
uniform vec4 DitherParams2[3];
uniform vec4 ViewPositionAndTime;

uniform vec4 FogAndDistanceControl;
uniform vec4 TimeOfDay;
uniform vec4 CameraPosition;
SAMPLER2D_AUTOREG(s_MatTexture);
SAMPLER2D_AUTOREG(s_LightMapTexture);
SAMPLER2D_AUTOREG(s_BrCausticNoise);
SAMPLER2D_AUTOREG(s_BrCloudNoise);
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
    vec3 relPos = v_worldPosition.xyz - ViewPositionAndTime.xyz;
    float camDist = length(relPos);
    vec3 viewDir = relPos/max(camDist, 0.0001);

    // derivatives are taken outside any branch, they are only defined when all pixels of a block run them
    // the normal of a visible face points toward the camera, which also fixes the sign the winding gives
    vec3 geoNormal = normalize(cross(dFdx(v_worldPosition.xyz), dFdy(v_worldPosition.xyz)));
    if (dot(geoNormal, viewDir) > 0.0) {
        geoNormal = -geoNormal;
    }

    bool isWater = false;
#if defined(TRANSPARENT) && !(defined(RENDER_AS_BILLBOARDS) || defined(SEASONS))
    if (v_water.x > 0.5) {
        isWater = true;

        // the top face gets the animated waves, the other faces use the normal of the geometry
        bool top = v_water.y > 0.5;
        vec3 nrm = geoNormal;
        if (top) {
            nrm = brWaterNormal(brWaterCoord(relPos.xz, CameraPosition.xz), ViewPositionAndTime.w, camDist);
        }

        if (top && viewDir.y > 0.0) {
            // under the surface: as clear as from above, with the sun and moon showing through
            diffuse = brUnderwaterSurface(viewDir, -nrm, env);
        } else {
            brSkyColor sky = brSkyColors(env); // only water pixels need it
            diffuse = brWaterAbove(viewDir, nrm, v_lightmapUV, v_color0.rgb, env, sky, s_BrCloudNoise, ViewPositionAndTime.w);
        }
    }
#endif

    if (!isWater) {
        vec3 lightMap = texture2D(s_LightMapTexture, v_lightmapUV).rgb;
        // the End has no sky light, the vanilla lightmap there is dark and green: a soft ambient takes its place
        if (env.end) {
            lightMap = mix(BR_END_AMBIENT_COL*BR_END_AMBIENT, lightMap*BR_END_AMBIENT_COL, smoothstep(0.1, 0.8, v_lightmapUV.x)*0.5);
        }
        diffuse.rgb *= brTorchLight(lightMap, v_lightmapUV.x, v_lightmapUV.y, env, ViewPositionAndTime.w);

        // night ambient: covered places go black, open sky keeps only a dim cool moonlight
        float torch = smoothstep(0.1, 0.8, v_lightmapUV.x);
        float skyLit = smoothstep(0.3, 0.9, v_lightmapUV.y);
        float night = smoothstep(0.0, 1.0, env.night)*(env.overworld && !env.underwater ? 1.0 : 0.0);
        float keep = max(torch, BR_MOONLIGHT*skyLit);
        diffuse.rgb *= 1.0 - BR_NIGHT_DARKNESS*night*(1.0 - keep);

        // sun and moon light color, only where sky light reaches and no torch lights the surface
        vec3 lightCol = v_sunLight.rgb;
        diffuse.rgb *= mix(vec3_splat(1.0), lightCol, skyLit*(1.0 - torch)*(env.overworld && !env.underwater ? 1.0 : 0.0));

        // shadows of Aerotion, from the lightmap uv where sky light 15 is 15/16: the shadow under leaves and
        // overhangs, and the shadow on the sides of blocks, only outdoors. Gone at night and in the rain
        float rainShadowMask = 1.0 - env.rainFactor;
        float nightShadowMask = 1.0 - env.night;
        float worldMask = env.overworld && !env.underwater ? 1.0 : 0.0;
        vec2 shadowUV = v_water.zw;
        float outdoorMask = smoothstep(0.86, 0.93, shadowUV.y); // full only at sky light 15, 14 and below is indoors
        float shadow = 1.0 - smoothstep(0.860, 0.875, pow(shadowUV.y, 2.0));
        shadow = mix(shadow, 0.0, min(pow(shadowUV.x*1.2, 6.0), 1.0)); // no shadow near torch light
        diffuse.rgb *= 1.0 - BR_SHADOW*shadow*rainShadowMask*nightShadowMask*worldMask;

        float sideShadow = 1.0 - smoothstep(0.62, 0.64, v_color0.g); // the x faces and the bottom of blocks
        diffuse.rgb *= 1.0 - BR_SIDE_SHADOW*sideShadow*outdoorMask*rainShadowMask*nightShadowMask*worldMask;

        // rain wets open ground, flat ground gets puddles with rings of drops (not on leaves and plants)
#if defined(ALPHA_TEST) || defined(SEASONS) || defined(TRANSPARENT)
        const bool wetOpaque = false;
#else
        const bool wetOpaque = true;
#endif
        diffuse.rgb = brWetSurface(diffuse.rgb, relPos + CameraPosition.xyz, geoNormal, viewDir, v_lightmapUV.y, wetOpaque, ViewPositionAndTime.w, env);

#if !defined(ALPHA_TEST) && !defined(SEASONS) && !defined(TRANSPARENT)
        // the moving light pattern on the bottom of the sea, from under and above the water
        diffuse.rgb = brCaustics(s_BrCausticNoise, diffuse.rgb, relPos.xz + CameraPosition.xz, relPos.y + CameraPosition.y, camDist, geoNormal, v_lightmapUV.y, env, ViewPositionAndTime.w);
#endif
    }

    diffuse.rgb = diffuse.rgb*v_fogMul.rgb + v_fogAdd.rgb; // fog, worked out in the vertex shader

    diffuse.rgb = brColorCorrection(diffuse.rgb);
    gl_FragColor = diffuse;
#endif
}
