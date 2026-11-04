#ifndef BR_DETECTION_H
#define BR_DETECTION_H

// Detection functions for Bedroalism Shader.
//
// Inputs follow the standard material uniforms:
//   TIME_OF_DAY  : TimeOfDay.x
//   FOG_COLOR    : FogColor.rgb
//   FOG_CONTROL  : FogControl.xyz / FogAndDistanceControl.xyz
//                  (x = fog start, y = fog end, z = render distance)
//
// Detection logic is ported from Newb X (devendrn/newb-x-mcbe).

#include "utils.h"

struct brEnvironment {
  // Dimension
  bool overworld;
  bool nether;
  bool end;

  // Environment
  bool underwater;
  bool underLava;   // also counted as nether
  float rainFactor; // [0.0, 1.0]

  // Time
  float dayFactor;  // [-1.0, 1.0] sine of the sun elevation
  float day;        // [0.0, 1.0]
  float night;      // [0.0, 1.0]
  float twilight;   // [0.0, 1.0] sunrise + sunset
  float sunrise;    // [0.0, 1.0]
  float sunset;     // [0.0, 1.0]

  // Directions
  vec3 sunDir;      // normalized
  vec3 moonDir;     // normalized

  vec3 fogCol;      // [0.0, 1.0]
};

/* ---------------------------------------------------------------- */
/* Dimension                                                        */
/* ---------------------------------------------------------------- */

bool brDetectEnd(vec3 FOG_COLOR) {
  // custom fog color set in the_end fog settings (mcpack) to help in detection
  return FOG_COLOR.r==FOG_COLOR.b && (FOG_COLOR.r-FOG_COLOR.g>0.24 || (FOG_COLOR.g==0.0 && FOG_COLOR.r>0.1));
}

bool brDetectUnderLava(vec3 FOG_COLOR, vec2 FOG_CONTROL) {
  return FOG_CONTROL.x == 0.0 && FOG_COLOR.b == 0.0 && FOG_COLOR.g < 0.18 && FOG_COLOR.r-FOG_COLOR.g > 0.1;
}

bool brDetectNether(vec3 FOG_COLOR, vec2 FOG_CONTROL) {
  // fogctrl.xy varies with renderdistance
  // x range (0.03,0.14)

  // reverse plotted relation (5,6,7,8,9,11,12,20,96 chunks data) with an accuracy of 0.02
  float expectedFogX = 0.029 + (0.09*FOG_CONTROL.y*FOG_CONTROL.y);

  // nether wastes, basalt delta, crimson forest, wrapped forest, soul sand valley
  bool netherFogCtrl = (FOG_CONTROL.x<0.14  && abs(FOG_CONTROL.x-expectedFogX) < 0.02);
  bool netherFogCol = (FOG_COLOR.r+FOG_COLOR.g)>0.0;

  // consider underlava as nether
  return (netherFogCtrl && netherFogCol) || brDetectUnderLava(FOG_COLOR, FOG_CONTROL);
}

/* ---------------------------------------------------------------- */
/* Environment                                                      */
/* ---------------------------------------------------------------- */

bool brDetectUnderwater(vec3 FOG_COLOR, vec2 FOG_CONTROL) {
  return FOG_CONTROL.x==0.0 && FOG_CONTROL.y<0.8 && (FOG_COLOR.b>FOG_COLOR.r || FOG_COLOR.g>FOG_COLOR.r);
}

float brDetectRain(vec3 FOG_CONTROL) {
  // clear fogctrl.x varies with render distance (z)
  // reverse plotted as 0.5 + 1.25/k (k is renderdistance in chunks, fogctrl.z = k*16)
  vec2 clear = vec2(0.5 + 20.0/FOG_CONTROL.z, 1.0); // clear fogctrl value
  vec2 rain = vec2(0.23, 0.70); // rain fogctrl value
  vec2 factor = clamp((FOG_CONTROL.xy-clear)/(rain-clear), vec2(0.0,0.0), vec2(1.0,1.0));
  float val = factor.x*factor.y;
  return val*val*(3.0 - 2.0*val);
}

/* ---------------------------------------------------------------- */
/* Time                                                             */
/* ---------------------------------------------------------------- */

// Sine of the sun elevation: 1.0 at noon, 0.0 at the horizon, -1.0 at midnight
float brDetectDayFactor(float TIME_OF_DAY) {
  return cos(2.0 * BR_PI * TIME_OF_DAY);
}

float brDetectDay(float dayFactor) {
  return smoothstep(0.0, 0.4, dayFactor);
}

float brDetectNight(float dayFactor) {
  return 1.0 - smoothstep(-0.22, 0.0, dayFactor);
}

// Transition between day and night; day + night + twilight = 1.0
float brDetectTwilight(float dayFactor) {
  return clamp(1.0 - brDetectDay(dayFactor) - brDetectNight(dayFactor), 0.0, 1.0);
}

// 1.0 when the sun is moving down (afternoon), 0.0 when moving up (morning)
float brDetectSunSetSide(float TIME_OF_DAY) {
  return smoothstep(-0.05, 0.05, sin(2.0 * BR_PI * TIME_OF_DAY));
}

float brDetectSunrise(float TIME_OF_DAY) {
  return brDetectTwilight(brDetectDayFactor(TIME_OF_DAY)) * (1.0 - brDetectSunSetSide(TIME_OF_DAY));
}

float brDetectSunset(float TIME_OF_DAY) {
  return brDetectTwilight(brDetectDayFactor(TIME_OF_DAY)) * brDetectSunSetSide(TIME_OF_DAY);
}

/* ---------------------------------------------------------------- */
/* Sun and moon direction                                           */
/* ---------------------------------------------------------------- */

vec3 brGetSunDirection(float TIME_OF_DAY) {
  float t = 2.0 * BR_PI * TIME_OF_DAY;
  vec3 sunDir = vec3(sin(t), cos(t), 0.0);
  sunDir.yz = mul(brRotation2D(-brDegToRad(31.0)), sunDir.yz);
  sunDir.xz = mul(brRotation2D(brDegToRad(15.0)), sunDir.xz);
  return sunDir;
}

vec3 brGetMoonDirection(float TIME_OF_DAY) {
  float t = 2.0 * BR_PI * TIME_OF_DAY;
  vec3 moonDir = vec3(-sin(t), -cos(t), 0.0);
  moonDir.yz = mul(brRotation2D(brDegToRad(28.0)), moonDir.yz);
  moonDir.xz = mul(brRotation2D(brDegToRad(17.0)), moonDir.xz);
  return moonDir;
}

brEnvironment brCalculateTimeParams(brEnvironment env, float TIME_OF_DAY) {
  env.dayFactor = brDetectDayFactor(TIME_OF_DAY);
  env.day = brDetectDay(env.dayFactor);
  env.night = brDetectNight(env.dayFactor);
  env.twilight = brDetectTwilight(env.dayFactor);
  env.sunrise = brDetectSunrise(TIME_OF_DAY);
  env.sunset = brDetectSunset(TIME_OF_DAY);
  env.sunDir = brGetSunDirection(TIME_OF_DAY);
  env.moonDir = brGetMoonDirection(TIME_OF_DAY);
  return env;
}

/* ---------------------------------------------------------------- */
/* Combined detection                                               */
/* ---------------------------------------------------------------- */

brEnvironment brDetectEnvironment(float TIME_OF_DAY, vec3 FOG_COLOR, vec3 FOG_CONTROL) {
  brEnvironment env;
  env.end = brDetectEnd(FOG_COLOR);
  env.nether = brDetectNether(FOG_COLOR, FOG_CONTROL.xy);
  env.overworld = !env.end && !env.nether;
  env.underwater = brDetectUnderwater(FOG_COLOR, FOG_CONTROL.xy);
  env.underLava = brDetectUnderLava(FOG_COLOR, FOG_CONTROL.xy);
  env.rainFactor = brDetectRain(FOG_CONTROL.xyz);
  env.fogCol = FOG_COLOR;
  env = brCalculateTimeParams(env, TIME_OF_DAY);
  return env;
}

#endif
