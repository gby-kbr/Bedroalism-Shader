#ifndef BR_RAIN_H
#define BR_RAIN_H

#include "utils.h"
#include "water.h"

// ---------------------------------------------------------------------------------------------
// Wet ground
//
// While it rains the open-air ground gets darker (wet stone and soil absorb more light) and a thin film of water
// collects on flat surfaces. The film covers more of the ground the harder it rains: the coverage is a
// threshold on tileable noise, so it grows and dries out at its edges. Water on dark ground is a mirror at
// grazing angles (Fresnel), so the puddles reflect the overcast sky and the drops that hit them leave rings.
// ---------------------------------------------------------------------------------------------

// 0.0 dry ~ 1.0 water. p: world position (x, z) in blocks. cover: 0.0 nothing is wet ~ 1.0 the whole ground
// is covered, only the thickness of the film still varies a little
float brPuddleMask(sampler2D noiseTex, vec2 p, float cover) {
  vec4 a = texture2DLod(noiseTex, p*(1.0/52.0), 0.0);
  vec4 b = texture2DLod(noiseTex, p*(1.0/13.0) + vec2(0.37, 0.61), 0.0);
  float n = 0.50*a.r + 0.25*a.g + 0.15*b.r + 0.10*b.g;
  float t = mix(0.82, -0.08, clamp(cover, 0.0, 1.0));
  return smoothstep(t, t + 0.06, n)*mix(0.80, 1.0, n);
}

// slope (x, z) of the water surface made by the rings of the drops. Every cell of the grid gets a drop
// now and then; its ring grows from the hit point and fades while it widens
vec2 brRippleSlope(vec2 p, float t, float rain) {
  vec2 slope = vec2(0.0, 0.0);
  for (int i = 0; i < 2; i++) {
    float fi = float(i);
    vec2 q = p*(0.85 + 0.6*fi) + vec2(17.3, 5.1)*fi;
    vec2 cell = floor(q);
    vec2 h = brHash22(cell);
    float h3 = brHash12(cell + vec2(7.7, 3.1));
    float h4 = brHash12(cell + vec2(31.7, 11.3));
    vec2 hit = (h - 0.5)*0.4;                               // inside the cell, so the ring never crosses its border
    float life = fract(t*(0.7 + 0.5*h3) + h3*7.0);          // 0.0 the drop just hit ~ 1.0 the ring is gone
    float alive = step(h4, 0.15 + 0.7*rain);                // heavier rain: more cells with drops
    vec2 dv = fract(q) - 0.5 - hit;
    float d = length(dv);
    float x = (d - life*0.27)*24.0;
    float amp = exp(-x*x)*(1.0 - life)*(1.0 - life)*alive;
    slope += dv/max(d, 0.001)*(-2.0*x*amp*0.45);
  }
  return slope;
}

// the color of a surface under the rain
// col: the lit color of the dry surface. albedo: its color before lighting
// geoNormal: normal of the face facing the camera. lightmap: block light, sky light
// worldXZ: world position (x, z) in blocks. renderDist: render distance in blocks
vec3 brWetSurface(sampler2D noiseTex, vec3 col, vec3 albedo, vec3 viewDir, vec3 geoNormal, vec2 lightmap, vec2 worldXZ, float camDist, float renderDist, brEnvironment env, float time, vec2 fragCoord) {
  float rain = env.rainFactor;
  if (!env.overworld || env.underwater || rain <= 0.0) {
    return col;
  }

  float open = smoothstep(0.55, 0.90, lightmap.y);          // under a roof or a tree nothing gets wet
  float top = smoothstep(0.85, 0.97, geoNormal.y);          // only flat, upward faces hold water
  float luma = dot(albedo, vec3(0.333, 0.333, 0.333));
  float dryMat = smoothstep(0.62, 0.80, luma);              // snow and other white surfaces stay as they are
  float wet = rain*open*(1.0 - dryMat);

  col *= 1.0 - BR_WET_DARKEN*wet*mix(0.6, 1.0, top);

  float cover = smoothstep(0.0, 0.8, rain)*BR_PUDDLES;
  float puddle = brPuddleMask(noiseTex, worldXZ, cover)*top*open*(1.0 - dryMat);
  if (puddle <= 0.001) {
    return col;
  }
  col *= 1.0 - 0.25*puddle;

  // the reflection fades out before the far end of the render distance, the wet color stays
  float reach = 1.0 - smoothstep(0.45*renderDist, 0.75*renderDist, camDist);
  float rippleFade = BR_PUDDLE_RIPPLES*(1.0 - smoothstep(14.0, 30.0, camDist));
  vec2 rip = brRippleSlope(worldXZ, time, rain)*rippleFade;
  vec3 nrm = normalize(vec3(rip.x, 1.0, rip.y));

  float cosV = clamp(-dot(viewDir, nrm), 0.0, 1.0);
  float fresnel = brFresnel(cosV, 0.04);
  vec3 refl = viewDir + 2.0*cosV*nrm;
  refl = normalize(vec3(refl.x, max(refl.y, 0.02), refl.z));
  vec3 sky = brSkyRadiance(noiseTex, refl, env, brSkyColors(env), vec2(0.0, 0.0), time, fragCoord);
  sky *= mix(0.05, 1.0, smoothstep(0.35, 0.9, lightmap.y));

  return mix(col, sky, clamp(fresnel*puddle*reach, 0.0, 1.0));
}

#endif
