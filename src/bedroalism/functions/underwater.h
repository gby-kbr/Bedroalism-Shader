#ifndef BR_UNDERWATER_H
#define BR_UNDERWATER_H

#include "water.h"

// Clear water takes red out of the light first, then green, blue goes on for a long way. Light from
// a surface point at distance d keeps exp(-sigma*d) of its color, the rest is replaced by the light the
// water scatters on the way, which has the same color as the surface water (BR_WATER_COLOR).

vec3 brUnderwaterAbsorption() {
  return vec3(0.22, 0.045, 0.018)*BR_UNDERWATER_DENSITY; // per block
}

// color of what is seen at distance dist under water
// skyLight: sky light of the seen position, 0.0 ~ 1.0
vec3 brUnderwaterFog(vec3 col, float dist, brEnvironment env, float skyLight) {
  vec3 T = exp(-brUnderwaterAbsorption()*dist);
  return col*T + brWaterScatterColor(env, skyLight)*(1.0 - T);
}

// The surface seen from below, as clear as the surface seen from above: the same transparency at every
// angle (no total internal reflection, so no round window). The sun and the moon are the real ones,
// seen through it.
// viewDir: normalized, pointing up. nrm: normal of the surface, pointing down (toward the eye)
// returns the color and opacity, to be blended over what is behind the surface
vec4 brUnderwaterSurface(vec3 viewDir, vec3 nrm, brEnvironment env) {
  vec3 scatter = brWaterScatterColor(env, 0.9);
  nrm = normalize(mix(nrm, vec3(0.0, -1.0, 0.0), BR_UNDERWATER_SURFACE_SMOOTH)); // calmer waves from below
  float eta = 1.333;
  float cosI = clamp(-dot(viewDir, nrm), 0.0, 1.0);
  float k = 1.0 - eta*eta*(1.0 - cosI*cosI);
  float cosT = sqrt(max(k, 0.0));

  float fresnel = min(brFresnel(cosT, 0.02), 0.3); // capped, so low angles stay clear
  float hide = -log(1.0 - clamp(BR_WATER_OPACITY, 0.01, 0.95));
  float transmit = exp(-hide/max(cosI, 0.12)); // the same body as from above
  float alpha = 1.0 - (1.0 - fresnel)*transmit;

  return vec4(scatter, alpha);
}

#endif
