#ifndef BR_UTILS_H
#define BR_UTILS_H

#define BR_PI 3.141592

vec2 brRotate2D(vec2 v, float t) {
  float c = cos(t);
  float s = sin(t);
  return vec2(c*v.x - s*v.y, s*v.x + c*v.y);
}

float brDegToRad(float t) {
  return 0.0174533 * t;
}

#endif
