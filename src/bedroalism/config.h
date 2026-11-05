#ifndef BR_CONFIG_H
#define BR_CONFIG_H

/* Sun/Moon path (degrees) */
#define BR_SUN_PATH_YAW    15.0
#define BR_MOON_PATH_YAW   17.0
#define BR_SUN_PATH_TILT   31.0
#define BR_MOON_PATH_TILT -28.0

/* Sun/Moon size */
#define BR_SUN_SIZE  1.0 // 0.3 tiny ~ 4.0 massive
#define BR_MOON_SIZE 1.0 // 0.3 tiny ~ 4.0 massive
#define BR_SUN_TILT  0.0 // rotation of the sun texture (degrees)
#define BR_MOON_TILT 0.0 // rotation of the moon texture (degrees)

/* Exposure and gamma */
#define BR_EXPOSURE 1.2 // 0.5 dark ~ 2.5 bright
#define BR_GAMMA    2.2 // 1.8 bright ~ 2.6 dark

/* Night ambient (overworld sky light only, other times and light sources stay untouched) */
#define BR_NIGHT_AMBIENT 0.8 // 1.0 no change ~ 0.0 pitch black

/* Sky colors (linear, before exposure, tonemap and gamma) */
#define BR_DAY_ZENITH_COL      vec3(0.0503, 0.1517, 0.5165)
#define BR_DAY_HORIZON_COL     vec3(0.3802, 0.5165, 0.8003)

#define BR_SUNRISE_ZENITH_COL  vec3(0.0641, 0.1059, 0.2911)
#define BR_SUNRISE_HORIZON_COL vec3(1.3701, 0.3341, 0.1977)

#define BR_SUNSET_ZENITH_COL   vec3(0.0439, 0.0716, 0.2237)
#define BR_SUNSET_HORIZON_COL  vec3(1.3701, 0.1814, 0.0606)

#define BR_NIGHT_ZENITH_COL    vec3(0.0013, 0.0034, 0.0115)
#define BR_NIGHT_HORIZON_COL   vec3(0.0040, 0.0078, 0.0191)

#define BR_RAIN_ZENITH_COL     vec3(0.826, 0.973, 1.217) // tint of the overcast sky
#define BR_RAIN_HORIZON_COL    vec3(0.760, 0.831, 0.914)
#define BR_SKY_RAIN_MIX_FACTOR 1.0 // 0.0 no change ~ 1.0 fully overcast when raining
#define BR_SKY_AFTERGLOW       2.0 // 0.0 none ~ 4.0 strong, extra night sky brightness right after dusk

#endif
