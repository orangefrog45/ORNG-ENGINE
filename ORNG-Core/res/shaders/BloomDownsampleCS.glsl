#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in; // invocations

layout(binding = 1, rgba16f) uniform image2D i_output;
layout(binding = 24) uniform sampler2D sampler_input;

uniform int u_mip_level;



void main() {
	ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy);
	vec2 local_tex_coords = tex_coords / vec2(imageSize(i_output));
	vec2 upsampled_texel_size = 1.0 / vec2(textureSize(sampler_input, u_mip_level - 1));

	vec3 A = textureLod(sampler_input, local_tex_coords + vec2(-2.0, -2.0) * upsampled_texel_size, u_mip_level - 1).rgb;
	vec3 B = textureLod(sampler_input, local_tex_coords + vec2(0.0, -2.0) * upsampled_texel_size, u_mip_level - 1).rgb;
	vec3 C = textureLod(sampler_input, local_tex_coords + vec2(2.0, -2.0) * upsampled_texel_size, u_mip_level - 1).rgb;
	vec3 D = textureLod(sampler_input, local_tex_coords + vec2(-1.0, -1.0) * upsampled_texel_size, u_mip_level - 1).rgb;
	vec3 E = textureLod(sampler_input, local_tex_coords + vec2(1.0, -1.0) * upsampled_texel_size, u_mip_level - 1).rgb;
	vec3 F = textureLod(sampler_input, local_tex_coords + vec2(-2.0, 0.0) * upsampled_texel_size, u_mip_level - 1).rgb;
	vec3 G = textureLod(sampler_input, local_tex_coords, u_mip_level - 1).rgb;
	vec3 H = textureLod(sampler_input, local_tex_coords + vec2(2.0, 0.0) * upsampled_texel_size, u_mip_level - 1).rgb;
	vec3 I = textureLod(sampler_input, local_tex_coords + vec2(-1.0, 1.0) * upsampled_texel_size, u_mip_level - 1).rgb;
	vec3 J = textureLod(sampler_input, local_tex_coords + vec2(1.0, 1.0) * upsampled_texel_size, u_mip_level - 1).rgb;
	vec3 K = textureLod(sampler_input, local_tex_coords + vec2(-2.0, 2.0) * upsampled_texel_size, u_mip_level - 1).rgb;
	vec3 L = textureLod(sampler_input, local_tex_coords + vec2(0.0, 2.0) * upsampled_texel_size, u_mip_level - 1).rgb;
	vec3 M = textureLod(sampler_input, local_tex_coords + vec2(2.0, 2.0) * upsampled_texel_size, u_mip_level - 1).rgb;
	




	vec2 div = (1.0 / 4.0) * vec2(0.5, 0.125);
	vec3 total_result = (D + E + I + J) * div.x;
	total_result += (A + B + G + F) * div.y;
	total_result += (B + C + H + G) * div.y;
	total_result += (F + G + L + K) * div.y;
	total_result += (G + H + M + L) * div.y;

	imageStore(i_output, tex_coords, vec4(total_result, 1));
}