#ifndef BR_WAVE_H
#define BR_WAVE_H

#include "utils.h"
#include "detection.h"

// ---------------------------------------------------------------------------------------------
// Waving of plants, leaves, vines and hanging lanterns
//
// Wind is not random shaking. It blows in one direction that turns slowly, in gust fronts: long swells
// of stronger air that travel with the wind over the land. Plants bend with the gust and spring back in a
// lull, so the push is never negative; on top of it comes a quicker, smaller flutter over a few blocks.
// The wind is a smooth function of the world position and the time, so neighbours that touch move together
// and nothing tears: a leaf block is one piece, the vines of a column keep their line.
// A bent plant keeps its length: the farther the tip leans, the lower it drops (the arc of a circle).
// A lantern is a pendulum: the wind pushes it, and on top of that it keeps swinging at its own
// frequency, sqrt(g/L), L the length of the chain, which is what makes it look heavy and not rubbery.
// ---------------------------------------------------------------------------------------------

// the sway of the air at a position: direction and strength of the push, about 0.0 ~ 1.5
// p: world position (x, z), t: time in seconds
vec2 brWind(vec2 p, float t, float rain) {
  t *= BR_WAVE_SPEED;

  float turn = 0.6 + 0.35*sin(t*0.021) + 0.20*sin(t*0.053 + 2.0);
  vec2 dir = vec2(cos(turn), sin(turn));
  vec2 side = vec2(-dir.y, dir.x);
  float al = dot(p, dir);       // distance along the wind
  float ac = dot(p, side);      // distance across it

  // gust fronts, wavelength 33 and 13 blocks, they travel at 5.8 and 3.7 blocks per second, bent a little sideways
  float gust = 0.40 + 0.26*sin(al*0.19 - t*1.10 + 0.9*sin(ac*0.09))
                    + 0.20*sin(al*0.47 - t*1.75 + 1.7 + 0.6*sin(ac*0.23));
  gust *= 1.0 + 0.7*rain;

  // flutter, wavelength 5 and 3 blocks
  float f1 = sin(al*1.15 - t*3.3 + ac*0.7);
  float f2 = sin(al*1.90 - t*4.6 - ac*1.3 + 2.0);

  vec2 sway = dir*(gust*(1.0 + 0.30*f1)) + side*(0.22*gust*f2);
  float len = length(sway);
  return sway*(min(len, 1.6)/max(len, 0.0001));
}

// bends a point by the sway. amount: how far it leans in blocks. The drop keeps the length of the plant
vec3 brBend(vec3 pos, vec2 sway, float amount) {
  vec2 d = sway*amount;
  pos.xz += d;
  pos.y -= 0.5*dot(d, d);
  return pos;
}

// swing of a hanging lantern around the top of its block. r: the point relative to the pivot
vec3 brSwing(vec3 pos, vec3 pivot, vec2 tilt) {
  vec3 r = pos - pivot;
  float ca = cos(tilt.x);
  float sa = sin(tilt.x);
  float cb = cos(tilt.y);
  float sb = sin(tilt.y);
  // around z by tilt.x (moves x), then around x by tilt.y (moves z)
  float x1 = r.x*ca - r.y*sa;
  float y1 = r.x*sa + r.y*ca;
  float y2 = y1*cb - r.z*sb;
  float z2 = y1*sb + r.z*cb;
  return pivot + vec3(x1, y2, z2);
}

// the position of a vertex of a plant, a leaf, a vine or a lantern after the wind
// worldPos: relative to the camera. cPos: position in the chunk (a_position). gPos: world position
// color: vertex color. uv0: texture coordinate. lightmap: block light, sky light
// atlasHeight: height of the terrain atlas in pixels. camDist: blocks. seasons: the leaves of the seasons pass
vec3 brWaveVertex(vec3 worldPos, vec3 cPos, vec3 gPos, vec4 color, vec2 uv0, vec2 lightmap, float atlasHeight, float camDist, float rain, float t, bool seasons) {
  if (camDist > BR_WAVE_RANGE) {
    return worldPos;
  }
  // the wave fades out before the end of the range, nothing pops
  float fade = 1.0 - smoothstep(0.5*BR_WAVE_RANGE, BR_WAVE_RANGE, camDist);

  vec3 bPos = fract(cPos);                      // position in the block
  vec2 bC = abs(bPos.xz - 0.5);
  bool isColored = color.r != color.g || color.r != color.b;
  float texY = fract(uv0.y*atlasHeight/16.0);   // 0.0 top of the tile ~ 1.0 bottom, tiles are 16 px
  bool isTop = texY < 0.5;

  bool isLeaf = (isColored && (bPos.x + bPos.y + bPos.z) < 0.001) || color.a == 0.0 || seasons;
  bool isGreen = color.r/max(color.g, 0.001) < 1.9;     // grass, ferns, vines: green tints (redstone dust is red)
  bool isVine = (bC.x == 0.453125 && bPos.z == 0.0) || (bC.y == 0.453125 && bPos.x == 0.0);
  bool isFlat = bPos.y == 0.015625;                      // lily pads and leaf litter lie flat, they do not wave
  bool isCrop = bPos.y == 0.9375 && (bC.x == 0.25 || bC.y == 0.25);

  // leaves and vines take the wind at their own position, so touching blocks move together. A plant takes it
  // at the center of its block, shifted by a random amount of its own, so the whole plant moves as one
  // and the tip bends, and neighbours do not all sway in step
  vec2 center = gPos.xz - bPos.xz + 0.5;
  vec2 own2 = brHash22(floor(cPos.xz));
  vec2 swayAt = brWind(gPos.xz + vec2(gPos.y*0.6, 0.0), t, rain);
  vec2 sway = brWind(center + (own2 - 0.5)*5.0, t, rain);
  float sky = smoothstep(0.3, 0.9, lightmap.y);          // in a cave or under a roof there is no wind

  // lanterns: the top part of the model hangs from the chain at the top of the block. Same geometry
  // test as the model of the game: the body and the chain piece have fixed heights and offsets in the block.
  // Only where light comes from them (block light), a standing lantern has the same heights
  bool y6875 = bPos.y == 0.6875;
  bool y5625 = bPos.y == 0.5625;
  bool isLantern = ((y6875 || y5625) && bC.x == 0.125) || ((y5625 || bPos.y == 0.125) && bC.x == 0.1875);
  bool isChain = bC.x == 0.0625 && y6875;
  if (y5625 && (texY < 0.001 || (texY > 0.43 && texY < 0.93))) {
    isLantern = false;                                    // the top of a standing lantern stays
  }
  if (lightmap.x > 0.6 && (isChain || isLantern)) {
    float phase = 6.2831853*brHash12(floor(cPos.xz));    // every lantern has its own rhythm
    float ring = 3.1*t*BR_WAVE_SPEED;                     // sqrt(9.8 g / 1 block of chain)
    float slow = 0.6 + 0.4*sin(t*0.37 + phase);           // the swing swells and dies out
    vec2 own = vec2(sin(ring + phase), cos(ring*1.01 + phase))*slow;
    vec2 tilt = (sway*0.8 + own*0.45)*BR_WAVE_LANTERN*fade*sky;
    return brSwing(worldPos, worldPos - (bPos - vec3(0.5, 1.0, 0.5)), vec2(tilt.x, -tilt.y));
  }

  if (!isColored && !isCrop && !seasons) {
    return worldPos;
  }

  if (isLeaf) {
    return brBend(worldPos, swayAt, BR_WAVE_LEAVES*fade*sky);
  }
  if (isVine && isColored) {
    return brBend(worldPos, swayAt, 1.3*BR_WAVE_LEAVES*fade*sky);
  }
  if (isFlat || (!isGreen && isColored)) {
    return worldPos;
  }
  // grass, ferns, sugar cane, crops: the foot stays on the ground, the tip bends
  float tip = isTop ? 1.0 : 0.0;
  float amount = BR_WAVE_PLANTS*(isCrop ? 0.8 : 1.0)*tip*fade*sky;
  return brBend(worldPos, sway, amount);
}

#endif
