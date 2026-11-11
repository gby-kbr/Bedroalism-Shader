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

#endif
