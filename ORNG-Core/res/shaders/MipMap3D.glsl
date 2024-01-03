#version 460 core

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;


#ifdef ANISOTROPIC_MIPMAP
layout(binding = 0, rgba16f) readonly uniform image3D input_image;
layout(binding = 1, rgba16f) writeonly uniform image3D output_mips[6]; // [pos_x, pos_y, pos_z, neg_x, neg_y, neg_z]
#elif defined(ANISOTROPIC_MIPMAP_CHAIN)
layout(binding = 0, rgba16f) writeonly uniform image3D output_mips[6]; // [pos_x, pos_y, pos_z, neg_x, neg_y, neg_z]
layout(binding = 0) uniform sampler3D input_mips[6];

uniform int u_mip_level;
#else
layout(binding = 0, rgba16f) readonly uniform image3D input_image;
layout(binding = 1, rgba16f) writeonly uniform image3D output_image;
#endif


const ivec3 aniso_offsets[] = ivec3[8]
(
	ivec3(1, 1, 1),
	ivec3(1, 1, 0),
	ivec3(1, 0, 1),
	ivec3(1, 0, 0),
	ivec3(0, 1, 1),
	ivec3(0, 1, 0),
	ivec3(0, 0, 1),
	ivec3(0, 0, 0)
);

#ifdef ANISOTROPIC_MIPMAP_CHAIN
void FetchTexels(ivec3 start_coord, uint face_index, out vec4[8] values) {
    for (int i = 0; i < 8; i++) {
        values[i] = texelFetch(input_mips[face_index], ivec3(start_coord + aniso_offsets[i]), u_mip_level - 1);
    }
}

#endif
vec4 AlphaBlend(vec4 v0, vec4 v1) {
    return v0 + v1 * (1 - v0.a);
}

void main() {
    ivec3 sample_coord = ivec3(gl_GlobalInvocationID.xyz * 2);

#ifdef ANISOTROPIC_MIPMAP
    vec4 ooo = imageLoad(input_image, ivec3(sample_coord + ivec3(0, 0, 0)));

    vec4 ioo = imageLoad(input_image, ivec3(sample_coord + ivec3(1, 0, 0)));
    vec4 oio = imageLoad(input_image, ivec3(sample_coord + ivec3(0, 1, 0)));
    vec4 ooi = imageLoad(input_image, ivec3(sample_coord + ivec3(0, 0, 1)));

    vec4 iio = imageLoad(input_image, ivec3(sample_coord + ivec3(1, 1, 0)));
    vec4 oii = imageLoad(input_image, ivec3(sample_coord + ivec3(0, 1, 1)));
    vec4 ioi = imageLoad(input_image, ivec3(sample_coord + ivec3(1, 0, 1)));

    vec4 iii = imageLoad(input_image, ivec3(sample_coord + ivec3(1, 1, 1)));

    vec4 neg_x = (AlphaBlend(ioo, ooi) + AlphaBlend(iio, oii) + AlphaBlend(ioi, oio) + AlphaBlend(iii, ooo)) * 0.25;
    vec4 neg_y = (AlphaBlend(oio,oio ) + AlphaBlend(iio,oio ) + AlphaBlend(oii,ooo ) + AlphaBlend(iii,ooo )) * 0.25;
    vec4 neg_z = (AlphaBlend(ooi,ooi ) + AlphaBlend(oii,ooo ) + AlphaBlend(ioi,ooi ) + AlphaBlend(iii,ooo )) * 0.25;
    vec4 pos_x = (AlphaBlend(ooi,ooi ) + AlphaBlend(oii,ooo ) + AlphaBlend(oio,oio ) + AlphaBlend(ooo,oii )) * 0.25;
    vec4 pos_y = (AlphaBlend(ioi,ooi ) + AlphaBlend(ioo,oii ) + AlphaBlend(ooi,ooi ) + AlphaBlend(ooo,oii )) * 0.25;
    vec4 pos_z = (AlphaBlend(ioo,oii ) + AlphaBlend(iio,oio ) + AlphaBlend(oio,oio ) + AlphaBlend(ooo,oii )) * 0.25;

    imageStore(output_mips[0], ivec3(gl_GlobalInvocationID.xyz), pos_x);
    imageStore(output_mips[1], ivec3(gl_GlobalInvocationID.xyz), pos_y);
    imageStore(output_mips[2], ivec3(gl_GlobalInvocationID.xyz), pos_z);

    imageStore(output_mips[3], ivec3(gl_GlobalInvocationID.xyz), neg_x);
    imageStore(output_mips[4], ivec3(gl_GlobalInvocationID.xyz), neg_y);
    imageStore(output_mips[5], ivec3(gl_GlobalInvocationID.xyz), neg_z);

#elif defined(ANISOTROPIC_MIPMAP_CHAIN)
    vec4 values[8];

    // X+
    FetchTexels(sample_coord, 0, values);
    imageStore(output_mips[0], ivec3(gl_GlobalInvocationID.xyz), 
        (
        AlphaBlend(values[4], values[0]) + 
        AlphaBlend(values[5], values[1]) + 
        AlphaBlend(values[6], values[2]) + 
        AlphaBlend(values[7], values[3])
        ) * 0.25
    );

    // Y+
    FetchTexels(sample_coord, 1, values);
    imageStore(output_mips[1], ivec3(gl_GlobalInvocationID.xyz), 
        (
        AlphaBlend(values[2], values[0]) + 
        AlphaBlend(values[3], values[1]) + 
        AlphaBlend(values[7], values[5]) + 
        AlphaBlend(values[6], values[4])
        ) * 0.25
    );

    // Z+
    FetchTexels(sample_coord, 2, values);
    imageStore(output_mips[2], ivec3(gl_GlobalInvocationID.xyz), 
        (
        AlphaBlend(values[1], values[0]) + 
        AlphaBlend(values[3], values[2]) + 
        AlphaBlend(values[5], values[4]) + 
        AlphaBlend(values[7], values[6])
        ) * 0.25
    );

    // X-
    FetchTexels(sample_coord, 3, values);
    imageStore(output_mips[3], ivec3(gl_GlobalInvocationID.xyz), 
        (
        AlphaBlend(values[0], values[4]) + 
        AlphaBlend(values[1], values[5]) + 
        AlphaBlend(values[2], values[6]) + 
        AlphaBlend(values[3], values[7])
        ) * 0.25
    );

    // Y-
    FetchTexels(sample_coord, 4, values);
    imageStore(output_mips[4], ivec3(gl_GlobalInvocationID.xyz), 
        (
        AlphaBlend(values[0], values[2]) + 
        AlphaBlend(values[1], values[3]) + 
        AlphaBlend(values[5], values[7]) + 
        AlphaBlend(values[4], values[6])
        ) * 0.25
    );

    // Z-
    FetchTexels(sample_coord, 5, values);
    imageStore(output_mips[5], ivec3(gl_GlobalInvocationID.xyz), 
        (
        AlphaBlend(values[0], values[1]) + 
        AlphaBlend(values[2], values[3]) + 
        AlphaBlend(values[4], values[5]) + 
        AlphaBlend(values[6], values[7])
        ) * 0.25
    );


#else
    vec4 accum = vec4(0);
    float sum = 0.f;

    for (int z = 0; z <= 1; ++z) {
        for (int y = 0; y <= 1; ++y) {
            for (int x = 0; x <= 1; ++x) {
                accum += imageLoad(input_image, ivec3(sample_coord + ivec3(x, y, z)));
                sum++;
            }
        }
    }

    imageStore(output_image, ivec3(gl_GlobalInvocationID.xyz), accum / sum);

#endif
}