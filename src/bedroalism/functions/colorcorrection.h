#ifndef BR_COLORCORRECTION_H
#define BR_COLORCORRECTION_H

// Uchimura 2017, "HDR theory and practice"
float brUchimura(float x) {
  const float P = BR_UCHIMURA_P;
  const float a = BR_UCHIMURA_A;
  const float m = BR_UCHIMURA_M;
  const float l = BR_UCHIMURA_L;
  const float c = BR_UCHIMURA_C;
  const float b = BR_UCHIMURA_B;

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

vec3 brColorCorrection(vec3 col) {
  col = brTonemap(col*BR_EXPOSURE);
  return pow(col, vec3_splat(1.0 / BR_GAMMA));
}

#endif
