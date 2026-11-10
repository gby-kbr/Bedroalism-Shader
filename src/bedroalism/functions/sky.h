#ifndef BR_SKY_H
#define BR_SKY_H

#include "detection.h"

struct brSkyColor {
  vec3 zenith;
  vec3 horizon;      // the horizon with an average amount of dawn, for the ambient light of the clouds
  vec3 baseHorizon;  // day, night and rain only
  vec3 dawnHorizon;
  float dawn;        // [0.0, 1.0] how much dawn there is, it only reaches the horizon near the sun
  vec3 sunDir;
};

// detection reports full rain under water and in the Nether, so it is masked here
float brRain(brEnvironment env) {
  return (env.underwater || env.nether || env.end) ? 0.0 : env.rainFactor;
}

float brDawnAmount(brEnvironment env) {
  float d = 1.0 - env.dayFactor*env.dayFactor;
  return d*d*d;
}

// day, night, dawn and rain, mixed the way Newb X does
brSkyColor brSkyColors(brEnvironment env) {
  brSkyColor s;
  float f = 1.0 + 2.0*(1.0 - max(-env.dayFactor, 0.0));
  float nightFactor = 1.0 - smoothstep(-0.12, 0.12, env.dayFactor); // a soft change, a hard step jumps at sunset
  s.zenith = mix(BR_DAY_ZENITH_COL, BR_NIGHT_ZENITH_COL*f, nightFactor);
  s.baseHorizon = mix(BR_DAY_HORIZON_COL, BR_NIGHT_HORIZON_COL*f, nightFactor);

  float dawnFactor = 1.0 - env.dayFactor*env.dayFactor;
  dawnFactor *= dawnFactor*dawnFactor;
  dawnFactor *= mix(1.0, dawnFactor*dawnFactor, nightFactor);
  dawnFactor *= 1.0 - 0.4*smoothstep(0.0, -0.25, env.dayFactor); // weaker once the sun is gone
  dawnFactor *= BR_DAWN_AMOUNT;
  s.zenith = mix(s.zenith, BR_DAWN_ZENITH_COL, dawnFactor);

  float zh = dot(s.zenith, vec3_splat(0.33));
  float hh = dot(s.baseHorizon, vec3_splat(0.33));
  float rainMix = brRain(env)*BR_SKY_RAIN_MIX;
  s.zenith = mix(s.zenith, BR_RAIN_ZENITH_COL*zh, rainMix);
  s.baseHorizon = mix(s.baseHorizon, BR_RAIN_HORIZON_COL*mix(hh, zh, 0.7), rainMix); // not brighter than the zenith

  // orange while the sun is at the horizon, a faded tone after it has gone
  vec3 faded = mix(BR_DAWN_HORIZON_COL, vec3_splat(dot(BR_DAWN_HORIZON_COL, vec3_splat(0.33))), 0.45);
  s.dawnHorizon = mix(faded, BR_DAWN_HORIZON_COL, smoothstep(-0.06, 0.10, env.dayFactor));
  s.dawn = dawnFactor*(1.0 - rainMix);
  s.sunDir = env.sunDir;
  s.horizon = mix(s.baseHorizon, s.dawnHorizon, s.dawn*0.4);
  return s;
}

// viewDir: normalized, y up
vec3 brRenderSky(brSkyColor s, vec3 viewDir) {
  float vh = 1.0 - viewDir.y*viewDir.y;
  float gradient = vh*vh*vh; // a long soft fade from the horizon to the zenith
  float nearSun = smoothstep(-0.2, 0.9, dot(viewDir, s.sunDir)); // dawn color only around the sun
  vec3 horizon = mix(s.baseHorizon, s.dawnHorizon, s.dawn*nearSun);
  vec3 sky = mix(s.zenith, horizon, gradient);
  return sky*(0.5 + 0.3*gradient);
}


// light below the horizon is gone
float brGlowVisibility(float lightHeight) {
  return smoothstep(-0.08, 0.12, lightHeight);
}

// angular radius of the visible disc inside the sun and moon quad, for size 1.0
#define BR_SUNMOON_DISC_RADIUS 0.0583

// the sun and moon discs, for reflections and for looking up through the water surface
// dir: normalized direction of the ray. radiance scales the brightness of each disc
vec3 brSunMoonDisc(vec3 dir, brEnvironment env, float sunRadiance, float moonRadiance) {
  // the visible disc scales with the size of the quad, the soft edge is the same share of it
  float sunOuter = cos(BR_SUNMOON_DISC_RADIUS*BR_SUN_SIZE);
  float sunInner = cos(BR_SUNMOON_DISC_RADIUS*BR_SUN_SIZE*0.767);
  float moonOuter = cos(BR_SUNMOON_DISC_RADIUS*BR_MOON_SIZE);
  float moonInner = cos(BR_SUNMOON_DISC_RADIUS*BR_MOON_SIZE*0.767);
  vec3 sunCol = mix(vec3(1.15, 1.10, 1.00), vec3(1.50, 0.55, 0.25), env.twilight);
  float sun = smoothstep(sunOuter, sunInner, dot(dir, env.sunDir))*brGlowVisibility(env.sunDir.y);
  float moon = smoothstep(moonOuter, moonInner, dot(dir, env.moonDir))*brGlowVisibility(env.moonDir.y)*env.night;
  return sunCol*(sun*sunRadiance) + vec3(0.62, 0.70, 0.90)*(moon*moonRadiance);
}

#endif
