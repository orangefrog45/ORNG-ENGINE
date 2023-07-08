#version 430 core
#define PI 3.1415926538

out vec4 FragColor;
in vec2 tex_coords; //uses QuadVS as vertex shader
layout(binding = 14) uniform sampler2D fog_overlay_sampler;
layout(binding = 16) uniform sampler2D depth_sampler;
layout(binding = 23) uniform sampler2D fog_overlay_sampler_2; // depends on ping pong fb currently bound

uniform bool u_horizontal;
uniform bool u_first_iter;

float gauss(float x, float y, float sigma)
{
	return  1.0f / (2.0f * PI * sigma * sigma) * exp(-(x * x + y * y) / (2.0f * sigma * sigma));
}


void main() {
	const int KERNEL_SIZE = 6;
	vec2 fog_tex_size = textureSize(fog_overlay_sampler_2, 0);


	vec4 result = vec4(0);
	float sum = 0;

	float original_depth = texture(depth_sampler, tex_coords).r;

	if (u_first_iter) // If first iteration, upsampling has to be done using these two loops to remove edge artifacts in one go, after this a regular ping-pong gaussian blur works fine.
	{
		for (int x = -4; x < 4; x++)
		{
			for (int y = -4; y < 4; y++) {

				ivec2 downscaled_coords = ivec2(tex_coords * fog_tex_size) + ivec2(x, y);
				vec4 sampled_offset = texelFetch(fog_overlay_sampler_2, downscaled_coords, 0);
				float density_dif = abs(texelFetch(depth_sampler, downscaled_coords * 2, 0).r - original_depth);

				float g_weight_1 = max(0.0, 1.0 - density_dif * 30000.f);
				result += sampled_offset * g_weight_1;

				sum += g_weight_1;
			}
		}
	}
	else if (u_horizontal)
	{
		for (int i = -KERNEL_SIZE / 2; i < KERNEL_SIZE / 2; i++) {

			ivec2 downscaled_coords = ivec2(tex_coords * fog_tex_size) + ivec2(i, 0);
			float g_weight_1 = 1;
			vec4 sampled_offset = texelFetch(fog_overlay_sampler_2, downscaled_coords, 0);
			float density_dif = abs(texelFetch(depth_sampler, ivec2(downscaled_coords), 0).r - original_depth);

			g_weight_1 *= max(0.0, 1.0 - density_dif * 30000.f);
			result += sampled_offset * g_weight_1;

			sum += g_weight_1;
		}
	}
	else {
		for (int i = -KERNEL_SIZE / 2; i < KERNEL_SIZE / 2; i++)
		{
			ivec2 downscaled_coords = ivec2(tex_coords * fog_tex_size) + ivec2(0, i);
			float g_weight_1 = 1;
			vec4 sampled_offset = texelFetch(fog_overlay_sampler, downscaled_coords, 0);
			float density_dif = abs(texelFetch(depth_sampler, ivec2(downscaled_coords), 0).r - original_depth);

			g_weight_1 *= max(0.0f, 1.0 - density_dif * 30000.f);
			result += sampled_offset * g_weight_1;

			sum += g_weight_1;
		}
	}

	FragColor = result * (1.0 / max(sum, 0.0000001f));
}