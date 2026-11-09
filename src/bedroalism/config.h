#ifndef BR_CONFIG_H
#define BR_CONFIG_H

// Color correction
#define BR_EXPOSURE 1.2  // 0.5 dark ~ 3.0 bright
#define BR_GAMMA    1.25 // 0.3 low ~ 2.0 high, Newb uses 1.33

// Uchimura tonemap
#define BR_UCHIMURA_P 1.0  // max display brightness
#define BR_UCHIMURA_A 1.1  // contrast, 1.0 neutral ~ 1.3 punchy
#define BR_UCHIMURA_M 0.2  // where the straight section starts
#define BR_UCHIMURA_L 0.5  // length of the straight section
#define BR_UCHIMURA_C 1.3  // black tightness, 1.0 lifted ~ 1.6 deep
#define BR_UCHIMURA_B 0.0  // pedestal, keep 0.0 or additive layers (sun, stars) lift black

// Sun and moon path
#define BR_SUN_PATH_YAW    15.0
#define BR_MOON_PATH_YAW   17.0
#define BR_SUN_PATH_TILT   31.0
#define BR_MOON_PATH_TILT -28.0


// Sky colors, linear, from Newb X
#define BR_DAY_ZENITH_COL    vec3(0.3, 0.9, 2.0)
#define BR_DAY_HORIZON_COL   vec3(1.0, 1.6, 1.8)
#define BR_DAWN_ZENITH_COL   vec3(0.1, 0.4, 0.7)
#define BR_DAWN_HORIZON_COL  vec3(3.0, 0.4, 0.4)
#define BR_NIGHT_ZENITH_COL  vec3(0.008, 0.048, 0.08)
#define BR_NIGHT_HORIZON_COL vec3(0.02, 0.06, 0.1)

#endif
