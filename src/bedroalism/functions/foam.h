#ifndef BR_FOAM_H
#define BR_FOAM_H

#include "water.h"

// Foam along the shore.
//
// The game gives a water surface no idea where the shore is, but the shore knows about the water. A bank
// is made of terrain faces that stand in the top block of the water: their sky light is one level lower
// than in the open air (the water takes some of it), and they are cut by the water surface, which sits at
// 14/16 of the block at sea level. So the foam is drawn on those faces, at the place where the line of
// sight to the face crosses the water surface: the ray from the camera goes through the surface, the hit
// point is the foam position and its horizontal distance to the bank is the distance to the shore. Foam is
// thick right at the bank and thins into lace over a band that breathes in and out like the swash of a wave.
//
// This works for water at sea level (oceans, rivers, beaches), where the surface height is known. A lake
// high in the mountains has another height and gets no foam. The two heights are the sea levels of the
// normal worlds and the flat worlds.

// the height of the water surface, if the fragment is in the top block of water at one of the sea levels
// returns 0.0 when it is not
float brFoamSurfaceHeight(float y) {
  float seaA = 62.890625;
  float seaB = -60.109375;
  if (y > seaA - 0.89 && y < seaA) {
    return seaA;
  }
  if (y > seaB - 0.89 && y < seaB) {
    return seaB;
  }
  return 0.0;
}

// foam amount 0.0 ~ 1.0 on a terrain face. Only for faces of banks: vertical faces, in the top block of
// the water, with the sky light of one block under water
// absPos: absolute world position of the fragment. viewDir: from the camera to the fragment
// faceNormal: facing the camera. skyLight: lightmap y
float brFoamAmount(sampler2D noiseTex, vec3 absPos, vec3 viewDir, vec3 faceNormal, float skyLight, float t) {
  float surface = brFoamSurfaceHeight(absPos.y);
  // a bank face is vertical, the camera is above the water and looks down at it, the sky light is about
  // one level below the open air (13 of 15)
  float vertical = 1.0 - smoothstep(0.25, 0.55, abs(faceNormal.y));
  float underWater = step(0.85, skyLight)*step(skyLight, 0.927);
  if (surface <= 0.0 || vertical <= 0.0 || underWater <= 0.0 || viewDir.y > -0.03) {
    return 0.0;
  }

  // where the line of sight crosses the water surface, and the distance from there to the bank
  float tau = (surface - absPos.y)/(-viewDir.y);          // blocks of water the ray crosses
  vec2 hit = absPos.xz - viewDir.xz*tau;
  float shoreDist = max(dot(hit - absPos.xz, faceNormal.xz)/max(length(faceNormal.xz), 0.001), 0.0);

  // the swash: the band of foam grows and shrinks, out of step along the shore
  float swash = 0.5 + 0.5*sin(t*0.85 + dot(hit, vec2(0.61, 0.43)) + 1.5*sin(dot(hit, vec2(-0.33, 0.52)) + 0.4*t));
  float band = 0.30 + 0.85*swash;
  float density = 1.0 - smoothstep(0.0, band, shoreDist);

  // lace: the foam breaks into bubbles toward the outer edge. The pattern drifts slowly
  vec4 c = texture2D(noiseTex, hit*0.24 + vec2(0.011, -0.007)*t);
  float lace = c.b*0.55 + c.a*0.45;
  float foam = smoothstep(0.35, 0.75, density*1.35 + (lace - 0.5)*0.9);

  // a thin steady line right at the bank
  foam = max(foam, (1.0 - smoothstep(0.0, 0.10, shoreDist))*0.85);
  return clamp(foam*vertical*BR_WATER_FOAM, 0.0, 1.0);
}

// color of the foam: white, lit by the day, almost dark at night. It is seen through the water, so it is
// brighter than the white it stands for
vec3 brFoamColor(brEnvironment env) {
  return vec3(0.93, 0.97, 1.00)*(2.6*brWaterLightLevel(env) + 0.03);
}

#endif
