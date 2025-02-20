#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;


layout(binding = 24) uniform sampler2D bloom_sampler;
layout(binding = 1) uniform sampler2D input_sampler;
layout(binding = 1, rgba16f) uniform image2D u_output_texture;


uniform float u_bloom_intensity;

void main() {
    ivec2 render_size = textureSize(input_sampler, 0);
	ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy);

    if (tex_coords.x < 0 || tex_coords.x >= render_size.x || tex_coords.y < 0 || tex_coords.y >= render_size.y) return;
    
	vec3 bloom_col = texture(bloom_sampler, (tex_coords * 0.5) / vec2(textureSize(bloom_sampler, 0)), 0).rgb * u_bloom_intensity;
    vec3 input_col = texelFetch(input_sampler, tex_coords, 0).rgb;
    vec3 composite_col = bloom_col + input_col;

	imageStore(u_output_texture, tex_coords, vec4(composite_col, 1.0));
}