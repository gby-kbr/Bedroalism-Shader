#ifndef BR_SUNMOON_H
#define BR_SUNMOON_H

#include "utils.h"

// modelDir: xy of the model origin, i.e. mul(u_model[0], vec4(0.0, 0.0, 0.0, 1.0)).xy
bool brDetectSun(vec2 modelDir, float TIME_OF_DAY) {
  float st = 2.0 * BR_PI * TIME_OF_DAY + 0.5 * BR_PI;
  return dot(vec2(cos(st), sin(st)), modelDir) > 0.0;
}

float brGetSunMoonSize(bool isSun) {
  if (isSun) {
    return BR_SUN_SIZE;
  }
  return BR_MOON_SIZE;
}

// pos: local vertex position of the sun/moon quad (a_position)
vec3 brScaleSunMoon(vec3 pos, bool isSun) {
  pos.xz *= brGetSunMoonSize(isSun);
  return pos;
}

#endif
