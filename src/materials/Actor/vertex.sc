$input a_position, a_color0, a_texcoord0, a_indices, a_normal
#ifdef INSTANCING
    $input i_data1, i_data2, i_data3
#endif

$output v_clipPosition, v_color0, v_fog, v_fogAdd, v_fogMul, v_light, v_texcoord0, v_worldPos

#include <bgfx_shader.sh>
#include <bedroalism/config.h>
#include <bedroalism/functions/fog.h>
#include <MinecraftRenderer.Materials/DynamicUtil.dragonh>
#include <MinecraftRenderer.Materials/FogUtil.dragonh>
#include <MinecraftRenderer.Materials/TAAUtil.dragonh>

uniform vec4 OverlayColor;
uniform vec4 TileLightColor;
uniform vec4 FogColor;
uniform vec4 FogControl;
uniform vec4 UVAnimation;
uniform mat4 Bones[8];
uniform vec4 FogAndDistanceControl;
uniform vec4 TimeOfDay;
uniform vec4 CameraPosition;
uniform vec4 ViewPositionAndTime;

void main() {
    mat4 World = u_model[0];

    World = mul(World, Bones[int(a_indices)]);

    vec2 texcoord0 = a_texcoord0;
    texcoord0 = applyUvAnimation(texcoord0, UVAnimation);

    float lightIntensity = calculateLightIntensity(World, vec4(a_normal.xyz, 0.0), TileLightColor);
    lightIntensity += OverlayColor.a * 0.35;
    vec4 light = vec4(lightIntensity * TileLightColor.rgb, 1.0);

    vec3 worldPosition;
#ifdef INSTANCING
    mat4 model = mtxFromCols(i_data1, i_data2, i_data3, vec4(0.0, 0.0, 0.0, 1.0));
    worldPosition = instMul(model, vec4(a_position, 1.0)).xyz;
#else
    worldPosition = mul(World, vec4(a_position, 1.0)).xyz;
#endif

    vec4 position = jitterVertexPosition(worldPosition);
    float cameraDepth = position.z;
    float fogIntensity = calculateFogIntensity(cameraDepth, FogControl.z, FogControl.x, FogControl.y);
    vec4 fog = vec4(FogColor.rgb, fogIntensity);

#if defined(DEPTH_ONLY) || defined(DEPTH_ONLY_OPAQUE)
    v_texcoord0 = vec2_splat(0.0);
    v_color0 = vec4_splat(0.0);
#else
    v_texcoord0 = texcoord0;
    v_color0 = a_color0;
#endif

    // fog is worked out here, once per vertex and not per pixel
    brEnvironment env = brDetectEnvironment(TimeOfDay.x, FogColor.rgb, FogAndDistanceControl.xyz);
    vec3 camPos = mul(u_invView, vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 relPos = worldPosition - camPos;
    vec3 fogMul;
    vec3 fogAdd;
    brFogTerms(relPos, relPos.xz + CameraPosition.xz, CameraPosition.y, 1.0, FogAndDistanceControl.z, ViewPositionAndTime.w, env, fog.rgb, fog.a, fogMul, fogAdd);
    // the player in the pause menu and the items in the inventory are drawn with this material too, but outside of
    // the world: there is no render distance or the model is far from the camera, and our fog would wash them out
    float inWorld = FogAndDistanceControl.z > 8.0 && length(relPos) < FogAndDistanceControl.z*1.5 ? 1.0 : 0.0;
    v_fogMul = vec4(mix(vec3_splat(1.0), fogMul, inWorld), 0.0);
    v_fogAdd = vec4(fogAdd*inWorld, 0.0);

    v_clipPosition = position;
    v_fog = fog;
    v_light = light;
    v_worldPos = worldPosition;
    gl_Position = position;
}
