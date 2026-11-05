#ifndef BR_SUNMOON_H
#define BR_SUNMOON_H

#include "utils.h"

// rotates v around the x axis by tilt, then around the y axis by yaw (radians)
vec3 brRotateOrbit(vec3 v, float tilt, float yaw) {
  float ct = cos(tilt);
  float st = sin(tilt);
  v = vec3(v.x, ct*v.y - st*v.z, st*v.y + ct*v.z);

  float cy = cos(yaw);
  float sy = sin(yaw);
  return vec3(cy*v.x - sy*v.z, v.y, sy*v.x + cy*v.z);
}

// direction (normalized) of the sun/moon along its path
// same space as the vertex position from brSunMoonPosition
vec3 brGetSunDirection(float TIME_OF_DAY) {
  float a = 2.0 * BR_PI * TIME_OF_DAY;
  return brRotateOrbit(vec3(-sin(a), cos(a), 0.0), brDegToRad(BR_SUN_PATH_TILT), brDegToRad(BR_SUN_PATH_YAW));
}

vec3 brGetMoonDirection(float TIME_OF_DAY) {
  float a = 2.0 * BR_PI * TIME_OF_DAY;
  return brRotateOrbit(vec3(sin(a), -cos(a), 0.0), brDegToRad(BR_MOON_PATH_TILT), brDegToRad(BR_MOON_PATH_YAW));
}

// modelDir: xy of the model origin, i.e. mul(u_model[0], vec4(0.0, 0.0, 0.0, 1.0)).xy
bool brDetectSun(vec2 modelDir, float TIME_OF_DAY) {
  float a = 2.0 * BR_PI * TIME_OF_DAY;
  return dot(vec2(-sin(a), cos(a)), modelDir) > 0.0;
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

// world position of a sun/moon quad vertex
// the quad faces the camera and stays upright against the horizon
vec4 brSunMoonPosition(vec3 pos, vec2 modelDir, float TIME_OF_DAY) {
  bool isSun = brDetectSun(modelDir, TIME_OF_DAY);

  vec3 dir = brGetMoonDirection(TIME_OF_DAY);
  float texAngle = brDegToRad(BR_MOON_TILT);
  if (isSun) {
    dir = brGetSunDirection(TIME_OF_DAY);
    texAngle = brDegToRad(BR_SUN_TILT);
  }

  // quad axes: right is parallel to the horizon, up is perpendicular to dir and right
  vec3 right = cross(dir, vec3(0.0, 1.0, 0.0));
  float rightLen = length(right);
  if (rightLen > 0.001) {
    right /= rightLen;
  } else {
    right = vec3(1.0, 0.0, 0.0); // dir points straight up or down
  }
  vec3 up = cross(right, dir);

  pos = brScaleSunMoon(pos, isSun);
  vec2 p = brRotate2D(pos.xz, texAngle);

  float skyDistance = 300.0; // distance from the camera
  float quadScale = 70.0;    // half size of the quad at size 1.0
  return vec4(dir*skyDistance + (right*p.x + up*p.y)*quadScale, 1.0);
}

#endif
