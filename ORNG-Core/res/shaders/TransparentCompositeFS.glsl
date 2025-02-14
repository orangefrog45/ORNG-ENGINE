#version 460 core
layout (binding = 0) uniform sampler2D accum_sampler;
layout (binding = 1) uniform sampler2D reveal_sampler;

in vec2 tex_coords;
out vec4 colour;

const float EPSILON = 0.00001f;

ORNG_INCLUDE "UtilINCL.glsl"


void main() {
    ivec2 render_res = textureSize(accum_sampler, 0);

    float reveal = texelFetch(reveal_sampler, ivec2(render_res.x * tex_coords.x, render_res.y * tex_coords.y), 0).r;

    if (isApproximatelyEqual(reveal, 1.0f))
        discard;


    vec4 accum = texelFetch(accum_sampler,ivec2(render_res.x * tex_coords.x, render_res.y * tex_coords.y), 0);

   if (isinf(max3(abs(accum.rgb))) )
       {
			accum.rgb = vec3(accum.a);
        }

    vec3 average_colour = accum.rgb / max(accum.a, EPSILON);

    colour = vec4(average_colour, 1.0 - reveal);
}