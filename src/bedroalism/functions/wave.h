#ifndef BR_WAVE_H
#define BR_WAVE_H

// Wind direction, the same as the drift of the mist.
#define BR_WIND_DIR vec2(0.912, 0.410)

// Smooth wind, about [-1, 1]: a swell that rolls across the world times a slow gust.
// Every vertex reads the wind at its own position, so neighbouring blocks never tear apart.
float brWind(vec3 gPos, float t, float rain) {
  float roll = dot(gPos, vec3(0.13, 0.05, 0.07));
  float a = t*BR_WAVE_SPEED + roll;
  float swell = sin(a) + 0.5*sin(1.7*a + 1.3);
  float gust = 0.5 + 0.5*sin(0.4*t + dot(gPos.xz, vec2(0.02, 0.015)));
  return swell*(0.35 + 0.65*gust)*(1.0 + 0.6*rain)*0.67;
}

// Plants, leaves and vines lean downwind, the tips more than the roots.
// texPosY: 0.0 at the top of the texture tile, 1.0 at its bottom
void brWavePlants(inout vec3 worldPos, vec3 gPos, vec3 bPos, vec4 color, float texPosY, float skyLight, float camDist, bool isTree, float t, float rain) {
  bool isColored = color.r != color.g || color.r != color.b;
  bool isTop = texPosY < 0.5;
  bool isPlants = color.r/color.g < 1.9;
  vec2 bPosC = abs(bPos.xz - 0.5);
  bool isVines = (bPosC.x == 0.453125 && bPos.z == 0.0) || (bPosC.y == 0.453125 && bPos.x == 0.0);
  bool isFarmPlant = bPos.y == 0.9375 && (bPosC.x == 0.25 || bPosC.y == 0.25);
  bool isLeafLitter = bPos.y == 0.015625 && (fract(bPos.x*2.0) + fract(bPos.z*2.0)) == 0.0;
  bool waves = ((isTree || isPlants || isVines) && isColored && !isLeafLitter) || (isFarmPlant && isTop);
  if (!waves) {
    return;
  }

  float weight = isTree ? 0.5 : 1.0;
  if (isVines) {
    weight = fract(0.01 + 0.5*gPos.y);
  } else if (isPlants && isColored && !isTree && !isTop) {
    weight = 0.0; // the roots stay in the ground
  }

  float fade = 1.0 - smoothstep(0.5, 1.0, camDist/BR_WAVE_RANGE);
  float w = BR_PLANT_WAVE*weight*fade*skyLight*(1.0 + brWind(gPos, t, rain));
  worldPos.xz += BR_WIND_DIR*w;
  worldPos.y -= 0.5*w*w; // the length of the stem stays the same
}

// Hanging lanterns and their chains swing as one piece around the top of the chain.
// The block geometry tests are the vanilla models, so only the lit ones move.
void brWaveLantern(inout vec3 worldPos, vec3 gPos, vec3 bPos, float texPosY, float torchLight, float camDist, float t, float rain) {
  vec2 bPosC = abs(bPos.xz - 0.5);
  bool y6875 = bPos.y == 0.6875;
  bool y5625 = bPos.y == 0.5625;
  bool isLantern = ((y6875 || y5625) && bPosC.x == 0.125) || ((y5625 || bPos.y == 0.125) && bPosC.x == 0.1875);
  bool isChain = bPosC.x == 0.0625 && y6875;
  if (y5625 && (texPosY < 0.001 || (texPosY > 0.43 && texPosY < 0.93))) {
    isLantern = false; // the top of a lantern that stands on the ground
  }
  if (torchLight > 0.6 && (isChain || isLantern)) {
    vec3 block = floor(gPos + 0.5 - bPos + 0.01); // one value for the whole block
    float fade = 1.0 - smoothstep(0.5, 1.0, camDist/BR_WAVE_RANGE);
    vec2 theta = BR_LANTERN_WAVE*fade*vec2(brWind(block, t, rain), brWind(block.zyx + 7.0, 1.3*t, rain));
    vec2 s = sin(theta);
    vec2 c = cos(theta);
    vec3 pivot = vec3(0.5, 1.0, 0.5) - bPos;
    worldPos.x += dot(pivot.xy, vec2(1.0 - c.x, -s.x));
    worldPos.y += dot(pivot, vec3(s.x*c.y, 1.0 - c.x*c.y, s.y));
    worldPos.z += dot(pivot, vec3(s.x*s.y, -c.x*s.y, 1.0 - c.y));
  }
}

#endif
