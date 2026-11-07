#ifndef BR_WATER_H
#define BR_WATER_H

#include "sky.h"
#include "glow.h"
#include "clouds.h"

// ---------------------------------------------------------------------------------------------
// Waves
//
// The surface is a sum of 6 travelling waves. Every wave has a whole-number wave vector over a 512 block
// tile, so the surface repeats seamlessly every 512 blocks and the position can be wrapped to keep the
// float precision high far from the origin. The speed follows the deep water dispersion, w = sqrt(g*k),
// so long waves travel faster than short ones. Each wave has the shape A*exp(sin(phase) - 1): narrow
// crests and wide troughs. The slope of every wave is computed from the derivative of its own formula,
// the surface is not differentiated numerically.
// ---------------------------------------------------------------------------------------------

// one wave: returns (height, d height / dx, d height / dz)
// p: position in blocks, K: whole-number wave vector, slope: steepness of the wave, about 0.03 ~ 0.07
vec3 brWaveTerm(vec2 p, float t, vec2 K, float slope) {
  vec2 k = K*(2.0*BR_PI/512.0);
  float kl = length(k);
  float phase = dot(k, p) - sqrt(9.8*kl)*BR_WATER_WAVE_SPEED*t;
  float amp = slope*BR_WATER_WAVE_HEIGHT/kl;
  float crest = amp*exp(sin(phase) - 1.0);
  float slopeAlong = crest*cos(phase);
  return vec3(crest - 0.466*amp, k.x*slopeAlong, k.y*slopeAlong); // 0.466 is the mean of exp(sin(x) - 1)
}

// height and slope of the whole surface
vec3 brWaterSurface(vec2 p, float t) {
  vec3 s = brWaveTerm(p, t, vec2(72.0, 26.0), 0.065);
  s += brWaveTerm(p, t, vec2(-41.0, 95.0), 0.058);
  s += brWaveTerm(p, t, vec2(150.0, -60.0), 0.052);
  s += brWaveTerm(p, t, vec2(-120.0, -250.0), 0.046);
  s += brWaveTerm(p, t, vec2(330.0, 190.0), 0.040);
  s += brWaveTerm(p, t, vec2(-300.0, 520.0), 0.032);
  return s;
}

// position that keeps the precision: the camera position wrapped to the 512 block tile plus the position
// relative to the camera. relPos: world position of the vertex or fragment minus the camera position
vec2 brWaterCoord(vec2 relXZ, vec2 cameraXZ) {
  return mod(cameraXZ, vec2(512.0, 512.0)) + relXZ;
}

// how far the surface mesh moves at this position (blocks, up is positive), 0.0 outside the range
float brWaterDisplacement(vec2 p, float t, float camDist) {
  float fade = 1.0 - smoothstep(0.6*BR_WATER_WAVE_RANGE, BR_WATER_WAVE_RANGE, camDist);
  return clamp(brWaterSurface(p, t).x, -0.09, 0.09)*fade;
}

// normal of the surface, pointing up. far away the slopes fade out so the surface does not sparkle
vec3 brWaterNormal(vec2 p, float t, float camDist) {
  vec3 s = brWaterSurface(p, t);
  float fade = 1.0 - 0.85*smoothstep(40.0, 200.0, camDist);
  return normalize(vec3(-s.y*fade, 1.0, -s.z*fade));
}

// ---------------------------------------------------------------------------------------------
// Light
// ---------------------------------------------------------------------------------------------

// Schlick approximation of the Fresnel reflectance, f0 = 0.02 for water in air
float brFresnel(float cosAngle, float f0) {
  float a = 1.0 - clamp(cosAngle, 0.0, 1.0);
  float a2 = a*a;
  return f0 + (1.0 - f0)*a2*a2*a;
}

// how much sun and sky light is around: bright by day, little at night
float brWaterLightLevel(brEnvironment env) {
  float level = env.day + 0.45*env.twilight + 0.035*env.night;
  return level*(1.0 - 0.5*env.rainFactor);
}

// color of the light that the water scatters back to the eye
// skyLight: 0.0 ~ 1.0, the sky light of the position (lightmap y), water in a cave stays dark
vec3 brWaterScatterColor(brEnvironment env, float skyLight) {
  return BR_WATER_COLOR*(brWaterLightLevel(env)*(0.12 + 0.88*skyLight*skyLight));
}

// everything the sky shows in one direction: gradient, glow, sun and moon discs behind the clouds, clouds
// discRadiance: x sun, y moon, 0.0 hides the discs
vec3 brSkyRadiance(sampler2D noiseTex, vec3 dir, brEnvironment env, brSkyColor sky, vec2 discRadiance, float time, vec2 fragCoord) {
  vec3 col = brSkyGradient(dir, sky) + brSunMoonGlow(dir, env);
  float cloudAlpha = 0.0;
  vec3 cloudCol = vec3(0.0, 0.0, 0.0);
  if (BR_WATER_CLOUD_REFLECTION > 0.0 && dir.y > 0.03) {
    vec4 clouds = brClouds(noiseTex, dir, env, sky, time, fragCoord);
    cloudCol = clouds.rgb;
    cloudAlpha = clouds.a;
  }
  col += brSunMoonDisc(dir, env, discRadiance.x, discRadiance.y)*(1.0 - cloudAlpha);
  return mix(col, cloudCol, cloudAlpha);
}

// ---------------------------------------------------------------------------------------------
// Surface seen from above
// ---------------------------------------------------------------------------------------------

// color and opacity of a water fragment seen from above, to be blended over what is behind it
// viewDir: from the camera to the fragment, normalized. nrm: normal facing the camera
// skyLight, blockLight: lightmap of the fragment. biomeTint: vanilla water color, only tints a little
vec4 brWaterAbove(sampler2D noiseTex, vec3 viewDir, vec3 nrm, vec2 lightmap, vec3 biomeTint, brEnvironment env, brSkyColor sky, float time, vec2 fragCoord) {
  float cosV = clamp(-dot(viewDir, nrm), 0.0, 1.0);
  float fresnel = brFresnel(cosV, 0.02);

  // reflection: a reflected ray that would go below the horizon is held at the horizon
  vec3 refl = viewDir + 2.0*cosV*nrm;
  refl = normalize(vec3(refl.x, max(refl.y, 0.0), refl.z));
  vec3 skyRefl = brSkyRadiance(noiseTex, refl, env, sky, vec2(40.0, 6.0)*BR_WATER_SUN_GLITTER, time, fragCoord);
  skyRefl *= mix(0.05, 1.0, smoothstep(0.35, 0.9, lightmap.y)); // nothing of the sky in the shade

  // body: the longer the path through the water, the more it hides what is behind
  float hide = -log(1.0 - clamp(BR_WATER_OPACITY, 0.01, 0.95));
  float transmit = exp(-hide/max(cosV, 0.12));
  float luma = dot(biomeTint, vec3(0.333, 0.333, 0.333));
  vec3 tint = clamp(biomeTint/max(luma, 0.001), vec3(0.5, 0.5, 0.5), vec3(1.6, 1.6, 1.6));
  vec3 body = brWaterScatterColor(env, lightmap.y)*mix(vec3(1.0, 1.0, 1.0), tint, 0.35);

  // what reaches the eye = reflection*F + body*(1 - F)*(1 - transmit) + behind*(1 - F)*transmit
  float alpha = 1.0 - (1.0 - fresnel)*transmit;
  vec3 col = (skyRefl*fresnel + body*((1.0 - fresnel)*(1.0 - transmit)))/max(alpha, 0.001);
  return vec4(col, alpha);
}

#endif
