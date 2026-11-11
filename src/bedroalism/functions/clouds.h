#ifndef BR_CLOUDS_H
#define BR_CLOUDS_H

#include "sky.h"

// ---------------------------------------------------------------------------------------------
// Clouds: one flat cumulus layer drawn on the sky, read from textures/environment/cloud_noise.png (made by
// script/generate_cloud_noise.py; R coverage, G puffs, B detail). Per sky pixel that is 3 texture reads and
// no loop.
//   cumulus: coverage + puffs + detail build the density. A second read, moved toward the sun, tells how
//            much cloud the sunlight crosses to reach the point: that is the shading and the bright rim.
// ---------------------------------------------------------------------------------------------

// returns the color of the clouds (rgb) and how much they cover the sky behind (a)
vec4 brClouds(sampler2D tex, vec3 dir, brEnvironment env, brSkyColor sky, float time) {
  // no early return below the horizon: a texture read in a branch that changes inside one screen quad
  // gives a line there. The clouds fade out to nothing at the horizon anyway
  float up = max(dir.y, 0.0);
  float rain = brRain(env);
  float lift = 1.0/(up*0.9 + 0.2);       // the layer is far away at the horizon
  vec2 plane = dir.xz*lift;
  vec2 wind = vec2(1.0, 0.45)*BR_CLOUD_WIND*time*0.024; // the direction of the mist and the plants
  float cover = clamp(BR_CLOUD_COVER + 0.35*rain, 0.0, 1.0);

  // the wind twists the clouds: one read of the big blobs bends the coordinates
  vec2 twist = texture2D(tex, plane*0.045 + wind*0.4).rg - 0.5;

  // cumulus
  vec2 uv = plane*0.2 + wind + twist*0.04;
  vec4 n = texture2D(tex, uv);
  float soft = n.r*0.35 + n.g*0.65;
  float d = soft + (n.b - 0.5)*BR_CLOUD_ROUGHNESS;
  float edge = 1.0 - cover;
  float density = smoothstep(edge, edge + BR_CLOUD_SOFTNESS + 0.12*rain, d);

  // how much cloud lies between the point and the sun: the side facing the sun is bright
  vec2 toSun = normalize(env.sunDir.xz + vec2(1e-4, 0.0))*0.012;
  vec4 m = texture2D(tex, uv + toSun*(0.4 + 0.6*env.sunDir.y));
  float dm = m.r*0.35 + m.g*0.65;
  float body = smoothstep(edge + 0.05, edge + 0.4, soft);              // the core is deeper than the rim
  float lit = clamp(0.6 + (soft - dm)*6.0 - body*0.45, 0.0, 1.0);
  float rim = 1.0 - smoothstep(edge, edge + 0.3, d);                 // thin edges let the light through

  // light
  vec3 sunCol = mix(vec3(1.0, 0.95, 0.86), vec3(1.0, 0.50, 0.26), env.twilight)*env.day*(1.0 - rain*0.7);
  vec3 moonCol = vec3(0.30, 0.38, 0.55)*env.night;
  vec3 ambient = mix(sky.horizon, sky.zenith, 0.5)*(0.55 + 0.25*env.day + 0.2*env.twilight) + moonCol*0.2;
  ambient = mix(ambient, vec3_splat(dot(ambient, vec3(0.33, 0.33, 0.33)))*0.75, rain*0.8);
  // at sunrise and sunset the clouds that face the sun glow, the ones that look away stay dim
  vec2 across = normalize(dir.xz + vec2(1e-4, 0.0));
  float facing = pow(max(dot(across, normalize(env.sunDir.xz + vec2(1e-4, 0.0))), 0.0), 2.0);
  vec3 glow = vec3(1.0, 0.62, 0.38)*env.twilight*(0.25 + 0.75*facing)*(1.0 - rain);
  vec3 col = ambient*(0.8 - 0.3*body) + (sunCol + moonCol)*lit*(0.55 + 0.3*rim) + glow*(0.5 + 0.5*rim)*(1.0 - 0.3*body);
  col += sunCol*pow(max(dot(dir, env.sunDir), 0.0), 6.0)*rim*density*0.6; // silver lining

  // clouds fade into the haze at the horizon
  float fade = smoothstep(0.0, 0.22, up);
  col = mix(sky.horizon, col, fade);
  return vec4(col, density*fade);
}

#endif
