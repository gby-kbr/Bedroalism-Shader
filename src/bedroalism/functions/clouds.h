#ifndef BR_CLOUDS_H
#define BR_CLOUDS_H

#include "sky.h"

// Clouds are built from one tileable noise texture (RGBA, see tools/generate_cloud_noise.py):
// R, G, B, A hold 4 octaves of noise from low to high frequency, so 1 fetch = 4 octaves.
//
// The cumulus layer is a slab of fixed thickness above the camera. The view ray is marched through the slab
// in BR_CLOUD_STEPS steps; every column of the noise is a flat-based, round-topped lump whose height comes
// from the low octaves, while the high octaves roughen its surface into billows.

// interleaved gradient noise (Jimenez 2014) in [0.0, 1.0), offsets the march steps per pixel
float brCloudJitter(vec2 fragCoord) {
  return fract(52.9829189*fract(dot(fragCoord, vec2(0.06711056, 0.00583715))));
}

// direct light: white by day, warm at sunrise and sunset, dark at night, grey and dimmer when it rains
vec3 brCloudLightColor(brEnvironment env) {
  vec3 sunCol = mix(vec3(1.15, 1.12, 1.05), vec3(1.5, 0.55, 0.25), env.twilight);
  sunCol = mix(sunCol, vec3(0.03, 0.035, 0.05), env.night);
  float grey = dot(sunCol, vec3(0.333, 0.333, 0.333));
  return mix(sunCol, vec3(grey, grey, grey)*0.6, env.rainFactor);
}

// light from the sky itself (blue by day, rose at dusk) plus a little bounce from the ground
vec3 brCloudAmbientColor(brEnvironment env, brSkyColor sky) {
  vec3 bounce = vec3(0.05, 0.045, 0.04)*((1.0 - env.night)*(0.3 + 0.7*env.day));
  return (sky.zenith*0.45 + sky.horizon*0.55)*1.05 + bounce;
}

// distant clouds take the color of the air in between
vec3 brCloudAerial(vec3 col, vec3 horizon, float elevation, float amount) {
  float w = amount*(1.0 - smoothstep(0.0, 0.45, elevation));
  return mix(col, horizon*0.9, w);
}

// cumulus slab. returns color and opacity
vec4 brCloudSlab(sampler2D noiseTex, vec3 viewDir, vec3 sunDir, vec3 sunCol, vec3 amb, float coverage, vec2 wind, float jitter) {
  float th = 1.0 - coverage;
  float k = 0.30;                                       // how fast a lump grows from its edge
  float y0 = 1.0;                                       // slab base
  float y1 = 1.0 + BR_CLOUD_THICKNESS;                  // slab top
  float dy = max(viewDir.y, 0.03);
  vec2 xz = vec2(viewDir.x, viewDir.z)/dy*BR_CLOUD_SCALE;
  float sdn = clamp(dot(viewDir, sunDir), 0.0, 1.0);

  // one sample on the sun side at mid height: when that neighbour is taller than this point, it shades it
  vec2 sunXZ = vec2(sunDir.x, sunDir.z);
  sunXZ = sunXZ/max(length(sunXZ), 0.001);
  vec4 cs = texture2D(noiseTex, xz*(0.5*(y0 + y1)) + wind + sunXZ*0.035);
  float Hs = pow(clamp((dot(cs, vec4(0.40, 0.30, 0.19, 0.11)) - th)/k, 0.0, 1.0), 0.8);

  float steps = float(BR_CLOUD_STEPS);
  float ds = (y1 - y0)/steps/dy;                        // path length per step, longer near the horizon
  float fwd = pow(sdn, 8.0);                            // silver lining toward the sun
  float T = 1.0;
  vec3 col = vec3(0.0, 0.0, 0.0);

  for (int i = 0; i < BR_CLOUD_STEPS; i++) {
    float hf = (float(i) + jitter)/steps;               // height inside the slab, 0.0 base ~ 1.0 top
    float y = y0 + (y1 - y0)*hf;
    vec4 c = texture2D(noiseTex, xz*y + wind);
    float low = c.r*0.55 + c.g*0.45;                    // lump
    float det = c.b*0.45 + c.a*0.55;                    // billows
    float Hc = pow(clamp((low - th)/k, 0.0, 1.0), 0.8); // column height, rounded top
    float edge = Hc + 0.62*(det - 0.5)*smoothstep(0.0, 0.35, Hc);
    float occ = smoothstep(0.0, 0.20, edge - hf);       // flat base, soft round top
    float depth = clamp((edge - hf)/0.45, 0.0, 1.0);    // 0.0 at the surface ~ 1.0 deep inside
    float a = occ*(1.0 - exp(-ds*4.2));

    float shadow = clamp(1.0 - (Hs - hf)*2.4, 0.12, 1.0);
    float top = pow(clamp(hf, 0.0, 1.0), 0.7);
    vec3 ambient = amb*((0.55 + 0.35*top)*0.95) + sunCol*(0.20*(1.0 - 0.5*depth));
    vec3 direct = sunCol*((0.30 + 0.70*top)*(1.0 - 0.45*depth*(1.0 - top)));
    vec3 c3 = ambient + direct*(shadow*0.95) + sunCol*(fwd*(1.0 - occ*0.5)*0.5*top);

    col += c3*(T*a);
    T *= 1.0 - a;
  }

  float alpha = (1.0 - T)*smoothstep(0.03, 0.30, viewDir.y);
  return vec4(col/max(1.0 - T, 0.0001), alpha);
}

// thin high streaks, 1 fetch
vec4 brCirrus(sampler2D noiseTex, vec3 viewDir, vec3 sunCol, vec2 wind, float amount) {
  float dy = max(viewDir.y, 0.02);
  vec2 uv = vec2(viewDir.x, viewDir.z)/dy*0.10 + vec2(0.3, 0.1) + wind*1.5;
  vec4 c = texture2D(noiseTex, vec2(uv.x*0.5, uv.y*1.8));
  float n = c.g*0.55 + c.a*0.45;
  float a = smoothstep(0.55, 0.85, n)*amount*smoothstep(0.06, 0.40, viewDir.y);
  return vec4(sunCol*0.92, a);
}

// color and opacity of all clouds for a view direction, to be mixed over the sky color:
// sky = mix(sky, clouds.rgb, clouds.a)
// viewDir: normalized, y is up. time: seconds. fragCoord: pixel position
vec4 brClouds(sampler2D noiseTex, vec3 viewDir, brEnvironment env, brSkyColor sky, float time, vec2 fragCoord) {
  vec3 sunCol = brCloudLightColor(env);
  vec3 amb = brCloudAmbientColor(env, sky);
  float coverage = min(BR_CLOUD_COVERAGE + 0.40*env.rainFactor, 1.0);
  vec2 wind = vec2(0.02, 0.0) + vec2(0.94, 0.34)*(BR_CLOUD_WIND_SPEED*time);

  vec4 cumulus = brCloudSlab(noiseTex, viewDir, env.sunDir, sunCol, amb, coverage, wind, 0.5 + 0.5*brCloudJitter(fragCoord));
  vec3 cumulusCol = brCloudAerial(cumulus.rgb, sky.horizon, viewDir.y, 0.5);

  vec4 cirrus = vec4(0.0, 0.0, 0.0, 0.0);
  if (BR_CIRRUS > 0.0) {
    cirrus = brCirrus(noiseTex, viewDir, sunCol, wind, BR_CIRRUS*(1.0 - env.rainFactor));
  }

  // cumulus in front of the cirrus
  float alpha = cumulus.a + cirrus.a*(1.0 - cumulus.a);
  vec3 col = (cumulusCol*cumulus.a + cirrus.rgb*(cirrus.a*(1.0 - cumulus.a)))/max(alpha, 0.0001);
  return vec4(col, alpha);
}

#endif
