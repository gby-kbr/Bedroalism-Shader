#ifndef BR_COLORCORRECTION_H
#define BR_COLORCORRECTION_H

// Filmic curve after Hable, "Filmic Tonemapping Operators" (Uncharted 2): a soft toe that
// deepens the shadows, a straight middle with contrast, and a long shoulder that rolls bright
// values off gently instead of clipping them.
vec3 brFilmicCurve(vec3 x) {
  const float A = 0.15; // shoulder strength
  const float B = 0.50; // linear strength
  const float C = 0.10; // linear angle
  const float D = 0.20; // toe strength
  const float E = 0.02; // toe numerator
  const float F = 0.30; // toe denominator
  return ((x*(A*x + C*B) + D*E) / (x*(A*x + B) + D*F)) - E/F;
}

vec3 brTonemap(vec3 col) {
  const float W = 11.2; // linear white point
  col = max(col, vec3(0.0, 0.0, 0.0));
  vec3 white = brFilmicCurve(vec3_splat(W));
  return clamp(brFilmicCurve(col*BR_FILMIC_BIAS) / white, 0.0, 1.0);
}

// col: linear scene color, returns the color to display
vec3 brColorCorrection(vec3 col) {
  col = brTonemap(col*BR_EXPOSURE);
  return pow(col, vec3_splat(1.0 / BR_GAMMA));
}

#endif
