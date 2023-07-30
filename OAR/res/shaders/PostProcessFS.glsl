R""(#version 430 core

out vec4 FragColor;

in vec2 tex_coords;

layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 1) uniform sampler2D quad_sampler;
layout(binding = 14) uniform sampler2D fog_overlay_sampler;
layout(binding = 24) uniform sampler2D bloom_sampler;

uniform bool u_show_depth_map;
uniform float exposure;
uniform float u_bloom_intensity;


void main() {

	float gamma = 2.2;

	vec4 sampled_fog_color = texture(fog_overlay_sampler, tex_coords);
	vec3 fog_mix_color = mix(texture(quad_sampler, tex_coords).rgb, sampled_fog_color.rgb, clamp(sampled_fog_color.w, 0.f, 1.f));
	fog_mix_color += texture(bloom_sampler, tex_coords).rgb * u_bloom_intensity;
	vec3 mapped = vec3(1.0) - exp(-fog_mix_color * exposure);

	FragColor = vec4(pow(mapped, vec3(1.0 / gamma)), 1.0);

})""