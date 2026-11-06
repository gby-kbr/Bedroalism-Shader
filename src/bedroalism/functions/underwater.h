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

// Looking up through the surface from below. The ray bends when it leaves the water, so the whole sky
// is squeezed into a circle around straight up, the angle where it ends is asin(1/1.333) = 48.6 degrees.
// Outside that circle the surface mirrors the water below, shown as the water color.
// viewDir: normalized, pointing up. nrm: normal of the surface, pointing down (toward the eye)
vec3 brUnderwaterSurface(sampler2D noiseTex, vec3 viewDir, vec3 nrm, brEnvironment env, brSkyColor sky, float skyVisible, float time, vec2 fragCoord) {
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

  float fresnel = brFresnel(cosT, 0.02); // goes to 1.0 at the edge of the circle
  vec3 skySeen = brSkyRadiance(noiseTex, outDir, env, sky, vec2(8.0, 3.0), time, fragCoord);
  vec3 keep = exp(-brUnderwaterAbsorption()*(1.5/max(cosI, 0.2))); // the ray also crossed a little water
  return mix(skySeen*keep, scatter, fresnel)*skyVisible + scatter*(1.0 - skyVisible);
}

#endif
