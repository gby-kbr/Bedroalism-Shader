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

// Looking up through the surface from below: the surface keeps the color of the water and only the sun
// and the moon show through it. The ray bends when it leaves the water, so the discs sit where the
// refracted ray points. Outside the angle asin(1/1.333) = 48.6 degrees the surface mirrors the water.
// viewDir: normalized, pointing up. nrm: normal of the surface, pointing down (toward the eye)
vec3 brUnderwaterSurface(vec3 viewDir, vec3 nrm, brEnvironment env, float skyVisible) {
  vec3 scatter = brWaterScatterColor(env, 0.9);
  float eta = 1.333;
  float cosI = clamp(-dot(viewDir, nrm), 0.0, 1.0);
  float k = 1.0 - eta*eta*(1.0 - cosI*cosI);
  if (k <= 0.0) {
    return scatter; // total internal reflection
  }
  float cosT = sqrt(k);
  vec3 outDir = eta*viewDir + (eta*cosI - cosT)*nrm; // direction of the ray in the air
  outDir = normalize(vec3(outDir.x, max(outDir.y, 0.0), outDir.z));

  float fresnel = brFresnel(cosT, 0.02);
  vec3 discs = brSunMoonDisc(outDir, env, 8.0, 3.0)*exp(-brUnderwaterAbsorption()*(1.5/max(cosI, 0.2)))*(1.0 - fresnel);
  return scatter + discs*skyVisible;
}

#endif
