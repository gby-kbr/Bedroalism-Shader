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

// The surface seen from below, as clear as the surface seen from above: the same transparency, only
// the angle where the water mirrors itself (total internal reflection, asin(1/1.333) = 48.6 degrees) makes
// it opaque. The mirror shows the water color, the sun and the moon show at the direction the ray has
// once it leaves the water.
// viewDir: normalized, pointing up. nrm: normal of the surface, pointing down (toward the eye)
// returns the color and opacity, to be blended over what is behind the surface
vec4 brUnderwaterSurface(vec3 viewDir, vec3 nrm, brEnvironment env, float skyVisible) {
  vec3 scatter = brWaterScatterColor(env, 0.9);
  float eta = 1.333;
  float cosI = clamp(-dot(viewDir, nrm), 0.0, 1.0);
  float k = 1.0 - eta*eta*(1.0 - cosI*cosI);
  if (k <= 0.0) {
    return vec4(scatter, 1.0); // total internal reflection
  }
  float cosT = sqrt(k);
  vec3 outDir = eta*viewDir + (eta*cosI - cosT)*nrm; // direction of the ray in the air
  outDir = normalize(vec3(outDir.x, max(outDir.y, 0.0), outDir.z));

  float fresnel = brFresnel(cosT, 0.02); // 1.0 at the edge of the circle
  float hide = -log(1.0 - clamp(BR_WATER_OPACITY, 0.01, 0.95));
  float transmit = exp(-hide/max(cosI, 0.12)); // the same body as from above
  float alpha = 1.0 - (1.0 - fresnel)*transmit;

  vec3 discs = brSunMoonDisc(outDir, env, 8.0, 3.0)*(1.0 - fresnel)*skyVisible;
  alpha = max(alpha, clamp(dot(discs, vec3(0.5, 0.5, 0.5)), 0.0, 1.0));
  return vec4(scatter + discs, alpha);
}

#endif
