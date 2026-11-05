#ifndef BR_COLORCORRECTION_H
#define BR_COLORCORRECTION_H

#include "detection.h"

// exposure for the current time of day, day + twilight + night = 1.0 (see detection.h)
float brGetExposure(brEnvironment env) {
  return env.day*BR_EXPOSURE_DAY + env.twilight*BR_EXPOSURE_TWILIGHT + env.night*BR_EXPOSURE_NIGHT;
}

// Uchimura 2017, "HDR theory and practice", with the original parameters
float brUchimura(float x) {
  const float P = 1.0;  // max display brightness
  const float a = 1.0;  // contrast
  const float m = 0.22; // linear section start
  const float l = 0.4;  // linear section length
  const float c = 1.33; // black tightness
  const float b = 0.0;  // pedestal

  float l0 = ((P - m) * l) / a;
  float s0 = m + l0;
  float s1 = m + a * l0;
  float c2 = (a * P) / (P - s1);
  float cp = -c2 / P;

  float w0 = 1.0 - smoothstep(0.0, m, x);
  float w2 = step(s0, x);
  float w1 = 1.0 - w0 - w2;

  float toe = m * pow(x / m, c) + b;
  float lin = m + a * (x - m);
  float shoulder = P - (P - s1) * exp(cp * (x - s0));

  return toe*w0 + lin*w1 + shoulder*w2;
}

vec3 brTonemap(vec3 col) {
  col = max(col, vec3(0.0, 0.0, 0.0));
  return vec3(brUchimura(col.r), brUchimura(col.g), brUchimura(col.b));
}

// col: linear scene color, returns the color to display
vec3 brColorCorrection(vec3 col, float exposure) {
  col = brTonemap(col*exposure);
  return pow(col, vec3_splat(1.0 / BR_GAMMA));
}

#endif
