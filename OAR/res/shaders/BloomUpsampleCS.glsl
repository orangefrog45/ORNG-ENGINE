R""(#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in; // invocations

layout(binding = 1, rgba16f) uniform image2D i_output;
layout(binding = 24) uniform sampler2D sampler_input;

uniform int u_mip_level; // Current mip level being upsampled to


void main() {
	ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy);
	vec2 normalized_tex_coords = vec2(tex_coords) / vec2(imageSize(i_output)); // tex coords in range 0-1

	int mip_sampling_level = u_mip_level + 1;
	vec2 downsampled_texel_size = 1.0 / vec2(textureSize(sampler_input, mip_sampling_level));
	normalized_tex_coords += vec2(0.5, 0.5) * downsampled_texel_size;

	float texel_offset = mip_sampling_level * 0.5;

	vec3 result = vec3(0);
	result += textureLod(sampler_input, normalized_tex_coords + vec2(-1.0, -1.0) * downsampled_texel_size * texel_offset, mip_sampling_level).rgb;
	result += textureLod(sampler_input, normalized_tex_coords + vec2(0.0, -1.0) * downsampled_texel_size * texel_offset, mip_sampling_level).rgb * 2.0;
	result += textureLod(sampler_input, normalized_tex_coords + vec2(1.0, -1.0) * downsampled_texel_size * texel_offset, mip_sampling_level).rgb;

	result += textureLod(sampler_input, normalized_tex_coords + vec2(-1.0, 0.0) * downsampled_texel_size * texel_offset, mip_sampling_level).rgb * 2.0;
	result += textureLod(sampler_input, normalized_tex_coords, mip_sampling_level).rgb * 4.0;
	result += textureLod(sampler_input, normalized_tex_coords + vec2(1.0, 0.0) * downsampled_texel_size * texel_offset, mip_sampling_level).rgb * 2.0;

	result += textureLod(sampler_input, normalized_tex_coords + vec2(-1.0, 1.0) * downsampled_texel_size * texel_offset, mip_sampling_level).rgb;
	result += textureLod(sampler_input, normalized_tex_coords + vec2(0.0, 1.0) * downsampled_texel_size * texel_offset, mip_sampling_level).rgb * 2.0;
	result += textureLod(sampler_input, normalized_tex_coords + vec2(1.0, 1.0) * downsampled_texel_size * texel_offset, mip_sampling_level).rgb;

	result *= (1.0 / 16.0);

	// Combine with current mip
	result += imageLoad(i_output, tex_coords).rgb;
	imageStore(i_output, tex_coords, vec4(result, 1.0));

})""