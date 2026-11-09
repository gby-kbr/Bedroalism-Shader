#ifndef BR_SKY_H
#define BR_SKY_H

#include "detection.h"

struct brSkyColor {
  vec3 zenith;
  vec3 horizon;
};

// detection reports full rain under water and in the Nether, so it is masked here
float brRain(brEnvironment env) {
  return (env.underwater || env.nether || env.end) ? 0.0 : env.rainFactor;
}

float brDawnAmount(brEnvironment env) {
  float d = 1.0 - env.dayFactor*env.dayFactor;
  return d*d*d;
}

// day, night, dawn, rain and underwater, mixed the way Newb X does
brSkyColor brSkyColors(brEnvironment env) {
  brSkyColor s;
  float f = 1.0 + 2.0*(1.0 - max(-env.dayFactor, 0.0));
  float nightFactor = step(env.dayFactor, 0.0);
  s.zenith = mix(BR_DAY_ZENITH_COL, BR_NIGHT_ZENITH_COL*f, nightFactor);
  s.horizon = mix(BR_DAY_HORIZON_COL, BR_NIGHT_HORIZON_COL*f, nightFactor);

  float dawnFactor = 1.0 - env.dayFactor*env.dayFactor;
  dawnFactor *= dawnFactor*dawnFactor;
  dawnFactor *= mix(1.0, dawnFactor*dawnFactor, nightFactor);
  s.zenith = mix(s.zenith, BR_DAWN_ZENITH_COL, dawnFactor);
  s.horizon = mix(s.horizon, BR_DAWN_HORIZON_COL, dawnFactor);

  float zh = dot(s.zenith, vec3_splat(0.33));
  float hh = dot(s.horizon, vec3_splat(0.33));
  float rainMix = brRain(env)*BR_SKY_RAIN_MIX;
  s.zenith = mix(s.zenith, BR_RAIN_ZENITH_COL*zh, rainMix);
  s.horizon = mix(s.horizon, BR_RAIN_HORIZON_COL*hh, rainMix);

  if (env.underwater) {
    vec3 water = env.fogCol*env.fogCol*BR_UNDERWATER_TINT;
    s.zenith = mix(2.0*water, water*zh, 0.8);
    s.horizon = mix(2.0*water, water*hh, 0.8);
  }
  return s;
}

// viewDir: normalized, y up
vec3 brRenderSky(brSkyColor s, vec3 viewDir) {
  float vh = 1.0 - viewDir.y*viewDir.y;
  float vh2 = vh*vh;
  float vh4 = vh2*vh2;
  float gradient = 0.8*vh4*vh4 + 0.2*vh2;
  vec3 sky = mix(s.zenith, s.horizon, gradient);
  return sky*(0.5 + 0.5*gradient);
}

#endif
