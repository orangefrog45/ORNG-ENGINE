#version 460 core

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout(binding = 0, rgba8) readonly uniform image3D input_image;
layout(binding = 1, rgba8) writeonly uniform image3D output_image;


uniform uint u_mip_level;

void main() {
    vec3 sample_coord = gl_GlobalInvocationID.xyz * 2;
    vec4 accum = vec4(0);

    for (int z = 0; z <= 1; ++z) {
        for (int y = 0; y <= 1; ++y) {
            for (int x = 0; x <= 1; ++x) {
                accum += imageLoad(input_image, ivec3(sample_coord + ivec3(x, y, z)));
            }
        }
    }

    imageStore(output_image, ivec3(gl_GlobalInvocationID.xyz), accum / 8.0);
}