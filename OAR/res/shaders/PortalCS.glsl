R""(#version 460 core
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(binding = 1, rgba16f) writeonly uniform image2D u_output_texture;
layout(binding = 12) uniform usampler2D shader_id_sampler;
layout(binding = 14) uniform sampler2D portal_input_sampler;



void main() {
ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy);

    if (texelFetch(shader_id_sampler, tex_coords, 0).r == 2) {
        imageStore(u_output_texture, tex_coords, vec4(texelFetch(portal_input_sampler, tex_coords, 0).rgb, 1.f));
    }
})""