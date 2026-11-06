$input v_color0, v_texcoord0, v_worldPos

#include <bgfx_shader.sh>
#include <bedroalism/config.h>
#include <bedroalism/functions/detection.h>
#include <bedroalism/functions/colorcorrection.h>

SAMPLER2D_AUTOREG(s_SunMoonTexture);
uniform vec4 SunMoonColor;
uniform vec4 FogColor;
uniform vec4 FogAndDistanceControl;

void main() {
    // under water the Sky material draws the sun and moon where the water surface bends them
    if (brDetectUnderwater(FogColor.rgb, FogAndDistanceControl.xy)) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    vec4 color = SunMoonColor * texture2D(s_SunMoonTexture, v_texcoord0);
    color.rgb = brColorCorrection(color.rgb);
    gl_FragColor = color;
}
