#ifndef BR_SHADOW_H
#define BR_SHADOW_H

#include "detection.h"

// Fake shadows from the lightmap. They do not follow the sun: they sit where the sky light is below full
// (under leaves and overhangs) and on the sides of blocks. Both only show where the sun can reach.

// how much the point is outdoors in the daytime: 0 inside, at night, under water and in other dimensions
float brSunlit(float skyLight, brEnvironment env) {
  float outdoors = smoothstep(BR_SHADOW_OUTDOOR_MIN, BR_SHADOW_OUTDOOR_MAX, skyLight);
  return outdoors*(1.0 - env.night)*(env.overworld && !env.underwater ? 1.0 : 0.0);
}

// 1.0 where the sky light is a little under full, near a torch it fades away
float brSkyShadow(vec2 lightmap) {
  float shadow = 1.0 - smoothstep(0.860, 0.875, pow(lightmap.y, 2.0));
  return shadow*(1.0 - min(pow(lightmap.x*1.2, 6.0), 1.0));
}

// 1.0 on the whole of the sides that face east or west, not on top, bottom or the slanted planes of plants
float brSideFace(vec3 geoNormal) {
  return step(0.9, abs(geoNormal.x));
}

// multiplier for the color of a lit block face
float brShadowFactor(vec2 lightmap, vec3 geoNormal, brEnvironment env) {
  float sunlit = brSunlit(lightmap.y, env);
  float shadow = brSkyShadow(lightmap)*sunlit;
  float side = brSideFace(geoNormal)*sunlit*(1.0 - shadow); // not on top of the first shadow
  return (1.0 - BR_SHADOW*shadow)*(1.0 - BR_SIDE_SHADOW*side);
}

#endif
