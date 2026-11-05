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

/* Exposure (blends smoothly between times of day) */
#define BR_EXPOSURE_DAY      1.2 // 0.5 dark ~ 2.5 bright
#define BR_EXPOSURE_TWILIGHT 1.3 // sunrise and sunset
#define BR_EXPOSURE_NIGHT    1.6 // higher than day, the eyes adapt to the dark

/* Gamma */
#define BR_GAMMA 2.2 // 1.8 bright ~ 2.6 dark

/* Sky colors (linear, before exposure, tonemap and gamma) */
#define BR_DAY_ZENITH_COL      vec3(0.0503, 0.1517, 0.5165)
#define BR_DAY_HORIZON_COL     vec3(0.3802, 0.5165, 0.8003)

#define BR_SUNRISE_ZENITH_COL  vec3(0.0592, 0.0978, 0.2687)
#define BR_SUNRISE_HORIZON_COL vec3(1.2647, 0.3084, 0.1825)

#define BR_SUNSET_ZENITH_COL   vec3(0.0405, 0.0661, 0.2065)
#define BR_SUNSET_HORIZON_COL  vec3(1.2647, 0.1674, 0.0559)

#define BR_NIGHT_ZENITH_COL    vec3(0.0010, 0.0025, 0.0086)
#define BR_NIGHT_HORIZON_COL   vec3(0.0030, 0.0059, 0.0143)

#define BR_RAIN_ZENITH_COL     vec3(0.826, 0.973, 1.217) // tint of the overcast sky
#define BR_RAIN_HORIZON_COL    vec3(0.760, 0.831, 0.914)
#define BR_SKY_RAIN_MIX_FACTOR 1.0 // 0.0 no change ~ 1.0 fully overcast when raining
#define BR_SKY_AFTERGLOW       2.0 // 0.0 none ~ 4.0 strong, extra night sky brightness right after dusk

#endif
