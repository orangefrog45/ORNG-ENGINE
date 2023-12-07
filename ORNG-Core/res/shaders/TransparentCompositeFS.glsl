#version 460 core
layout (binding = 0) uniform sampler2D accum_sampler;
layout (binding = 1) uniform sampler2D reveal_sampler;

in vec2 tex_coords;
out vec4 color;

const float EPSILON = 0.00001f;

ORNG_INCLUDE "UtilINCL.glsl"


void main() {
    float reveal = texture(reveal_sampler, tex_coords.xy).r;

    if (isApproximatelyEqual(reveal, 1.0f))
        discard;


    vec4 accum = texture(accum_sampler, tex_coords.xy);

   if (isinf(max3(abs(accum.rgb))) )
       {
			accum.rgb = vec3(accum.a);
        }

    vec3 average_colour = accum.rgb / max(accum.a, EPSILON);

    color = vec4(average_colour, 1.0 - reveal);
}