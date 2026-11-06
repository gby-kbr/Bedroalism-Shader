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

/* Sun/Moon glow in the sky */
#define BR_SUN_GLOW  1.0 // 0.0 none ~ 2.0 strong
#define BR_MOON_GLOW 1.0 // 0.0 none ~ 2.0 strong

/* Shadow */
#define BR_SHADOW_STRENGTH   0.65 // 0.0 none ~ 1.0 black
#define BR_SHADOW_SUN_FOLLOW 1.0  // 0.0 side shadows always on the same faces ~ 1.0 side shadows follow the sun and moon

/* Water */
#define BR_WATER_COLOR      vec3(0.030, 0.270, 0.340) // linear color of the light scattered back by clear water
#define BR_WATER_OPACITY    0.35 // 0.1 glass clear ~ 0.7 murky, how much of the bottom is hidden looking straight down
#define BR_WATER_WAVE_HEIGHT 1.0 // 0.0 flat ~ 2.0 rough, scales the slope and the height of the waves
#define BR_WATER_WAVE_SPEED  0.8 // 0.0 still ~ 2.0 fast
#define BR_WATER_WAVE_RANGE  48.0 // blocks around the camera where the surface mesh moves up and down
#define BR_WATER_SUN_GLITTER 1.0 // 0.0 none ~ 2.0 strong, reflection of the sun and the moon
#define BR_WATER_CLOUD_REFLECTION 1.0 // 0.0 off (faster) ~ 1.0 clouds visible in the water
#define BR_UNDERWATER_DENSITY 1.0 // 0.5 clearer ~ 2.0 hazier, absorption and haze under the surface

/* Exposure and gamma */
#define BR_EXPOSURE 1.2 // 0.5 dark ~ 2.5 bright
#define BR_GAMMA    2.2 // 1.8 bright ~ 2.6 dark

/* Night ambient (overworld sky light only, other times and light sources stay untouched) */
#define BR_NIGHT_AMBIENT 0.5 // 1.0 no change ~ 0.0 pitch black

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

/* Clouds */
#define BR_CLOUD_COVERAGE   0.53  // 0.0 clear sky ~ 1.0 fully covered (rain adds to it)
#define BR_CLOUD_SCALE      0.12  // 0.06 huge clouds ~ 0.30 small clouds
#define BR_CLOUD_THICKNESS  0.6   // 0.2 flat ~ 1.2 tall and puffy
#define BR_CLOUD_WIND_SPEED 0.001 // 0.0 still ~ 0.005 fast (texture units per second)
#define BR_CLOUD_STEPS      6     // 4 faster, rougher ~ 8 slower, smoother (1 texture fetch per step)
#define BR_CIRRUS           0.35  // 0.0 off ~ 0.8 strong, thin high streaks (1 texture fetch)

#endif
