vec4 a_color0    : COLOR0;
vec3 a_position  : POSITION;
vec2 a_texcoord0 : TEXCOORD0;

vec4 i_data1 : TEXCOORD7;
vec4 i_data2 : TEXCOORD6;
vec4 i_data3 : TEXCOORD5;

vec4  v_color0          : COLOR0;
vec4  v_fog             : COLOR1;
float v_occlusionHeight : TEXCOORD2;
vec2  v_occlusionUV     : TEXCOORD1;
vec2  v_texcoord0       : TEXCOORD0;
vec3  v_worldPos        : TEXCOORD3;
vec3  v_local           : TEXCOORD4; // xy: position in the quad, z: 1.0 for rain, 0.0 for snow
vec4  v_fogMul          : TEXCOORD5; // fog: color*mul + add
vec4  v_fogAdd          : TEXCOORD6;
