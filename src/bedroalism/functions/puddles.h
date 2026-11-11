#ifndef BR_PUDDLES_H
#define BR_PUDDLES_H

#include "fog.h"

// One ring of a rain drop per cell: [0.0, 1.0] where the ring is. p: world xz in blocks
float brRainRing(vec2 p, float t, float cells, float rate) {
  vec2 q = p*cells;
  vec2 cell = floor(q);
  float h = brHash(cell);
  float phase = fract(t*rate + h*7.0);
  vec2 centre = vec2_splat(0.5) + 0.3*vec2(brHash(cell + 11.0) - 0.5, brHash(cell + 23.0) - 0.5);
  float d = length(fract(q) - centre);
  float radius = phase*0.5;
  float ring = smoothstep(0.07, 0.0, abs(d - radius))*(1.0 - phase)*(1.0 - phase);
  return ring*step(0.6, h); // not every cell has a drop
}

// A short flash where a drop hits dry wet ground. p: world xz in blocks
float brRainSplash(vec2 p, float t) {
  vec2 q = p*5.0;
  vec2 cell = floor(q);
  float h = brHash(cell + 41.0);
  float phase = fract(t*1.7 + h*9.0);
  float d = length(fract(q) - 0.5);
  return smoothstep(0.16, 0.0, d)*(1.0 - smoothstep(0.0, 0.22, phase))*step(0.55, h);
}

// Rain wets open ground: it darkens, and flat ground gets puddles that mirror the sky and show the rings of the drops.
// col: lit colour of the surface. wp: world position. skyLight: the sky light of the lightmap, [0.0, 1.0]
vec3 brWetSurface(vec3 col, vec3 wp, vec3 geoNormal, vec3 viewDir, float skyLight, bool opaque, float t, brEnvironment env) {
  float rain = env.rainFactor*(env.overworld && !env.underwater ? 1.0 : 0.0);
  float wet = rain*smoothstep(0.7, 0.9, skyLight); // only where the sky is open
  if (wet < 0.01) return col;

  float upFacing = smoothstep(0.75, 0.95, geoNormal.y);
  col *= 1.0 - BR_WET_DARKEN*wet*mix(0.5, 1.0, upFacing);
  if (!opaque || upFacing < 0.01) return col;

  // where the puddles are: large patches that grow with the rain
  float n = 0.65*brNoise(wp.xz*0.35) + 0.35*brNoise(wp.xz*0.9 + 5.0);
  float limit = mix(0.78, 0.42, wet*BR_PUDDLES);
  float puddle = smoothstep(limit, limit + 0.06, n)*wet*upFacing*min(BR_PUDDLES, 1.0);

  // drops on the ground outside the puddles
  col += vec3_splat(0.5)*brRainSplash(wp.xz, t)*(1.0 - puddle)*wet*upFacing*BR_RAIN_SPLASH*0.25*skyLight;

  if (puddle > 0.01) {
    brSkyColor sky = brSkyColors(env);
    float ring = brRainRing(wp.xz, t, 3.0, 0.8) + 0.5*brRainRing(wp.xz + 7.3, t, 5.0, 1.3);
    ring *= BR_RAIN_SPLASH;
    // the ring bends the mirrored direction a little
    vec3 r = normalize(vec3(viewDir.x, -viewDir.y*(1.0 - 0.5*ring) + 0.04, viewDir.z));
    vec3 mirror = brRenderSky(sky, r);
    float fresnel = brFresnel(clamp(-viewDir.y, 0.0, 1.0), 0.02);
    float refl = min(0.4 + 0.55*fresnel, 0.92);
    vec3 water = mix(col*0.7, mirror*(0.6 + 0.4*skyLight), refl);
    water += ring*vec3(0.22, 0.24, 0.30)*skyLight;
    col = mix(col, water, puddle);
  }
  return col;
}

#endif
