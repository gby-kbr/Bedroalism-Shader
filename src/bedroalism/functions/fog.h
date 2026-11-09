#ifndef BR_FOG_H
#define BR_FOG_H

#include "sky.h"

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
  float open = env.underwater ? 1.0 : smoothstep(0.15, 0.75, skyLight);

  float amount;
  if (env.underwater) {
    amount = 1.0 - exp(-dist*BR_UNDERWATER_DENSITY);
  } else {
    float fog = 1.0 - exp(-brFogOpticalDepth(camY, dir.y, dist, env)*(0.3 + 0.7*open));
    float mist = brRain(env)*open*brWindMist(worldXZ, t)*BR_RAIN_MIST*smoothstep(0.03, 0.5, dist/max(renderDist, 1.0));
    amount = 1.0 - (1.0 - fog)*(1.0 - mist);
  }
  amount = max(amount, vanillaFog);
  vec3 fogCol = mix(vanillaFogColor, brFogColor(dir, env), open);
  return mix(col, fogCol, amount);
}

#endif
