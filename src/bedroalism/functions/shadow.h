#ifndef BR_SHADOW_H
#define BR_SHADOW_H

#include "detection.h"
#include "sky.h"

// Shadows of the blocks. The game gives no depth from the sun, so there is no shadow map: a shadow is
// decided from what a block knows about itself. Two parts, both 0.0 lit ~ 1.0 fully shadowed:
//   - the light of the block: sky light below the top of the range means a roof or an overhang, torch
//     light takes the shadow away again (a lit cave wall is not dark)
//   - the faces turned away from the light: this part follows the sun, and the moon at night. It is a
//     wrapped diffuse term, so a face darkens smoothly as it turns from the light instead of switching, and
//     the tops get a little of it too: a low sun lights the ground less than a high one
// The shade is colored like the air does it: a place the sun does not reach is lit by the blue sky alone, so
// shadows turn cool, and a place the sun reaches turns slightly warm (BR_SHADOW_TINT)
// A shadow is the contrast with a lit place, so it only exists where there is light to be shut out:
//   - outdoors, or just inside the edge of a roof. Deeper indoors and in caves the sky light is gone and so
//     is the shadow (brOutdoor)
//   - while the sun shines. At night the moon is too weak and in rain the light comes from the whole sky,
//     the shadows almost disappear (brShadowVisibility)

// shadow from the light level of the block
// lightmapUV: x = block light, y = sky light, both 0.0 ~ 1.0
float brLightmapShadow(vec2 lightmapUV) {
  float shadow = 1.0 - smoothstep(0.740, 0.875, pow(lightmapUV.y, 2.0)); // one level below the top is a soft edge
  return mix(shadow, 0.0, clamp(pow(lightmapUV.x*1.2, 6.0), 0.0, 1.0)); // above 1.0 the mix would brighten
}

// the light that counts: the sun while it is up, the moon after it, blended when both are near the horizon
// returns dot(faceNormal, direction of the light), 1.0 faces the light, -1.0 faces away
float brFaceToLight(vec3 faceNormal, brEnvironment env) {
  float sunUp = smoothstep(-0.10, 0.10, env.sunDir.y);
  return mix(dot(faceNormal, env.moonDir), dot(faceNormal, env.sunDir), sunUp);
}

// shadow of the faces that look away from the light. The top faces never get it, they look at the sky
// faceNormal: normal of the face, outward (facing the camera). vertexG: green of the vertex color, the
// face brightness the game bakes in (1.0 top, 0.8 north and south, 0.6 east and west, 0.5 bottom)
float brSideShadow(vec3 faceNormal, float vertexG, brEnvironment env) {
  float fixedShadow = 1.0 - smoothstep(0.62, 0.64, vertexG); // the faces the game darkens, always the same
  float follow = 1.0 - smoothstep(-0.10, 0.40, brFaceToLight(faceNormal, env)); // soft terminator
  follow *= mix(1.0, 0.30, step(0.5, faceNormal.y));         // the tops only feel a low sun a little
  float amount = BR_SHADOW_SUN_FOLLOW*float(env.overworld);  // nether and end have no sun
  return mix(fixedShadow, follow, amount);
}

// 1.0 where the sky is open or near, fades to 0.0 as the sky light runs out indoors
float brOutdoor(float skyLight) {
  return smoothstep(0.15, 0.50, skyLight);
}

// how visible the shadows are: full by day, weaker at sunrise and sunset, almost gone at night,
// nearly gone in rain, none in the nether and the end
float brShadowVisibility(brEnvironment env) {
  float light = env.day + 0.60*env.twilight + 0.06*env.night;
  return light*(1.0 - 0.90*env.rainFactor)*float(env.overworld);
}

// color of the light in the shade: only the sky reaches it, so it follows the sky color, kept to the
// same brightness so that only the hue changes
vec3 brShadeTint(brEnvironment env) {
  brSkyColor sky = brSkyColors(env);
  vec3 c = sky.zenith*0.5 + sky.horizon*0.5;
  c = c/max(dot(c, vec3(0.299, 0.587, 0.114)), 0.0001);
  c = clamp(c, vec3(0.65, 0.65, 0.65), vec3(1.30, 1.30, 1.30));
  return mix(vec3(1.0, 1.0, 1.0), c, BR_SHADOW_TINT);
}

// color of the light where the sun reaches: white by day, warm when the sun is low
vec3 brSunTint(brEnvironment env) {
  vec3 c = mix(vec3(1.00, 0.97, 0.90), vec3(1.25, 0.88, 0.62), env.twilight);
  c = c/max(dot(c, vec3(0.299, 0.587, 0.114)), 0.0001);
  return mix(vec3(1.0, 1.0, 1.0), c, BR_SHADOW_TINT);
}

// color of a block with its shadows
vec3 brShadow(vec3 col, vec2 lightmapUV, vec3 faceNormal, float vertexG, brEnvironment env) {
  float visible = brOutdoor(lightmapUV.y)*brShadowVisibility(env);
  float roof = brLightmapShadow(lightmapUV);
  float side = brSideShadow(faceNormal, vertexG, env);
  float shade = 1.0 - (1.0 - roof)*(1.0 - side); // shaded by either
  float amount = BR_SHADOW_STRENGTH*visible;

  vec3 cool = brShadeTint(env);
  vec3 warm = brSunTint(env);
  // shaded: darker and cool. lit: slightly warm. both fade out with the visibility of shadows
  vec3 tint = mix(warm, cool, shade);
  col *= mix(vec3(1.0, 1.0, 1.0), tint, visible);
  col *= 1.0 - amount*shade;
  return col;
}

#endif
