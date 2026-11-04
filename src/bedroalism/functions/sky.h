#ifndef BR_SKY_H
#define BR_SKY_H

#include "detection.h"

struct brSkyColor {
  vec3 zenith;
  vec3 horizon;
};

brSkyColor brDaySkyColor() {
  brSkyColor s;
  s.zenith = vec3(0.20, 0.42, 0.82);
  s.horizon = vec3(0.68, 0.79, 0.92);
  return s;
}

brSkyColor brSunriseSkyColor() {
  brSkyColor s;
  s.zenith = vec3(0.22, 0.33, 0.60);
  s.horizon = vec3(0.98, 0.60, 0.45);
  return s;
}

brSkyColor brSunsetSkyColor() {
  brSkyColor s;
  s.zenith = vec3(0.20, 0.28, 0.52);
  s.horizon = vec3(0.98, 0.48, 0.25);
  return s;
}

// night factor [0.0, 1.0]: brighter right after dusk, darkest at full night
brSkyColor brNightSkyColor(float night) {
  float afterglow = 1.0 + 2.0*(1.0 - night);
  brSkyColor s;
  s.zenith = vec3(0.008, 0.048, 0.08)*afterglow;
  s.horizon = vec3(0.02, 0.06, 0.10)*afterglow;
  return s;
}

// overcast sky keeps the brightness of the current sky, only desaturated
brSkyColor brRainSkyColor(brSkyColor clear, float rainFactor) {
  float zh = dot(clear.zenith, vec3(0.333, 0.333, 0.333));
  float hh = dot(clear.horizon, vec3(0.333, 0.333, 0.333));
  float rainMix = rainFactor*0.9;

  brSkyColor s;
  s.zenith = mix(clear.zenith, vec3(0.78, 0.84, 0.92)*zh, rainMix);
  s.horizon = mix(clear.horizon, vec3(0.85, 0.85, 0.85)*hh, rainMix);
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
