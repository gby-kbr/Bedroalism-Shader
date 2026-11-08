#ifndef BR_UTILS_H
#define BR_UTILS_H

#define BR_PI 3.141592

// rotates v counter-clockwise by t (radians)
vec2 brRotate2D(vec2 v, float t) {
  float c = cos(t);
  float s = sin(t);
  return vec2(c*v.x - s*v.y, s*v.x + c*v.y);
}

float brDegToRad(float t) {
  return 0.0174533 * t;
}

// hashes without sin(): stable for large positions. result in [0.0, 1.0)
float brHash12(vec2 p) {
  vec3 p3 = fract(vec3(p.x, p.y, p.x)*0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y)*p3.z);
}

float brHash13(vec3 p) {
  vec3 p3 = fract(p*0.1031);
  p3 += dot(p3, p3.zyx + 31.32);
  return fract((p3.x + p3.y)*p3.z);
}

vec2 brHash22(vec2 p) {
  vec3 p3 = fract(vec3(p.x, p.y, p.x)*vec3(0.1031, 0.1030, 0.0973));
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.xx + p3.yz)*p3.zy);
}

#endif
