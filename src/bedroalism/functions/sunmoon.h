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

// Position of the sun/moon vertex following the sun/moon path (ported from Newb X)
// pos: a_position, modelDir: see brDetectSun
vec4 brSunMoonPosition(vec3 pos, vec2 modelDir, float TIME_OF_DAY) {
  float st = 2.0 * BR_PI * TIME_OF_DAY + 0.5 * BR_PI;
  bool isSun = brDetectSun(modelDir, TIME_OF_DAY);

  pos.x = -pos.x;

  float dist = 300.0;
  float angle = 0.0;
  float tilt = 0.0;
  float yaw = 0.0;
  if (isSun) {
    dist = -dist;
    pos.x = -pos.x;
    angle = brDegToRad(BR_SUN_TILT);
    tilt = brDegToRad(BR_SUN_PATH_TILT);
    yaw = brDegToRad(BR_SUN_PATH_YAW);
  } else {
    angle = brDegToRad(BR_MOON_TILT);
    tilt = brDegToRad(BR_MOON_PATH_TILT);
    yaw = brDegToRad(BR_MOON_PATH_YAW);
  }

  pos = brScaleSunMoon(pos, isSun);
  pos.xz = mul(brRotation2D(angle + 0.5*st), pos.xz);

  vec4 wpos = vec4(dist, 70.0*pos.xz, 1.0);
  wpos.xy = mul(brRotation2D(st), -wpos.xy);
  wpos.yz = mul(brRotation2D(tilt), wpos.yz);
  wpos.xz = mul(brRotation2D(yaw), wpos.xz);

  return wpos;
}

#endif
