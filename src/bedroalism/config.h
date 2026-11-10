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

// Sun and moon path in degrees, must match the SunMoon material (the game's own sun has no tilt)
#define BR_SUN_PATH_YAW    0.0
#define BR_MOON_PATH_YAW   0.0
#define BR_SUN_PATH_TILT   0.0
#define BR_MOON_PATH_TILT  0.0

// Sky colors, linear, from Newb X
#define BR_DAY_ZENITH_COL    vec3(0.3, 0.9, 2.0)
#define BR_DAY_HORIZON_COL   vec3(1.0, 1.6, 1.8)
#define BR_DAWN_ZENITH_COL   vec3(0.1, 0.4, 0.7)
#define BR_DAWN_HORIZON_COL  vec3(3.0, 0.4, 0.4)
#define BR_NIGHT_ZENITH_COL  vec3(0.008, 0.048, 0.08)
#define BR_NIGHT_HORIZON_COL vec3(0.02, 0.06, 0.1)
#define BR_RAIN_ZENITH_COL   vec3(0.47, 0.51, 0.56)
#define BR_RAIN_HORIZON_COL  vec3(0.6, 0.6, 0.6)
#define BR_SKY_RAIN_MIX      1.0 // 0.0 no change ~ 1.0 fully overcast in rain

// Fog, overworld only
#define BR_FOG_DENSITY 0.003  // 0.0015 thin ~ 0.012 thick, light lost per block at sea level
#define BR_FOG_FALLOFF 0.02   // 0.008 deep layer ~ 0.05 thin ground mist, thinning with height
#define BR_FOG_RAIN    2.0    // 0.0 none ~ 5.0 dense, extra fog in rain
#define BR_FOG_DAWN    1.5    // 0.0 none ~ 4.0 dense, extra mist at sunrise and sunset
#define BR_RAIN_MIST   0.35   // 0.0 none ~ 0.6 thick, banks of mist that drift with the wind in rain
#define BR_GROUND_MIST 0.002  // 0.0 none ~ 0.006 thick, low patchy mist over water and ground
#define BR_GROUND_MIST_FALLOFF 0.08// 0.05 deep layer ~ 0.25 thin layer hugging the ground

// Water
#define BR_WATER_COLOR       vec3(0.030, 0.270, 0.340) // linear color of the light scattered back by clear water
#define BR_WATER_OPACITY     0.35 // 0.1 glass clear ~ 0.7 murky, how much of the bottom is hidden looking straight down
#define BR_WATER_WAVE_HEIGHT 0.75  // 0.0 flat ~ 2.0 rough
#define BR_WATER_WAVE_SPEED  0.6  // 0.0 still ~ 2.0 fast
#define BR_WATER_WAVE_RANGE  48.0 // blocks around the camera where the surface mesh moves
#define BR_WATER_SUN_GLITTER 1.0  // 0.0 none ~ 2.0 strong, reflection of the sun and the moon
#define BR_CAUSTICS          2.0  // 0.0 off ~ 2.0 strong, light pattern on the bottom, only under water
#define BR_UNDERWATER_DENSITY 0.9 // 0.5 clearer ~ 2.0 hazier, absorption and haze under the surface
#define BR_UNDERWATER_SURFACE_SMOOTH 0.75 // 0 = full waves, 1 = flat surface seen from below

// Wind
#define BR_PLANT_WAVE   0.05 // 0.02 gentle ~ 0.2 strong, grass, leaves and vines
#define BR_LANTERN_WAVE 0.16 // 0.05 subtle ~ 0.4 large swing
#define BR_WAVE_SPEED   2.0  // 0.5 slow ~ 5.0 fast
#define BR_WAVE_RANGE   24.0 // 8.0 near ~ 48.0 far, blocks around the camera that move

// Clouds
#define BR_CLOUD_COVER 0.5  // 0.0 clear ~ 1.0 overcast, how much of the sky the clouds cover
#define BR_CLOUD_WIND  0.05 // 0.0 still ~ 1.0 fast, how fast the clouds drift with the wind

// Shadows
#define BR_SHADOW      0.65 // 0.0 none ~ 1.0 black, the shadow under leaves and overhangs
#define BR_SIDE_SHADOW 0.65 // 0.0 none ~ 1.0 black, the shadow on the sides of blocks

#endif
