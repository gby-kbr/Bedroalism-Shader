$input v_clipPosition, v_worldPosition, v_color0, v_ditheringAndMaskTinting, v_fog, v_lightmapUV, v_texcoord0, v_water, v_worldPos

#include <bgfx_shader.sh>
#include <bedroalism/config.h>
#include <bedroalism/functions/colorcorrection.h>
#include <bedroalism/functions/fog.h>
#include <bedroalism/functions/caustics.h>
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
        brSkyColor sky = brSkyColors(env);

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
            diffuse = brWaterAbove(viewDir, nrm, v_lightmapUV, v_color0.rgb, env, sky, s_BrCloudNoise, ViewPositionAndTime.w);
        }
    }
#endif

    if (!isWater) {
        diffuse.rgb *= texture2D(s_LightMapTexture, v_lightmapUV).rgb;

        // lightmap shadows, only in the overworld daytime and only where sky light reaches
        float sunlit = smoothstep(0.45, 0.70, v_lightmapUV.y)*(1.0 - env.night)*(env.overworld && !env.underwater ? 1.0 : 0.0);
        float shadow = (1.0 - smoothstep(0.860, 0.875, v_lightmapUV.y*v_lightmapUV.y))*(1.0 - min(pow(v_lightmapUV.x*1.2, 6.0), 1.0))*sunlit;
        float sideShadow = step(0.9, abs(geoNormal.x))*sunlit*(1.0 - shadow); // the whole side face, not over the first shadow
        diffuse.rgb *= (1.0 - BR_SHADOW*shadow)*(1.0 - BR_SIDE_SHADOW*sideShadow);

#if !defined(ALPHA_TEST) && !defined(SEASONS) && !defined(TRANSPARENT)
        // the moving light pattern on the bottom, only under water in the overworld
        diffuse.rgb = brCaustics(s_BrCausticNoise, diffuse.rgb, relPos.xz + CameraPosition.xz, relPos.y + CameraPosition.y, camDist, geoNormal, v_lightmapUV.y, env, ViewPositionAndTime.w);
#endif
    }

    diffuse.rgb = brApplyFog(diffuse.rgb, relPos, relPos.xz + CameraPosition.xz, CameraPosition.y, v_lightmapUV.y, FogAndDistanceControl.z, ViewPositionAndTime.w, env, FogColor.rgb, v_fog.a);

    diffuse.rgb = brColorCorrection(diffuse.rgb);
    gl_FragColor = diffuse;
#endif
}
