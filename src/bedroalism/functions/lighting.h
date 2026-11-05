#ifndef BR_LIGHTING_H
#define BR_LIGHTING_H

#include "detection.h"

// Multiplier for the light of a block, darkens only the sky light at night in the overworld.
// Day, sunrise, sunset, nether, end, caves (no sky light) and torch light stay at 1.0.
// lightmapUV: x = block light, y = sky light, both 0.0 ~ 1.0
float brGetNightAmbient(vec2 lightmapUV, float TIME_OF_DAY, vec3 FOG_COLOR, vec3 FOG_CONTROL) {
  bool overworld = !brDetectEnd(FOG_COLOR) && !brDetectNether(FOG_COLOR, FOG_CONTROL.xy);
  if (!overworld) {
    return 1.0;
  }

  float night = brDetectNight(brDetectDayFactor(TIME_OF_DAY));
  float amount = night * lightmapUV.y * (1.0 - lightmapUV.x);
  return mix(1.0, BR_NIGHT_AMBIENT, amount);
}

#endif
