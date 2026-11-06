#ifndef BR_TORCH_H
#define BR_TORCH_H

#include "underwater.h"

// Light of torches and every other block that gives light.
//
// The game stores the block light of a face as a level from 0 to 15 that drops by one for every block of
// distance from the source, so the distance to the nearest source is (15 - level) blocks. A small lamp
// lights a surface with 1/(1 + (d/d0)^2): bright next to it, a quarter of that at twice the distance.
// The light is added on top of the ambient light of the sky and divided by (1 + luminance of the
// ambient): in the sun a torch is lost, in the dark it is the only light.
//
// Under water the light of a torch has crossed water: red is absorbed first, so near the torch it
// stays warm and further away it turns yellow and then green. The absorption is half of the haze's, a
// torch lights its surroundings from close by.

// how much light reaches a face with this block light, 1.0 right next to the source
// blockLight: 0.0 ~ 1.0 (level / 15)
float brTorchFalloff(float blockLight) {
  float d = (1.0 - blockLight)*15.0; // blocks to the source
  float r = d/BR_TORCH_RADIUS;
  return smoothstep(0.0, 0.12, blockLight)/(1.0 + r*r); // the smoothstep ends the light at level 0 smoothly
}

// color of the flame light
vec3 brTorchColor(brEnvironment env, float blockLight) {
  if (env.underwater) {
    float d = (1.0 - blockLight)*15.0;
    return BR_UNDERWATER_TORCH_COLOR*exp(-brUnderwaterAbsorption()*(0.5*d)); // half the haze absorption, the light is local
  }
  return BR_TORCH_COLOR;
}

// flames flicker: slow and fast waves of different frequencies never repeat exactly. pos: world position,
// the phase differs along the ground so torches next to each other do not pulse together
float brTorchFlicker(vec2 pos, float t) {
  float phase = dot(pos, vec2(0.37, 0.29));
  float f = 0.5*sin(t*6.1 + phase) + 0.3*sin(t*10.7 + 1.3*phase) + 0.2*sin(t*17.3 + 2.1*phase);
  return 1.0 - BR_TORCH_FLICKER*(0.5 + 0.5*f);
}

// light to add to the ambient light of a face
// ambient: ambient light of the face (sky light, no torch). pos: absolute xz of the fragment
vec3 brTorchLight(float blockLight, vec3 ambient, brEnvironment env, vec2 pos, float t) {
  float lum = dot(ambient, vec3(0.299, 0.587, 0.114));
  float amount = BR_TORCH_INTENSITY*brTorchFalloff(blockLight)*brTorchFlicker(pos, t);
  return brTorchColor(env, blockLight)*(amount/(1.0 + lum));
}

#endif
