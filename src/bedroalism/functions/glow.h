#ifndef BR_GLOW_H
#define BR_GLOW_H

#include "detection.h"

// Scattering of light by the haze around the sun: air and aerosols bend light mostly forward, so the
// sky is brightest right next to the sun and fades quickly with the angle. This is the Henyey-Greenstein
// phase function divided by its value at 0 degrees, which leaves
//   ((1 - g)^3 / (1 + g^2 - 2*g*cosAngle)^1.5)
// = 1.0 straight at the light, falling toward 0.0. g near 1.0 gives a tight aureole, g near 0.5 a wide haze.
float brForwardScatter(float cosAngle, float g) {
  float d = max(1.0 + g*g - 2.0*g*cosAngle, 0.0001);
  float f = (1.0 - g)*(1.0 - g)*(1.0 - g);
  return f/(d*sqrt(d));
}

// light below the horizon is gone
float brGlowVisibility(float lightHeight) {
  return smoothstep(-0.08, 0.12, lightHeight);
}

// additive glow of the sun and the moon on the sky, linear color before exposure and tonemap
// viewDir: normalized, y is up
vec3 brSunMoonGlow(vec3 viewDir, brEnvironment env) {
  // low sun: the light crosses more air, so the haze is stronger and warmer
  float sunLow = 1.0 - clamp(env.sunDir.y, 0.0, 1.0);
  vec3 sunCol = mix(vec3(1.15, 1.10, 1.00), vec3(1.50, 0.55, 0.25), env.twilight);
  float cs = dot(viewDir, env.sunDir);
  float sunScatter = 0.28*brForwardScatter(cs, 0.90) + 0.03*brForwardScatter(cs, 0.60);
  float sunAmount = BR_SUN_GLOW*sunScatter*(1.0 + 0.8*sunLow*sunLow)*brGlowVisibility(env.sunDir.y);

  // moonlight is about a million times weaker, the glow only shows against a dark sky
  float cm = dot(viewDir, env.moonDir);
  float moonScatter = 0.03*brForwardScatter(cm, 0.92) + 0.004*brForwardScatter(cm, 0.60);
  float moonAmount = BR_MOON_GLOW*moonScatter*env.night*brGlowVisibility(env.moonDir.y);

  // overcast sky: the clouds diffuse the light, no distinct glow is left
  float clear = 1.0 - 0.85*env.rainFactor;
  return (sunCol*sunAmount + vec3(0.55, 0.65, 0.90)*moonAmount)*clear;
}

#endif
