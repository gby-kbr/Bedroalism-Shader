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
#define BR_SUN_TILT  45.0 // rotation of the sun texture: 0.0 no tilt ~ 90.0
#define BR_MOON_TILT 45.0 // rotation of the moon texture: 0.0 no tilt ~ 90.0

/* Sky colors (0.0 ~ 1.0) */
#define BR_DAY_ZENITH_COL      vec3(0.20, 0.42, 0.82)
#define BR_DAY_HORIZON_COL     vec3(0.68, 0.79, 0.92)

#define BR_SUNRISE_ZENITH_COL  vec3(0.22, 0.33, 0.60)
#define BR_SUNRISE_HORIZON_COL vec3(0.98, 0.60, 0.45)

#define BR_SUNSET_ZENITH_COL   vec3(0.20, 0.28, 0.52)
#define BR_SUNSET_HORIZON_COL  vec3(0.98, 0.48, 0.25)

#define BR_NIGHT_ZENITH_COL    vec3(0.008, 0.048, 0.08)
#define BR_NIGHT_HORIZON_COL   vec3(0.02, 0.06, 0.10)

#define BR_RAIN_ZENITH_COL     vec3(0.78, 0.84, 0.92)
#define BR_RAIN_HORIZON_COL    vec3(0.85, 0.85, 0.85)
#define BR_SKY_RAIN_MIX_FACTOR 1.0 // 0.0 no change ~ 1.0 fully overcast when raining

#endif
