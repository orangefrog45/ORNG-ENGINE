#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in; // invocations

layout(binding = 0, rgba16f) writeonly uniform image2D i_output;
layout(binding = 1) uniform sampler2D sampler_input;

uniform float u_threshold;
uniform float u_knee;

//https://github.com/Unity-Technologies/Graphics/blob/1e60cd5a0d89c55a887a1bff8344c4aea36a73ea/Packages/com.unity.render-pipelines.core/Runtime/Utilities/ColorUtils.cs#L203
float Luminance(vec3 colour) {
	return (colour.r * 0.2126729f + colour.g * 0.7151522f + colour.b * 0.072175f);
}

vec4 karis_avg(vec4 colour) {
	float w = 1.0 / (Luminance(colour.rgb) + 1.0);
	vec3 new_colour = colour.rgb * w;
	return vec4(new_colour, w);
}


void main() {

	ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy);
	vec2 local_tex_coords = tex_coords / vec2(imageSize(i_output));
	vec2 upsampled_texel_size = 1.0 / vec2(textureSize(sampler_input, 0));

	vec4 sum = vec4(0);
	vec4 A = textureLod(sampler_input, local_tex_coords + vec2(-2.0, -2.0) * upsampled_texel_size, 0);
	vec4 B = textureLod(sampler_input, local_tex_coords + vec2(0.0, -2.0) * upsampled_texel_size, 0);
	vec4 C = textureLod(sampler_input, local_tex_coords + vec2(-2.0, 2.0) * upsampled_texel_size, 0);
	vec4 F = textureLod(sampler_input, local_tex_coords + vec2(-2.0, 0.0) * upsampled_texel_size, 0);
	vec4 G = textureLod(sampler_input, local_tex_coords, 0);
	vec4 H = textureLod(sampler_input, local_tex_coords + vec2(2.0, 0.0) * upsampled_texel_size, 0);
	vec4 K = textureLod(sampler_input, local_tex_coords + vec2(2.0, -2.0) * upsampled_texel_size, 0);
	vec4 L = textureLod(sampler_input, local_tex_coords + vec2(0.0, 2.0) * upsampled_texel_size, 0);
	vec4 M = textureLod(sampler_input, local_tex_coords + vec2(2.0, 2.0) * upsampled_texel_size, 0);

	vec4 D = (A + B + G + F) * 0.25;
	vec4 E = (B + C + H + G) * 0.25;
	vec4 I = (F + G + L + K) * 0.25;
	vec4 J = (G + H + M + L) * 0.25;


	vec2 div = (1.0 / 4.0) * vec2(0.5, 0.125);
	vec4 c = karis_avg((D + E + I + J) * div.x);
	c += karis_avg((A + B + G + F) * div.y);
	c += karis_avg((B + C + H + G) * div.y);
	c += karis_avg((F + G + L + K) * div.y);
	c += karis_avg((G + H + M + L) * div.y);

	//vec3 colour = texture(sampler_input, local_tex_coords);
	//total_result.r *= int(total_result.r > threshold) + clamp(threshold - total_result.r, 0.0, knee);
	//total_result.g *= int(total_result.g > threshold) + clamp(threshold - total_result.g, 0.0, knee);
	//total_result.b *= int(total_result.b > threshold) + clamp(threshold - total_result.b, 0.0, knee);

	float brightness = max(max(c.r, c.g), c.b);
	float contribution = max(brightness - u_threshold, 0.0) / max(brightness, 0.00001);
	c *= contribution;

	imageStore(i_output, tex_coords, c);
}