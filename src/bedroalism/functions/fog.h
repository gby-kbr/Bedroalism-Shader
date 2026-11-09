#ifndef BR_FOG_H
#define BR_FOG_H

#include "sky.h"
#include "underwater.h"

#define BR_SEA_LEVEL 63.0

// Light that survives a path of length d through the air is exp(-optical depth). The air thins out
// with height, density(y) = d0*exp(-b*(y - sea level)), and the integral along a straight ray has a
// closed form, so there is no ray marching.
float brFogOpticalDepth(float camY, float dirY, float dist, brEnvironment env) {
  float rain = brRain(env);
  float b = BR_FOG_FALLOFF*(1.0 - 0.4*rain);
  float density = BR_FOG_DENSITY*(1.0 + BR_FOG_RAIN*rain)*(1.0 + BR_FOG_DAWN*brDawnAmount(env));
  float x = b*dist*dirY;
  float k = abs(x) < 0.001 ? 1.0 - 0.5*x : (1.0 - exp(-x))/x;
  float d0 = density*exp(-b*max(camY - BR_SEA_LEVEL, -15.0));
  return min(d0*dist*k, 8.0);
}

// patches of mist drifting with the wind, [0.0, 1.0]. p: world xz in blocks
float brWindMist(vec2 p, float t) {
  vec2 q = p + vec2(1.0, 0.45)*4.0*t;
  float v = sin(0.045*q.x + 0.03*q.y)*sin(0.03*q.y - 0.02*q.x + 0.25*t);
  v += sin(0.021*q.x - 0.018*q.y + 1.7);
  return 0.25*v*v;
}

// value noise for the mist patches
float brHash(vec2 p) {
  p = fract(p*vec2(0.1031, 0.1030));
  p += dot(p, p.yx + 33.33);
  return fract((p.x + p.y)*p.x);
}

float brNoise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  f = f*f*(3.0 - 2.0*f);
  return mix(mix(brHash(i), brHash(i + vec2(1.0, 0.0)), f.x),
             mix(brHash(i + vec2(0.0, 1.0)), brHash(i + vec2(1.0, 1.0)), f.x), f.y);
}

// thickness of the mist patches over the ground, around 1.0. p: world xz in blocks
float brMistPatch(vec2 p, float t) {
  vec2 q = (p + vec2(1.0, 0.45)*2.5*t)*0.012;
  float n = 0.55*brNoise(q) + 0.3*brNoise(q*2.1 + 7.0) + 0.15*brNoise(q*4.3 + 3.0);
  return mix(0.0, 2.6, smoothstep(0.35, 0.7, n));
}

// A low layer of mist that clings to the water and the ground. Its density falls quickly with height,
// so like the fog it has a closed form. The patches are sampled along the ray, not only at its end.
float brGroundMist(vec2 camXZ, vec2 relXZ, float camY, float dirY, float dist, float t, brEnvironment env) {
  float b = BR_GROUND_MIST_FALLOFF;
  float strength = BR_GROUND_MIST*(0.6 + 1.4*brDawnAmount(env) + brRain(env));
  float x = b*dist*dirY;
  float k = abs(x) < 0.001 ? 1.0 - 0.5*x : (1.0 - exp(-x))/x;
  float d0 = strength*exp(-b*max(camY - BR_SEA_LEVEL, -10.0));
  float thickness = 0.5*(brMistPatch(camXZ + relXZ*0.45, t) + brMistPatch(camXZ + relXZ*0.9, t));
  return min(d0*dist*k*thickness, 8.0);
}

// the air is the color of the sky in that direction, so the fog meets the sky
vec3 brFogColor(vec3 dir, brEnvironment env) {
  return brRenderSky(brSkyColors(env), dir);
}

// relPos: camera to surface. worldXZ: absolute xz of the surface. skyLight: 0.0 ~ 1.0, caves get no sky fog
// vanillaFogColor, vanillaFog: the game's fog, it still hides the edge of the render distance
vec3 brApplyFog(vec3 col, vec3 relPos, vec2 worldXZ, float camY, float skyLight, float renderDist, float t, brEnvironment env, vec3 vanillaFogColor, float vanillaFog) {
  if (env.end || env.nether) {
    return mix(col, vanillaFogColor, vanillaFog);
  }
  float dist = length(relPos);
  vec3 dir = relPos/max(dist, 0.0001);
  if (env.underwater) {
    // clear water haze instead of the thick vanilla fog
    return brUnderwaterFog(col, dist, env, skyLight);
  }
  float open = smoothstep(0.15, 0.75, skyLight);
  float fog = 1.0 - exp(-brFogOpticalDepth(camY, dir.y, dist, env)*(0.3 + 0.7*open));
  float mist = brRain(env)*open*brWindMist(worldXZ, t)*BR_RAIN_MIST*smoothstep(0.03, 0.5, dist/max(renderDist, 1.0));
  float amount = max(1.0 - (1.0 - fog)*(1.0 - mist), vanillaFog);
  vec3 fogCol = mix(vanillaFogColor, brFogColor(dir, env), open);
  col = mix(col, fogCol, amount);

  // ground mist: whiter than the sky and brighter toward the sun
  float ground = 1.0 - exp(-brGroundMist(worldXZ - relPos.xz, relPos.xz, camY, dir.y, dist, t, env)*open);
  float toSun = pow(max(dot(dir, env.sunDir), 0.0), 6.0)*env.day;
  vec3 mistCol = fogCol*(1.0 + 0.5*env.day) + vec3(0.55, 0.5, 0.42)*toSun;
  return mix(col, mistCol, ground);
}

#endif
