#ifndef BR_SKY_H
#define BR_SKY_H

#include "detection.h"

struct brSkyColor {
  vec3 zenith;
  vec3 horizon;
};

brSkyColor brDaySkyColor() {
  brSkyColor s;
  s.zenith = BR_DAY_ZENITH_COL;
  s.horizon = BR_DAY_HORIZON_COL;
  return s;
}

brSkyColor brSunriseSkyColor() {
  brSkyColor s;
  s.zenith = BR_SUNRISE_ZENITH_COL;
  s.horizon = BR_SUNRISE_HORIZON_COL;
  return s;
}

brSkyColor brSunsetSkyColor() {
  brSkyColor s;
  s.zenith = BR_SUNSET_ZENITH_COL;
  s.horizon = BR_SUNSET_HORIZON_COL;
  return s;
}

// night factor [0.0, 1.0]: brighter right after dusk, darkest at full night
brSkyColor brNightSkyColor(float night) {
  float afterglow = 1.0 + 2.0*(1.0 - night);
  brSkyColor s;
  s.zenith = BR_NIGHT_ZENITH_COL*afterglow;
  s.horizon = BR_NIGHT_HORIZON_COL*afterglow;
  return s;
}

// overcast sky keeps the brightness of the current sky, only desaturated
brSkyColor brRainSkyColor(brSkyColor clear, float rainFactor) {
  float zh = dot(clear.zenith, vec3(0.333, 0.333, 0.333));
  float hh = dot(clear.horizon, vec3(0.333, 0.333, 0.333));
  float rainMix = rainFactor*BR_SKY_RAIN_MIX_FACTOR;

  brSkyColor s;
  s.zenith = mix(clear.zenith, BR_RAIN_ZENITH_COL*zh, rainMix);
  s.horizon = mix(clear.horizon, BR_RAIN_HORIZON_COL*hh, rainMix);
  return s;
}

// day + night + sunrise + sunset = 1.0 (see detection.h)
brSkyColor brSkyColors(brEnvironment env) {
  brSkyColor day = brDaySkyColor();
  brSkyColor night = brNightSkyColor(env.night);
  brSkyColor sunrise = brSunriseSkyColor();
  brSkyColor sunset = brSunsetSkyColor();

  brSkyColor s;
  s.zenith = day.zenith*env.day + night.zenith*env.night + sunrise.zenith*env.sunrise + sunset.zenith*env.sunset;
  s.horizon = day.horizon*env.day + night.horizon*env.night + sunrise.horizon*env.sunrise + sunset.horizon*env.sunset;

  return brRainSkyColor(s, env.rainFactor);
}

#endif
