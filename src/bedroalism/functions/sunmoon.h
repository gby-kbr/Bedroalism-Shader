#ifndef BR_SUNMOON_H
#define BR_SUNMOON_H

// The sun and the moon quads are placed here, instead of by the game, so they follow the same tilted
// path as brCalculateSunParams. The quad is 70 units wide at a distance of 300 (as in Newb X).
#define BR_SUNMOON_DIST  300.0
#define BR_SUNMOON_SCALE 70.0

float brSunSize()  { return BR_SUN_SIZE; }
float brMoonSize() { return BR_MOON_SIZE; }

// position: the quad vertex of the game, xz in [-1, 1]. angle: time of day * 2 pi
vec3 brSunMoonPosition(vec3 position, float angle, bool isSun) {
  float size = isSun ? brSunSize() : brMoonSize();
  float tilt = brDegToRad(isSun ? BR_SUN_PATH_TILT : BR_MOON_PATH_TILT);
  float yaw = brDegToRad(isSun ? BR_SUN_PATH_YAW : BR_MOON_PATH_YAW);

  // same circle as the light direction: the sun at (-sin, cos), the moon on the other side
  vec2 along = vec2(cos(angle), sin(angle)); // tangent of the circle, the quad's x axis
  vec2 circle = isSun ? vec2(-sin(angle), cos(angle)) : vec2(sin(angle), -cos(angle));
  float x = isSun ? position.x : -position.x;

  vec3 p = vec3(circle*BR_SUNMOON_DIST + along*(x*BR_SUNMOON_SCALE*size), position.z*BR_SUNMOON_SCALE*size);
  p.yz = brRotate2D(p.yz, -tilt);
  p.xz = brRotate2D(p.xz, yaw);
  return p;
}

#endif
