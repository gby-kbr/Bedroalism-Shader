$input v_clipPosition, v_color0, v_dirLight, v_fog, v_light, v_texcoord0, v_worldPos

#include <bgfx_shader.sh>
#include <MinecraftRenderer.Materials/ActorUtil.dragonh>
#include <MinecraftRenderer.Materials/FogUtil.dragonh>
#include <MinecraftRenderer.Materials/DitherUtil.dragonh>
#include <bedroalism/config.h>
#include <bedroalism/functions/detection.h>
#include <bedroalism/functions/underwater.h>
#include <bedroalism/functions/torch.h>
#include <bedroalism/functions/colorcorrection.h>
#include <bedroalism/functions/fog.h>
#include <bedroalism/functions/glow.h>

uniform vec4 ColorBased;
uniform vec4 ChangeColor;
uniform vec4 UseAlphaRewrite;
uniform vec4 TintedAlphaTestEnabled;
uniform vec4 MatColor;
uniform vec4 OverlayColor;
uniform vec4 TileLightColor;
uniform vec4 MultiplicativeTintColor;
uniform vec4 ActorFPEpsilon;
uniform vec4 HudOpacity;
uniform vec4 DitherParams;
uniform vec4 DitherParams2[3];
uniform vec4 DitheringEnabledToggle;
uniform vec4 TimeOfDay;
uniform vec4 FogColor;
uniform vec4 FogAndDistanceControl;
uniform vec4 CameraPosition;
uniform vec4 ViewPositionAndTime;

SAMPLER2D_AUTOREG(s_MatTexture);
SAMPLER2D_AUTOREG(s_MatTexture1);

void main() {
#if defined(DEPTH_ONLY) || defined(DEPTH_ONLY_OPAQUE)
    gl_FragColor = vec4_splat(0.0);
    return;
#else

    vec4 albedo = getActorAlbedoNoColorChange(v_texcoord0, s_MatTexture, s_MatTexture1, MatColor);

    bool ditherDiscard = false;
    if (DitheringEnabledToggle.x != 0.0) {
        vec3 camPos = mul(u_invView, vec4(0.0, 0.0, 0.0, 1.0)).xyz;
        ditherDiscard = evaluateDitherDiscard(v_clipPosition, v_worldPos, camPos, u_view, DitherParams, DitherParams2[0]);
    }

#if defined(ALPHA_TEST)
    float alpha = mix(albedo.a, albedo.a * OverlayColor.a, TintedAlphaTestEnabled.x);
    if (ditherDiscard || shouldDiscard(albedo.rgb, alpha, ActorFPEpsilon.x)) {
        discard;
    }
#endif

#if defined(CHANGE_COLOR_MULTI)
    albedo = applyMultiColorChange(albedo, ChangeColor.rgb, MultiplicativeTintColor.rgb);
#elif defined(CHANGE_COLOR)
    albedo = applyColorChange(albedo, ChangeColor, albedo.a);
    albedo.a *= ChangeColor.a;
#endif

#if defined(ALPHA_TEST)
    albedo.a = max(UseAlphaRewrite.x, albedo.a);
#endif

    brEnvironment env = brDetectEnvironment(TimeOfDay.x, FogColor.rgb, FogAndDistanceControl.xyz);
    vec3 eyePos = mul(u_invView, vec4(0.0, 0.0, 0.0, 1.0)).xyz;

    // the light of the sky shaded by the direction of the face, plus our own torch light
    vec3 ambient = brEntityAmbient(TileLightColor.rgb)*v_dirLight;
    vec3 torch = brTorchLight(brEntityTorchLevel(TileLightColor.rgb), ambient, env, v_worldPos.xz - eyePos.xz + CameraPosition.xz, ViewPositionAndTime.w);
    albedo = applyActorDiffuse(albedo, v_color0.rgb, vec4(ambient + torch, 1.0), ColorBased.x, OverlayColor);

#if defined(TRANSPARENT)
    albedo = applyHudOpacity(albedo, HudOpacity.x);
    if (ditherDiscard) {
        albedo.a = 0.0;
    }
#endif

    if (env.underwater) {
        // same clear water haze as the terrain
        albedo.rgb = brUnderwaterFog(albedo.rgb, length(v_worldPos - eyePos), env, 0.8);
    } else {
        albedo.rgb = brApplyFog(albedo.rgb, v_worldPos - eyePos, CameraPosition.y, 1.0, FogAndDistanceControl.z, env, v_fog.rgb, v_fog.a);
    }
    // dazzle when looking toward the sun; the sky light is estimated from the light color of the tile
    albedo.rgb = brApplyGlare(albedo.rgb, normalize(v_worldPos - eyePos), env, clamp(brEntityAmbient(TileLightColor.rgb).b, 0.0, 1.0));
    albedo.rgb = brColorCorrection(albedo.rgb);
    gl_FragColor = albedo;
#endif
}
