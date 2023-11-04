#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;


layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 14) uniform sampler2D fog_overlay_sampler;
layout(binding = 24) uniform sampler2D bloom_sampler;
layout(binding = 1, rgba16f) uniform image2D u_output_texture;


uniform bool u_show_depth_map;
uniform float exposure;
uniform float u_bloom_intensity;


void main() {
	ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy);
	float gamma = 2.2;

	vec4 sampled_fog_color = texelFetch(fog_overlay_sampler, tex_coords, 0);
	vec3 fog_mix_color = mix(imageLoad(u_output_texture, tex_coords).rgb, sampled_fog_color.rgb, clamp(sampled_fog_color.w, 0.f, 1.f));
	fog_mix_color += texelFetch(bloom_sampler, tex_coords / 2, 0).rgb * u_bloom_intensity;
	vec3 mapped = vec3(1.0) - exp(-fog_mix_color * exposure);
	
	vec3 gamma_adjusted = pow(mapped, vec3(1.0 / gamma));
	imageStore(u_output_texture, tex_coords, vec4(gamma_adjusted, 1.0));

}