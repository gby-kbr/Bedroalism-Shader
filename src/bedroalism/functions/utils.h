#ifndef BR_UTILS_H
#define BR_UTILS_H

#define BR_PI 3.141592

// 2D rotation matrix for angle t (radians)
mat2 brRotation2D(float t) {
  float sint = sin(t);
  float cost = cos(t);
  return mtxFromRows(vec2(cost, -sint), vec2(sint, cost));
}

float brDegToRad(float t) {
  return 0.0174533 * t;
}

#endif
