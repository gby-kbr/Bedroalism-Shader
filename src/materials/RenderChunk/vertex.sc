$input a_color0, a_position, a_texcoord0, a_texcoord1
#ifdef INSTANCING
    $input i_data1, i_data2, i_data3
#endif

$output v_clipPosition, v_color0, v_ditheringAndMaskTinting, v_fog, v_lightmapUV, v_texcoord0, v_water, v_worldPos, v_worldPosition

#include <bgfx_shader.sh>
#include <MinecraftRenderer.Materials/TAAUtil.dragonh>
#include <bedroalism/config.h>
#include <bedroalism/functions/water.h>
#include <bedroalism/functions/wave.h>
#include <bedroalism/functions/lava.h>

uniform vec4 RenderChunkFogAlpha;
uniform vec4 FogAndDistanceControl;
uniform vec4 ViewPositionAndTime;
uniform vec4 FogColor;
uniform vec4 MeshContext;
uniform vec4 CameraPosition;

SAMPLER2D_AUTOREG(s_MatTexture);

void main() {
#ifdef INSTANCING
    mat4 model = mtxFromCols(i_data1, i_data2, i_data3, vec4(0.0, 0.0, 0.0, 1.0));
    vec3 worldPos = instMul(model, vec4(a_position, 1.0)).xyz;
#else
    vec3 worldPos = mul(u_model[0], vec4(a_position, 1.0)).xyz;
#endif

    vec4 color;
#ifdef RENDER_AS_BILLBOARDS
    worldPos += vec3_splat(0.5);
    vec3 viewDir = normalize(worldPos - ViewPositionAndTime.xyz);
    vec3 boardPlane = normalize(cross(vec3(0.0, 1.0, 0.0), viewDir));
    worldPos -= (cross(viewDir, boardPlane) * (a_color0.z - 0.5)) + (boardPlane * (a_color0.x - 0.5));
    color = vec4_splat(1.0);
#else
    color = a_color0;
#endif

    vec4 fogAndDist = FogAndDistanceControl;
    if (MeshContext.x > 0.5) {
        fogAndDist = vec4(0.99, 1.0, 100000.0, 100000.0);
    }

    uvec2 uv0_16 = uvec2(round(a_texcoord0 * 65535.0));
    vec2 uv0 = vec2(float((uv0_16.x & 32767u) << uint(1)), float((uv0_16.y & 32767u) << uint(1))) * 1.525902189314365386962890625e-05;
    uv0.x += 3.0517578125e-05 * ((2.0 * float((uv0_16.x & 32768u) >> uint(15))) - 1.0);
    uv0.y += 3.0517578125e-05 * ((2.0 * float((uv0_16.y & 32768u) >> uint(15))) - 1.0);

    uvec2 uv1_16 = uvec2(round(a_texcoord1 * 65535.0));
    float lightmapX = float((uv1_16.y >> 4u) & 15u) * 0.06666667;
    float lightmapY = float(uv1_16.y & 15u) * 0.06666667;
    vec2 lightmapUV = vec2(lightmapX, lightmapY);

    float ditherX = float((uv1_16.x & 256u) != 0u ? 1.0 : 0.0);
    float ditherY = float((uv1_16.y & 256u) != 0u ? 1.0 : 0.0);
    vec2 ditheringAndMaskTinting = vec2(ditherX, ditherY);

    // water: the transparent faces with a blue tint and a see-through alpha, top faces are the ones that
    // do not sit on a block boundary (a water surface is lower than the full block)
    vec2 water = vec2(0.0, 0.0);
#if defined(TRANSPARENT) && !(defined(RENDER_AS_BILLBOARDS) || defined(SEASONS)) && !defined(INSTANCING)
    if (a_color0.b > 0.3 && a_color0.a < 0.95) {
        water.x = 1.0;
        if (fract(a_position.y) > 0.0) {
            water.y = 1.0;
            float waterDist = length(ViewPositionAndTime.xyz - worldPos);
            if (waterDist < BR_WATER_WAVE_RANGE) {
                vec2 wavePos = brWaterCoord(worldPos.xz - ViewPositionAndTime.xz, CameraPosition.xz);
                worldPos.y += brWaterDisplacement(wavePos, ViewPositionAndTime.w, waterDist);
            }
        }
    }
#endif

    // lava: the top face of a lava block swells slowly, the pattern of the surface is made in the fragment
#if !defined(ALPHA_TEST) && !defined(RENDER_AS_BILLBOARDS) && !defined(INSTANCING)
    if (brIsLavaTop(a_color0, fract(a_position), lightmapUV, uv0)) {
        water.x = -1.0;
        vec3 lavaRel = worldPos - ViewPositionAndTime.xyz;
        worldPos.y += brLavaSwell(lavaRel.xz + CameraPosition.xz, ViewPositionAndTime.w);
    }
#endif

    // plants, leaves, vines and hanging lanterns move in the wind
#if defined(ALPHA_TEST) && !defined(RENDER_AS_BILLBOARDS) && !defined(INSTANCING)
    {
        vec3 relPos = worldPos - ViewPositionAndTime.xyz;
        float waveDist = length(relPos);
        if (waveDist < BR_WAVE_RANGE) {
#ifdef SEASONS
            bool seasons = true;
#else
            bool seasons = false;
#endif
            worldPos = brWaveVertex(worldPos, a_position, relPos + CameraPosition.xyz, color, uv0, lightmapUV, float(textureSize(s_MatTexture, 0).y), waveDist, brDetectRain(FogAndDistanceControl.xyz), ViewPositionAndTime.w, seasons);
        }
    }
#endif

    vec4 position = jitterVertexPosition(worldPos);

    float camDist = length(ViewPositionAndTime.xyz - worldPos);
    float fogIntensity = clamp((((camDist / fogAndDist.z) + RenderChunkFogAlpha.x) - fogAndDist.x) / (fogAndDist.y - fogAndDist.x), 0.0, 1.0);
    vec4 fog = vec4(FogColor.rgb, fogIntensity);

#if defined(TRANSPARENT)
    if (a_color0.a < 0.95) {
        color.a = mix(a_color0.a, 1.0, clamp(camDist / FogAndDistanceControl.w, 0.0, 1.0));
    }
#endif

    v_clipPosition = position;
    v_worldPosition = vec4(worldPos, 1.0);
    v_color0 = color;
    v_ditheringAndMaskTinting = ditheringAndMaskTinting;
    v_fog = fog;
    v_lightmapUV = lightmapUV;
    v_texcoord0 = uv0;
    v_water = water;
    v_worldPos = worldPos;
    gl_Position = position;
}
