#ifndef BR_LAVA_H
#define BR_LAVA_H

#include "utils.h"

// Lava. A real lava lake is a thin crust of cooled rock floating on molten rock: the crust is black and
// cracks into plates that drift apart, and the glow comes out of the cracks, hottest where the crust is
// thinnest. The molten rock itself is viscous, the surface swells slowly and never splashes.
//
// Lava is found from facts of the geometry of the game: its top face is not white-tinted, sits 14/16 up
// the block, and its light level is between 12 and 13 (brIsLavaTop). The texture is not used at all:
// the plates and cracks come from the cell noise texture (the Worley octaves of the cloud noise: every
// cell is a plate and the creases between the cells are the cracks), warped by a second sample so the
// plates are not straight-edged, and moved with a slow drift.

// the top face of a still lava block
// color: vertex color. bPos: position inside the block (fract of the chunk position)
// lightmapUV: x = block light. uv0: texture coordinate
bool brIsLavaTop(vec4 color, vec3 bPos, vec2 lightmapUV, vec2 uv0) {
  bool white = (color.r + color.g + color.b) > 2.999 && color.a > 0.99;
  bool height = bPos.y < 0.891 && bPos.y > 0.889;
  bool light = lightmapUV.x > 0.80 && lightmapUV.x < 0.89;
  return white && height && light && uv0.y > 0.5;
}

// how far the lava surface is lifted at this position (blocks): slow, broad swells, three waves that do
// not line up. A function of the world position, so neighbouring blocks stay joined
// p: absolute world xz. t: seconds
float brLavaSwell(vec2 p, float t) {
  float s = sin(dot(p, vec2(0.47, 0.31)) + 0.55*t) + sin(dot(p, vec2(-0.29, 0.61)) - 0.43*t + 1.7) + sin(dot(p, vec2(0.83, -0.12)) + 0.71*t + 4.1);
  return BR_LAVA_SWELL*(s*0.333)*0.5;
}

// color of the molten rock by temperature, linear and above 1.0: the exposure and tonemap take it down.
// heat 0.0 ~ 1.0. dull red at 800 C, orange, yellow near 1200 C
vec3 brLavaHeatColor(float heat) {
  vec3 red = vec3(0.60, 0.07, 0.01);
  vec3 orange = vec3(2.00, 0.62, 0.07);
  vec3 yellow = vec3(3.40, 1.90, 0.55);
  return mix(mix(red, orange, smoothstep(0.0, 0.55, heat)), yellow, smoothstep(0.60, 1.0, heat));
}

// emissive color of the lava surface, it is lit by itself: no sky light, no shadow
// noiseTex: the cell noise texture. p: absolute world xz. t: seconds. camDist: distance to the camera
vec3 brLavaSurface(sampler2D noiseTex, vec2 p, float t, float camDist) {
  // slow drift of the whole crust, and a warp so that the plates bend as they move
  vec2 uv = p*0.045 + vec2(0.0035, 0.0012)*t;
  vec2 warp = texture2D(noiseTex, uv*0.8 + vec2(0.31, 0.57) - vec2(0.0020, 0.0030)*t).gb - 0.5;
  uv += warp*0.06;

  vec4 c = texture2D(noiseTex, uv);          // r: big plates, g: plates, b and a: finer cracks
  float plate = mix(c.g, c.r, 0.35);         // low in the cracks between the plates
  float fine = c.b*0.6 + c.a*0.4;

  // the crust is thinner in patches: there the cracks open wide. The patches move and breathe slowly
  vec4 hotSpot = texture2D(noiseTex, p*0.017 + vec2(0.0009, -0.0006)*t);
  float thin = smoothstep(0.45, 0.85, hotSpot.r*0.6 + hotSpot.g*0.4 + 0.15*sin(0.5*t + hotSpot.b*6.28));

  // crack: plate value below a threshold that grows where the crust is thin
  float limit = 0.12 + 0.10*thin;
  float crack = 1.0 - smoothstep(limit - 0.05, limit + 0.05, plate);
  // finer cracks inside the plates
  float hairline = (1.0 - smoothstep(0.07, 0.17, fine))*0.50;
  float glow = clamp(max(crack, hairline*(0.4 + 0.6*thin)), 0.0, 1.0);

  // hottest in the middle of a crack and where the crust is thin
  float heat = clamp(glow*(0.45 + 0.55*crack)*(0.55 + 0.6*thin), 0.0, 1.0);
  vec3 molten = brLavaHeatColor(heat);

  // the crust: black rock, a dull red from the heat just under it, brighter near a crack
  float rim = 1.0 - smoothstep(limit, limit + 0.30, plate); // the rock heated just next to a crack
  vec3 crust = vec3(0.030, 0.020, 0.018)*(0.5 + 0.9*fine) + vec3(0.55, 0.075, 0.008)*rim*rim;

  vec3 col = mix(crust, molten, smoothstep(0.02, 0.55, glow));

  // far away the pattern is smaller than a pixel: take the average so that it does not shimmer
  vec3 average = mix(crust, brLavaHeatColor(0.32), 0.22 + 0.20*thin);
  col = mix(col, average, smoothstep(45.0, 110.0, camDist));
  return col*BR_LAVA_BRIGHTNESS;
}

#endif
