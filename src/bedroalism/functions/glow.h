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

// Glare of the sun. Looking toward a bright light, the eye does not see it alone: light scatters inside the
// eye and spreads as a veil over everything near the line of sight, that washes the colors out, lifts the
// blacks and leaves a bloom around the sun. The game gives no screen to blur, so it is made per pixel from
// the angle between the pixel and the sun:
//   - a bloom, tight around the sun
//   - a wide veil, the forward scatter of the eye (the same phase function as the sky aureole, broader)
// Both are stronger when the sun is low (long path through haze, and the eye is looking along the ground),
// warm at sunrise and sunset, and weaker through clouds and rain. Under a roof, in a cave, under water and in
// the other dimensions there is none.
// skyLight: 0.0 ~ 1.0 sky light of the pixel (1.0 for the sky itself)
vec3 brApplyGlare(vec3 col, vec3 viewDir, brEnvironment env, float skyLight) {
  float cs = dot(viewDir, env.sunDir);
  if (BR_SUN_GLARE <= 0.0 || cs <= 0.0) {
    return col;
  }
  float visible = brGlowVisibility(env.sunDir.y)*smoothstep(0.30, 0.75, skyLight)*float(env.overworld);
  visible *= (1.0 - 0.85*env.rainFactor)*(1.0 - float(env.underwater));
  if (visible <= 0.0) {
    return col;
  }

  float sunLow = 1.0 - clamp(env.sunDir.y, 0.0, 1.0);
  float lowBoost = 1.0 + 1.5*sunLow*sunLow;
  vec3 sunCol = mix(vec3(1.15, 1.10, 1.00), vec3(1.50, 0.62, 0.30), env.twilight);

  float bloom = pow(cs, 120.0);                    // about 9 degrees wide
  float veil = 0.5*brForwardScatter(cs, 0.80);     // 0.5 at the sun, 0.05 at 25 degrees
  float g = BR_SUN_GLARE*visible*lowBoost*(0.80*bloom + 0.80*veil);

  // the veil lifts everything toward the color of the sun and takes contrast away
  col = col*(1.0 - clamp(0.45*g, 0.0, 0.6)) + sunCol*g;
  return col;
}

// the sun and moon discs, for reflections and for looking up through the water surface
// dir: normalized direction of the ray. radiance scales the brightness of each disc
vec3 brSunMoonDisc(vec3 dir, brEnvironment env, float sunRadiance, float moonRadiance) {
  float outer = 0.9983; // cos(0.058), the discs of the SunMoon material span about 0.058 rad at size 1.0
  float inner = 0.9990; // cos(0.045), soft edge
  vec3 sunCol = mix(vec3(1.15, 1.10, 1.00), vec3(1.50, 0.55, 0.25), env.twilight);
  float sun = smoothstep(outer, inner, dot(dir, env.sunDir))*brGlowVisibility(env.sunDir.y);
  float moon = smoothstep(outer, inner, dot(dir, env.moonDir))*brGlowVisibility(env.moonDir.y)*env.night;
  return sunCol*(sun*sunRadiance) + vec3(0.62, 0.70, 0.90)*(moon*moonRadiance);
}

#endif
