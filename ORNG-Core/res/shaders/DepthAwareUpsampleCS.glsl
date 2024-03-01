#version 460 core
#define PI 3.1415926538

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in; // invocations

layout(binding = 1, rgba16f) uniform image2D out_tex;
layout(binding = 16) uniform sampler2D depth_sampler;
layout(binding = 23) uniform sampler2D in_tex; // depends on ping pong fb currently bound

#ifdef CONE_TRACE_UPSAMPLE
layout(binding = 7) uniform sampler2D normal_sampler;
#endif


void main() {
	const int KERNEL_SIZE = 8;
	ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy);

	vec4 result = vec4(0);
	float sum = 0;

	float original_depth = texelFetch(depth_sampler, tex_coords, 0).r;

#ifdef CONE_TRACE_UPSAMPLE
	vec3 original_normal = texelFetch(normal_sampler, tex_coords, 0).xyz;
#endif


    for (int x = 0; x <= 1; x++) {
        for (int y = 0; y <= 1; y++) {
            // Sample value from the half-resolution input texture
            vec4 sampled_offset = texture(in_tex, vec2((tex_coords + vec2(x, y) * 0.5)) * 0.5 / textureSize(in_tex, 0));
            float depth_dif = abs(texelFetch(depth_sampler, tex_coords + ivec2(x, y), 0).r - original_depth);

#ifdef CONE_TRACE_UPSAMPLE
            float dot_n = dot(texelFetch(normal_sampler, tex_coords + ivec2(x, y), 0).xyz, original_normal);

            if (depth_dif < 0.001 ) {
                result += sampled_offset;
                sum += 1.0 ;
            }
#else
            if (depth_dif < 0.001) {
                result += sampled_offset;
                sum += 1.0 ;
            }
#endif
        }
    }


#ifdef CONE_TRACE_UPSAMPLE // Composite here (luminance written directly to the main render texture)
	vec4 original = imageLoad(out_tex, tex_coords);
	imageStore(out_tex, tex_coords, original + result * (1.0 / max(sum, 0.0000001f)));
#else // Store for later composition (fog uses this as it needs blur passes before)
	imageStore(out_tex, tex_coords, result * (1.0 / max(sum, 0.0000001f)));
#endif
}