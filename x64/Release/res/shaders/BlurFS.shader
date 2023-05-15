#version 430 core

out vec4 FragColor;
in vec2 tex_coords; //uses QuadVS as vertex shader
layout(binding = 14) uniform sampler2D fog_overlay_sampler;

uniform bool u_horizontal;
uniform float weight[5] = float[](0.13533528, 0.13242822, 0.11905037, 0.09941364, 0.07774728);

void main() {

	vec2 tex_offset = 1.0 / textureSize(fog_overlay_sampler, 0); // gets size of single texel
	vec3 result = texture(fog_overlay_sampler, tex_coords).rgb * weight[0]; // current fragment's contribution
	if (u_horizontal)
	{
		for (int i = 1; i < 5; ++i)
		{
			result += texture(fog_overlay_sampler, tex_coords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
			result += texture(fog_overlay_sampler, tex_coords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
		}
	}
	else
	{
		for (int i = 1; i < 5; ++i)
		{
			result += texture(fog_overlay_sampler, tex_coords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
			result += texture(fog_overlay_sampler, tex_coords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
		}
	}
	FragColor = vec4(result, texture(fog_overlay_sampler, tex_coords).w);
}