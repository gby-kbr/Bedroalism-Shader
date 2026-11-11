#ifndef BR_STARS_H
#define BR_STARS_H

// Real constellations from a baked star map (R stars, G lines), indexed by right ascension and declination.
// The sky turns with the same sun path as the sun and the moon, so the stars rise and set with them.
vec3 brStars(sampler2D starMap, vec3 dir, brEnvironment env) {
  float up = dir.y; // height above the horizon, before the sky is turned
  // back to the frame where the sky turns around the z axis
  dir.xz = brRotate2D(dir.xz, -brDegToRad(BR_SUN_PATH_YAW));
  dir.yz = brRotate2D(dir.yz, brDegToRad(BR_SUN_PATH_TILT));

  float hourAngle = atan2(-dir.x, dir.y);
  float ra = -hourAngle;
  float dec = asin(clamp(-dir.z, -1.0, 1.0)); // the game's +z is south, the map's pole is north
  vec4 map = texture2D(starMap, vec2(fract(ra/(2.0*BR_PI)), dec/BR_PI + 0.5));

  float visible = env.night*(1.0 - env.rainFactor)*smoothstep(0.0, 0.22, up);
  vec3 stars = vec3(0.96, 0.97, 1.0)*map.r*BR_STAR_BRIGHTNESS + vec3(0.42, 0.60, 0.95)*map.g*0.55*BR_STAR_LINES;
  return stars*visible;
}

#endif
