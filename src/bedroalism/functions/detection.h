#ifndef BR_DETECTION_H
#define BR_DETECTION_H

#include "utils.h"

struct brEnvironment {
  bool end;
  bool nether;
  bool underwater;
  bool overworld;
  float rainFactor; // [0.0, 1.0]
  float dayFactor;  // [-1.0, 1.0]
  float day;        // [0.0, 1.0]
  float night;      // [0.0, 1.0]
  float twilight;   // [0.0, 1.0] sunrise and sunset
  vec3 sunDir;      // normalized
  vec3 moonDir;     // normalized
  vec3 fogCol;      // [0.0, 1.0]
};

bool brDetectEnd(vec3 FOG_COLOR) {
  // fog color is set to #DDDFA5 (end stone) in fogs/the_end_fog_setting.json, no other dimension has it
  vec3 d = abs(FOG_COLOR - BR_END_FOG_COL);
  return max(max(d.r, d.g), d.b) < 0.006;
}

bool brDetectNether(vec3 FOG_COLOR, vec2 FOG_CONTROL) {
  // FOG_CONTROL.x follows the render distance, fitted with an accuracy of 0.02
  float expectedFogX = 0.029 + (0.09*FOG_CONTROL.y*FOG_CONTROL.y);

  bool netherFogCtrl = (FOG_CONTROL.x<0.14 && abs(FOG_CONTROL.x-expectedFogX) < 0.02);
  bool netherFogCol = (FOG_COLOR.r+FOG_COLOR.g)>0.0;

  // under lava counts as the Nether
  bool underLava = FOG_CONTROL.x == 0.0 && FOG_COLOR.b == 0.0 && FOG_COLOR.g < 0.18 && FOG_COLOR.r-FOG_COLOR.g > 0.1;

  return (netherFogCtrl && netherFogCol) || underLava;
}

bool brDetectUnderwater(vec3 FOG_COLOR, vec2 FOG_CONTROL) {
  return FOG_CONTROL.x==0.0 && FOG_CONTROL.y<0.8 && (FOG_COLOR.b>FOG_COLOR.r || FOG_COLOR.g>FOG_COLOR.r);
}

float brDetectRain(vec3 FOG_CONTROL) {
  // clear weather FOG_CONTROL.x is 0.5 + 1.25/k, k = render distance in chunks (z = k*16)
  vec2 clear = vec2(0.5 + 20.0/FOG_CONTROL.z, 1.0);
  vec2 rain = vec2(0.23, 0.70);
  vec2 factor = clamp((FOG_CONTROL.xy-clear)/(rain-clear), vec2(0.0, 0.0), vec2(1.0, 1.0));
  float val = factor.x*factor.y;
  return val*val*(3.0 - 2.0*val);
}

float brDetectDayFactor(vec3 FOG_COLOR) {
  return min(dot(FOG_COLOR, vec3(0.5, 0.7, 0.5)), 1.0);
}

brEnvironment brCalculateSunParams(brEnvironment env, float TIME_OF_DAY) {
  float t = 2.0 * BR_PI * TIME_OF_DAY;
  // the game's sun rises in the east (+x), Newb X mirrors it because its own sun quad is mirrored
  vec3 sunDir = vec3(-sin(t), cos(t), 0.0);
  vec3 moonDir = vec3(-sunDir.x, -sunDir.y, 0.0);
  env.dayFactor = sunDir.y;
  env.day = smoothstep(0.0, 0.4, env.dayFactor);
  env.night = 1.0 - smoothstep(-0.22, 0.0, env.dayFactor);
  env.twilight = clamp(1.0 - env.day - env.night, 0.0, 1.0);

  sunDir.yz = brRotate2D(sunDir.yz, -brDegToRad(BR_SUN_PATH_TILT));
  sunDir.xz = brRotate2D(sunDir.xz, brDegToRad(BR_SUN_PATH_YAW));

  moonDir.yz = brRotate2D(moonDir.yz, -brDegToRad(BR_MOON_PATH_TILT));
  moonDir.xz = brRotate2D(moonDir.xz, brDegToRad(BR_MOON_PATH_YAW));

  env.sunDir = sunDir;
  env.moonDir = moonDir;
  return env;
}

brEnvironment brDetectEnvironment(float TIME_OF_DAY, vec3 FOG_COLOR, vec3 FOG_CONTROL) {
  brEnvironment env;
  env.end = brDetectEnd(FOG_COLOR);
  env.nether = brDetectNether(FOG_COLOR, FOG_CONTROL.xy);
  env.underwater = brDetectUnderwater(FOG_COLOR, FOG_CONTROL.xy);
  env.overworld = !env.end && !env.nether;
  env.rainFactor = brDetectRain(FOG_CONTROL.xyz);
  env.fogCol = FOG_COLOR;
  env = brCalculateSunParams(env, TIME_OF_DAY);
  return env;
}

#endif
