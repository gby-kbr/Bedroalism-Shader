#ifndef BR_CAUSTICS_H
#define BR_CAUSTICS_H

#include "water.h"

// ---------------------------------------------------------------------------------------------
// Caustics
//
// Light that enters the water through a curved surface is focused where the surface bulges like a lens and
// spread out where it hollows. Sunlight falls through the surface and travels a distance d to the bottom.
// To first order a ray that enters at p lands at p + d*(1 - 1/n)*grad(h), so the light density on the bottom
// is 1/det(I + s*H) with H the matrix of second derivatives of the surface height h and s = d*(1 - 1/n).
// Where det comes close to 0 the light piles up in bright, thin lines that swim as the waves move.
// The surface used is the visible wave field of water.h plus 4 short, steep ripples that only the light sees.
// Only under water in the overworld: the function returns the color unchanged anywhere else.
// ---------------------------------------------------------------------------------------------

#define BR_CAUSTIC_SURFACE_Y 62.8   // height of the water surface of the sea
#define BR_WATER_INDEX       1.333  // refraction index of water

// second derivatives (xx, xz, zz) of one wave of water.h
vec3 brWaveHessian(vec2 p, float t, vec2 K, float slope) {
  vec2 k = K*(2.0*BR_PI/512.0);
  float kl = length(k);
  float phase = dot(k, p) - sqrt(9.8*kl)*BR_WATER_WAVE_SPEED*t;
  float amp = slope*BR_WATER_WAVE_HEIGHT/kl;
  float s = sin(phase);
  float c = cos(phase);
  float d2 = amp*exp(s - 1.0)*(c*c - s);
  return vec3(k.x*k.x, k.x*k.y, k.y*k.y)*d2;
}

// the matrix of second derivatives of the surface. fine: 0.0 ~ 1.0, how much of the short ripples counts
vec3 brCausticHessian(vec2 p, float t, float fine) {
  vec3 h = brWaveHessian(p, t, vec2(72.0, 26.0), 0.065);
  h += brWaveHessian(p, t, vec2(-41.0, 95.0), 0.058);
  h += brWaveHessian(p, t, vec2(150.0, -60.0), 0.052);
  h += brWaveHessian(p, t, vec2(-120.0, -250.0), 0.046);
  h += brWaveHessian(p, t, vec2(330.0, 190.0), 0.040);
  h += brWaveHessian(p, t, vec2(-300.0, 520.0), 0.032);
  vec3 f = brWaveHessian(p, t, vec2(610.0, -390.0), 0.030);
  f += brWaveHessian(p, t, vec2(-480.0, 700.0), 0.028);
  f += brWaveHessian(p, t, vec2(900.0, 260.0), 0.024);
  f += brWaveHessian(p, t, vec2(-250.0, -1050.0), 0.022);
  return h + f*fine;
}

// light density on the bottom, 1.0 is the light of a flat surface
// p: where the light entered the surface (x, z). depth: blocks of water above the point
float brCausticLight(vec2 p, float t, float depth, float fine) {
  vec3 h = brCausticHessian(p, t, fine);
  float s = depth*(1.0 - 1.0/BR_WATER_INDEX);
  float det = (1.0 + s*h.x)*(1.0 + s*h.z) - s*s*h.y*h.y;
  return min(1.0/max(abs(det), 0.12), 8.0);
}

// col: the lit color of a surface. worldXZ, worldY: where it is. camDist: distance to the camera, blocks
// geoNormal: normal of the face toward the camera. skyLight: lightmap y of the surface
vec3 brCaustics(vec3 col, vec2 worldXZ, float worldY, float camDist, vec3 geoNormal, float skyLight, brEnvironment env, float time) {
  if (!env.overworld || !env.underwater) {
    return col;
  }

  float depth = clamp(BR_CAUSTIC_SURFACE_Y - worldY, 0.4, 14.0);

  // the sun refracts into the water, the pattern lies shifted away from the sun: the ray that lights a point
  // entered the surface closer to the sun by d*tan(refracted angle)
  float sinI = length(env.sunDir.xz);
  float sinR = sinI/BR_WATER_INDEX;
  vec2 toSun = env.sunDir.xz/max(sinI, 0.0001);
  vec2 entry = worldXZ + toSun*(depth*sinR/sqrt(1.0 - sinR*sinR));

  vec2 p = mod(entry, vec2(512.0, 512.0));
  float fine = 1.0 - smoothstep(12.0, 40.0, camDist);        // short ripples would only flicker far away
  float light = brCausticLight(p, time, depth, fine);

  // only sunlight makes the pattern: by day, in the open sky light, fading with the depth as the water absorbs
  float sun = env.day + 0.5*env.twilight;
  float open = smoothstep(0.35, 0.9, skyLight);
  float up = smoothstep(0.1, 0.7, geoNormal.y);
  float reach = exp(-0.16*depth)*(1.0 - smoothstep(30.0, 70.0, camDist));
  float k = clamp(BR_CAUSTICS*sun*open*up*reach*(1.0 - 0.6*env.rainFactor), 0.0, 1.0);

  vec3 tint = vec3(0.92, 1.0, 1.08);
  return col*mix(vec3(1.0, 1.0, 1.0), tint*light*0.75, k);
}

#endif
