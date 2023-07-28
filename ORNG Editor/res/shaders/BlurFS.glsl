#version 460 core
#define PI 3.1415926538

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in; // invocations

layout(binding = 1, rgba16f) writeonly uniform image2D out_tex;
layout(binding = 16) uniform sampler2D depth_sampler;
layout(binding = 23) uniform sampler2D in_tex; // depends on ping pong fb currently bound

uniform bool u_horizontal;
uniform bool u_first_iter;

float gauss(float x, float y, float sigma)
{
	return  1.0f / (2.0f * PI * sigma * sigma) * exp(-(x * x + y * y) / (2.0f * sigma * sigma));
}




void main() {
	const int KERNEL_SIZE = 6;
	ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy);

	vec4 result = vec4(0);
	float sum = 0;

	float original_depth = texelFetch(depth_sampler, tex_coords, 0).r;

	if (u_first_iter) // If first iteration, upsampling has to be done using these two loops to remove edge artifacts in one go, after this a regular ping-pong gaussian blur works fine.
	{
		for (int x = -1; x < 1; x++)
		{
			for (int y = -1; y < 1; y++) {

				ivec2 offset_coords = tex_coords + ivec2(x, y);
				vec4 sampled_offset = texelFetch(in_tex, offset_coords / 2, 0); // first tex will be raw fog input at half res, so adjust
				float density_dif = abs(texelFetch(depth_sampler, offset_coords, 0).r - original_depth);

				float g_weight_1 = max(0.0, 1.0 - density_dif * 10000.f);
				result += sampled_offset * g_weight_1;

				sum += g_weight_1;
			}
		}
	}
	else
	{
		for (int i = -KERNEL_SIZE / 2; i < KERNEL_SIZE / 2; i++) {

			ivec2 offset_coords = tex_coords + ivec2(i * int(u_horizontal), i * int(!u_horizontal)); // If u_horizontal, will sample horizontally (i * 1), vice versa for vertical
			float g_weight_1 = 1.0;
			vec4 sampled_offset = texelFetch(in_tex, offset_coords, 0);
			float density_dif = abs(texelFetch(depth_sampler, offset_coords, 0).r - original_depth);

			g_weight_1 *= max(0.0, 1.0 - density_dif * 10000.f);
			result += sampled_offset * g_weight_1;

			sum += g_weight_1;
		}
	}


	imageStore(out_tex, tex_coords, result * (1.0 / max(sum, 0.0000001f)));

}