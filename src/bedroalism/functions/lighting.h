#ifndef BR_LIGHTING_H
#define BR_LIGHTING_H

#include "sky.h"

// color of the sun or moon light on the world: white warm at noon, the pale dawn color
// of the sky at sunrise and sunset, cool dim blue at night, greyer in the rain
vec3 brSunMoonLightColor(brEnvironment env, brSkyColor sky) {
  vec3 dawnCol = sky.dawnHorizon/max(max(sky.dawnHorizon.r, sky.dawnHorizon.g), sky.dawnHorizon.b);
  dawnCol = mix(vec3_splat(1.0), dawnCol, BR_DAWN_LIGHT_AMOUNT);

  vec3 col = mix(BR_NOON_LIGHT_COL, dawnCol, sky.dawn);
  col = mix(col, BR_MOONLIGHT_COL, smoothstep(0.0, 1.0, env.night));
  return mix(col, vec3_splat(1.0), 0.7*env.rainFactor);
}

// Light of torches and of every block that glows (lava, glowstone, lanterns, sea lanterns...): the game lights
// them all through the block light of the lightmap. Where the block light dominates over the sky light, the
// grey of the vanilla lightmap is replaced by BR_TORCH_COL, so the places lit by fire look orange instead of
// yellow white. The light also gets brighter near the source and flickers a little.
// lightMap: the vanilla lightmap color. blockLight, skyLight: lightmap uv, [0.0, 1.0]. t: time in seconds
vec3 brTorchLight(vec3 lightMap, float blockLight, float skyLight, brEnvironment env, float t) {
  float att = smoothstep(0.1, 1.0, blockLight);
  att *= att;
  // how much daylight reaches the point: the torch only shows in the shade and in caves by day
  float sky = smoothstep(0.3, 0.9, skyLight)*(env.overworld ? env.day + 0.35*env.twilight + 0.05 : 0.0);
  float share = att/(att + sky + 0.02);

  vec3 torchCol = BR_TORCH_COL/max(dot(BR_TORCH_COL, vec3(0.333, 0.333, 0.333)), 0.001); // same brightness as the grey
  vec3 grey = vec3_splat(dot(lightMap, vec3(0.333, 0.333, 0.333)));
  vec3 light = mix(lightMap, grey*torchCol, clamp(share*BR_TORCH_AMOUNT, 0.0, 1.0));

  float flame = 0.5 + 0.25*sin(t*13.0) + 0.25*sin(t*5.7 + 1.3); // [0.0, 1.0]
  float boost = 1.0 + BR_TORCH_INTENSITY*att*(1.0 - sky)*(1.0 - BR_TORCH_FLICKER*flame);
  return light*boost;
}

#endif
