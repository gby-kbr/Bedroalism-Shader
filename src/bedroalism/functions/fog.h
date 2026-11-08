#ifndef BR_FOG_H
#define BR_FOG_H

#include "detection.h"
#include "sky.h"
#include "glow.h"

#define BR_SEA_LEVEL 63.0

// ---------------------------------------------------------------------------------------------
// Fog
//
// Air scatters the light on its way to the eye: what is left of a surface after a path of length d is
// T = exp(-optical depth), the rest is replaced by the color of the air itself (the sky color at the
// horizon, brighter toward the sun). The density of the air falls off exponentially with height,
//   density(y) = d0*exp(-b*(y - sea level)),
// so fog lies thickest in the valleys and over water and thins out on the hills. Along a straight ray the
// integral of that density has a closed form, no marching is needed.
// Rain fills the air with moisture (denser and deeper), the cool morning air holds mist near the ground.
// ---------------------------------------------------------------------------------------------

// density at sea level, per block
float brFogDensity(brEnvironment env) {
  float d = BR_FOG_DENSITY;
  d *= 1.0 + BR_FOG_MORNING*env.sunrise;
  d *= 1.0 - 0.3*env.night;
  d *= 1.0 + BR_FOG_RAIN*env.rainFactor;
  return d;
}

// 1/blocks: how fast the density falls off with height. The morning mist is a shallow layer, the rain fog a deep one
float brFogFalloff(brEnvironment env) {
  return clamp(BR_FOG_FALLOFF*(1.0 + 1.2*env.sunrise)*(1.0 - 0.4*env.rainFactor), 0.004, 0.08);
}

// optical depth of the air between the camera and a point
// camY: world height of the camera. dirY: vertical part of the normalized direction. dist: blocks
float brFogOpticalDepth(float camY, float dirY, float dist, brEnvironment env) {
  float b = brFogFalloff(env);
  float x = b*dist*dirY;
  float k = abs(x) < 0.001 ? 1.0 - 0.5*x : (1.0 - exp(-x))/x;
  float d0 = brFogDensity(env)*exp(-b*max(camY - BR_SEA_LEVEL, -15.0));
  return min(d0*dist*k, 8.0);
}

// mist: a soft veil that is not there up close and grows with the square of the distance, so the scene
// stays clear nearby and turns hazy far away. At most 35% of the color is replaced. Darker, damper air
// holds more of it: night, dusk and dawn, rain
// relDist: distance divided by the render distance
float brMistAmount(float relDist, brEnvironment env) {
  float density = BR_MIST*6.0*(1.0 + 1.5*env.night + 1.0*env.twilight + 2.0*env.rainFactor);
  return 0.35*(1.0 - exp(-relDist*relDist*density));
}

// the color of the air in the direction dir
vec3 brFogColor(vec3 dir, brEnvironment env) {
  brSkyColor sky = brSkyColors(env);
  return mix(sky.horizon, sky.zenith, 0.12) + min(brSunMoonGlow(dir, env), vec3(3.0, 3.0, 3.0))*BR_FOG_GLOW;
}

// col: color of the surface. relPos: from the camera to the surface. camY: world height of the camera
// skyLight: sky light of the surface, 0.0 ~ 1.0, caves have no fog of the open sky
// renderDist: render distance in blocks
// vanillaFogColor, vanillaFog: the fog of the game, it still hides the edge of the render distance
vec3 brApplyFog(vec3 col, vec3 relPos, float camY, float skyLight, float renderDist, brEnvironment env, vec3 vanillaFogColor, float vanillaFog) {
  if (!env.overworld) {
    return mix(col, vanillaFogColor, vanillaFog);
  }
  float dist = length(relPos);
  vec3 dir = relPos/max(dist, 0.0001);
  float open = smoothstep(0.15, 0.75, skyLight);
  float optical = brFogOpticalDepth(camY, dir.y, dist, env)*(0.3 + 0.7*open);
  float fog = 1.0 - exp(-optical);
  float mist = brMistAmount(dist/max(renderDist, 1.0), env)*open;
  float amount = max(1.0 - (1.0 - fog)*(1.0 - mist), vanillaFog);
  vec3 fogCol = mix(vanillaFogColor, brFogColor(dir, env), open);
  return mix(col, fogCol, amount);
}

#endif
