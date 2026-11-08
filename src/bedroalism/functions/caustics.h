#ifndef BR_CAUSTICS_H
#define BR_CAUSTICS_H

#include "water.h"

// ---------------------------------------------------------------------------------------------
// Caustics
//
// Light that enters the water through a curved surface is focused where the surface bulges like a lens and
// spread out where it hollows, and lands on the bottom as a net of thin bright lines. The texture
// (textures/environment/caustic_noise.png, made by script/generate_caustic_noise.py) is that net, simulated
// by sending rays through a random wave surface with Snell's law: 256x256 px for 10x10 blocks, tileable. Its 4
// channels hold the same surface seen at the water depths 1.5, 3.5, 7 and 12 blocks: shallow caustics are
// soft, deep ones are sharp and folded. The shader blends the channels by the depth of the point.
//
// The animation: two layers of different scale and direction slide over each other, and the depth they are
// read at swells slowly, so the lines also fold and open like real caustics instead of only drifting.
// Water bends blue light more than red, so the three colors are read at slightly different depths.
// Only under water in the overworld: the function returns the color unchanged anywhere else.
// ---------------------------------------------------------------------------------------------

#define BR_CAUSTIC_SURFACE_Y 62.8   // height of the water surface of the sea
#define BR_WATER_INDEX       1.333  // refraction index of water

// weights of the 4 channels (depth 1.5, 3.5, 7, 12 blocks) for a depth in blocks
vec4 brCausticDepthWeights(float depth) {
  float f = clamp((depth - 1.5)/2.0, 0.0, 1.0) + clamp((depth - 3.5)/3.5, 0.0, 1.0) + clamp((depth - 7.0)/5.0, 0.0, 1.0);
  return clamp(1.0 - abs(f - vec4(0.0, 1.0, 2.0, 3.0)), 0.0, 1.0);
}

// light density on the bottom for red, green and blue, 1.0 is the light of a flat surface
// p: where the light entered the surface (x, z), blocks. depth: blocks of water above the point
vec3 brCausticLight(sampler2D tex, vec2 p, float t, float depth) {
  // layer 1: 10 block tile, drifts one way. layer 2: turned and smaller, drifts the other way
  vec2 uv1 = p*0.1 + vec2(0.040, 0.022)*t;
  vec2 uv2 = vec2(0.80*p.x - 0.60*p.y, 0.60*p.x + 0.80*p.y)*0.153 + vec2(-0.030, 0.046)*t;
  vec4 c1 = texture2D(tex, uv1)*8.0;
  vec4 c2 = texture2D(tex, uv2)*8.0;

  // the depth swells, a little differently for each layer
  float d1 = depth*(1.0 + 0.22*sin(0.9*t + 1.3));
  float d2 = depth*(1.0 + 0.22*sin(0.7*t + 4.1));
  vec3 disp = vec3(0.93, 1.0, 1.07);                       // red is bent least, blue most
  vec3 l1 = vec3(dot(c1, brCausticDepthWeights(d1*disp.x)), dot(c1, brCausticDepthWeights(d1*disp.y)), dot(c1, brCausticDepthWeights(d1*disp.z)));
  vec3 l2 = vec3(dot(c2, brCausticDepthWeights(d2*disp.x)), dot(c2, brCausticDepthWeights(d2*disp.y)), dot(c2, brCausticDepthWeights(d2*disp.z)));

  // two surfaces cross: where both focus, the light piles up
  return sqrt(l1*l2)*1.45;
}

// col: the lit color of a surface. worldXZ, worldY: where it is. camDist: distance to the camera, blocks
// geoNormal: normal of the face toward the camera. skyLight: lightmap y of the surface
vec3 brCaustics(sampler2D tex, vec3 col, vec2 worldXZ, float worldY, float camDist, vec3 geoNormal, float skyLight, brEnvironment env, float time) {
  if (!env.overworld || !env.underwater) {
    return col;
  }

  float depth = clamp(BR_CAUSTIC_SURFACE_Y - worldY, 0.4, 14.0);

  // the sun refracts into the water, the pattern lies shifted away from the sun: the ray that lights a point
  // entered the surface closer to the sun by d*tan(refracted angle)
  float sinI = length(env.sunDir.xz);
  float sinR = sinI/BR_WATER_INDEX;
  vec2 toSun = env.sunDir.xz/max(sinI, 0.0001);
  vec2 entry = worldXZ + toSun*(depth*sinR/sqrt(1.0 - sinR*sinR));

  vec2 p = mod(entry, vec2(512.0, 512.0));
  vec3 light = brCausticLight(tex, p, time, depth);

  // only sunlight makes the pattern: by day, in the open sky light, fading with the depth as the water absorbs
  float sun = env.day + 0.5*env.twilight;
  float open = smoothstep(0.35, 0.9, skyLight);
  float up = smoothstep(0.1, 0.7, geoNormal.y);
  float reach = exp(-0.16*depth)*(1.0 - smoothstep(30.0, 70.0, camDist));
  float k = clamp(BR_CAUSTICS*sun*open*up*reach*(1.0 - 0.6*env.rainFactor), 0.0, 1.0);

  return col*mix(vec3(1.0, 1.0, 1.0), light, k);
}

#endif
