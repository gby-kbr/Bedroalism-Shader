#ifndef BR_CONFIG_H
#define BR_CONFIG_H

// Global configuration for Bedroalism Shader.
// Must be included before any file in functions/.

/* Sun/Moon path (degrees) */
#define BR_SUN_PATH_YAW    15.0
#define BR_MOON_PATH_YAW   17.0
#define BR_SUN_PATH_TILT   31.0
#define BR_MOON_PATH_TILT -28.0

/* Time of day thresholds, expressed as sine of the sun elevation (-1.0 ~ 1.0) */
#define BR_DAY_FULL_ELEVATION    0.4   // sun elevation sine where day factor reaches 1.0 (~23.6 degrees)
#define BR_NIGHT_FULL_ELEVATION -0.22  // sun elevation sine where night factor reaches 1.0 (~-12.7 degrees, end of nautical twilight)

#endif
