#version 460 core

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;


#ifdef ANISOTROPIC_MIPMAP
layout(binding = 0, r32ui) readonly uniform uimage3D input_image;
layout(binding = 1, rgba16f) writeonly uniform image3D output_mips; // [pos_x, pos_y, pos_z, neg_x, neg_y, neg_z], each face starts at z = face_index * 128
layout(binding = 2, r32ui) readonly uniform uimage3D input_normals;
#elif defined(ANISOTROPIC_MIPMAP_CHAIN)
layout(binding = 0, rgba16f) writeonly uniform image3D output_mips; // [pos_x, pos_y, pos_z, neg_x, neg_y, neg_z], each face starts at z = face_index * 128
layout(binding = 0) uniform sampler3D input_mips;

uniform int u_mip_level;
#else
layout(binding = 0, rgba16f) readonly uniform image3D input_image;
layout(binding = 1, rgba16f) writeonly uniform image3D output_image;
#endif

ORNG_INCLUDE "VoxelCommonINCL.glsl"

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

float FetchTexels(ivec3 start_coord, uint face_index, out vec4[8] values, uint mip_dim) {
    float sum = 0.0;
    ivec3 octant = ivec3(start_coord / (mip_dim / 2));

    for (int i = 0; i < 8; i++) {
        ivec3 local_coord = ivec3(start_coord + aniso_offsets[i]);
        ivec3 coord = ApplyMipFaceTexCoordOffset(local_coord, face_index, mip_dim);
        ivec3 current_octant = ivec3(local_coord / (mip_dim / 2));
        values[i] = texelFetch(input_mips, coord, u_mip_level - 1)  * float(face_index == coord.z / mip_dim);
        sum += float(face_index == coord.z / mip_dim );
    }
    return 8.0;
}

#endif


vec4 AlphaBlend(vec4 v0, vec4 v1) {
    return v0 + v1 * (1.0 - v0.a);
}


vec4 Average8(vec4 values[8]) {
    vec4 val = vec4(0);
    for (int i = 0; i < 8; i++) {
        val += values[i];
    }

    return val / 8.0;
}


void main() {
ivec3 sample_coord = ivec3(gl_GlobalInvocationID.xyz * 2);

#ifdef ANISOTROPIC_MIPMAP
    vec4 values[8] = {
        convRGBA8ToVec4(imageLoad(input_image, ivec3(sample_coord + ivec3(1, 1, 1))).r) / 255.,
        convRGBA8ToVec4(imageLoad(input_image, ivec3(sample_coord + ivec3(1, 1, 0))).r) / 255.,
        convRGBA8ToVec4(imageLoad(input_image, ivec3(sample_coord + ivec3(1, 0, 1))).r) / 255.,
        convRGBA8ToVec4(imageLoad(input_image, ivec3(sample_coord + ivec3(1, 0, 0))).r) / 255.,
        convRGBA8ToVec4(imageLoad(input_image, ivec3(sample_coord + ivec3(0, 1, 1))).r) / 255.,
        convRGBA8ToVec4(imageLoad(input_image, ivec3(sample_coord + ivec3(0, 1, 0))).r) / 255.,
        convRGBA8ToVec4(imageLoad(input_image, ivec3(sample_coord + ivec3(0, 0, 1))).r) / 255.,
        convRGBA8ToVec4(imageLoad(input_image, ivec3(sample_coord + ivec3(0, 0, 0))).r) / 255.,
         
    };

    vec4 values_normals[8] = {
        normalize((convRGBA8ToVec4(imageLoad(input_normals, ivec3(sample_coord + ivec3(1, 1, 1))).r) - 127)),
        normalize((convRGBA8ToVec4(imageLoad(input_normals, ivec3(sample_coord + ivec3(1, 1, 0))).r) - 127)),
        normalize((convRGBA8ToVec4(imageLoad(input_normals, ivec3(sample_coord + ivec3(1, 0, 1))).r) - 127)),
        normalize((convRGBA8ToVec4(imageLoad(input_normals, ivec3(sample_coord + ivec3(1, 0, 0))).r) - 127)),
        normalize((convRGBA8ToVec4(imageLoad(input_normals, ivec3(sample_coord + ivec3(0, 1, 1))).r) - 127)),
        normalize((convRGBA8ToVec4(imageLoad(input_normals, ivec3(sample_coord + ivec3(0, 1, 0))).r) - 127)),
        normalize((convRGBA8ToVec4(imageLoad(input_normals, ivec3(sample_coord + ivec3(0, 0, 1))).r) - 127)),
        normalize((convRGBA8ToVec4(imageLoad(input_normals, ivec3(sample_coord + ivec3(0, 0, 0))).r) - 127)),
         
    };

    float sum = 0.f;
    vec4 final = vec4(0);

    for (int i = 0; i < 8; i++) {
        if (values_normals[i].x < 0) {
            sum++;
            final += values[i];
        }
    }

    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 0, 128), final / max(sum, 1.f));

    sum = 0.f;
    final = vec4(0);

     for (int i = 0; i < 8; i++) {
        if (values_normals[i].y < 0) {
            sum++;
            final += values[i];
        }
    }

    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 1, 128), final / max(sum, 1.f));

    sum = 0.f;
    final = vec4(0);

     for (int i = 0; i < 8; i++) {
        if (values_normals[i].z < 0) {
            sum++;
            final += values[i];
        }
    }

    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 2, 128), final / max(sum, 1.f));

        sum = 0.f;
    final = vec4(0);

     for (int i = 0; i < 8; i++) {
        if (values_normals[i].x > 0) {
            sum++;
            final += values[i];
        }
    }

    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 3, 128), final / max(sum, 1.f));


        sum = 0.f;
    final = vec4(0);

     for (int i = 0; i < 8; i++) {
        if (values_normals[i].y > 0) {
            sum++;
            final += values[i];
        }
    }

    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 4, 128), final / max(sum, 1.f));

    
        sum = 0.f;
    final = vec4(0);

     for (int i = 0; i < 8; i++) {
        if (values_normals[i].z > 0) {
            sum++;
            final += values[i];
        }
    }

    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 5, 128), final / max(sum, 1.f));


    
#elif defined(ANISOTROPIC_MIPMAP_CHAIN)
    vec4 values[8];
    uint mip_dim = imageSize(output_mips).x;
    float sum = 0.f;

    // X+
    sum = FetchTexels(sample_coord, 0, values, mip_dim * 2);
    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 0, mip_dim), 
        Average8(values) * (8.0 / sum)
    );

    // Y+
    sum = FetchTexels(sample_coord, 1, values, mip_dim * 2);
    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 1, mip_dim), 
               Average8(values) * (8.0 / sum)

    );

    // Z+
    sum = FetchTexels(sample_coord, 2, values, mip_dim * 2);
    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 2, mip_dim), 
                Average8(values) * (8.0 / sum)

    );

    // X-
    sum = FetchTexels(sample_coord, 3, values, mip_dim * 2);
    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 3, mip_dim), 
              Average8(values) * (8.0 / sum)

    );

    // Y-
    sum = FetchTexels(sample_coord, 4, values, mip_dim * 2);
    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 4, mip_dim), 
               Average8(values) * (8.0 / sum)

    );

    // Z-
    sum = FetchTexels(sample_coord, 5, values, mip_dim * 2);
    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 5, mip_dim), 
                Average8(values) * (8.0 / sum)

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