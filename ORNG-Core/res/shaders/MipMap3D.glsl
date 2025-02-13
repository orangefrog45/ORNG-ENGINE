#version 460 core

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;


#ifdef ANISOTROPIC_MIPMAP
layout(binding = 0, r32ui) readonly uniform uimage3D input_image;
layout(binding = 1, rgba16f) writeonly uniform image3D output_mips; // [pos_x, pos_y, pos_z, neg_x, neg_y, neg_z], each face starts at z = face_index * voxel_tex_resolution
layout(binding = 2, r32ui) readonly uniform uimage3D input_normals;
#elif defined(ANISOTROPIC_MIPMAP_CHAIN)
layout(binding = 0, rgba16f) writeonly uniform image3D output_mips; // [pos_x, pos_y, pos_z, neg_x, neg_y, neg_z], each face starts at z = face_index * voxel_tex_resolution
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

void FetchTexels(ivec3 start_coord, uint face_index, out vec4[8] values, uint mip_dim) {
    for (int i = 0; i < 8; i++) {
        ivec3 local_coord = ivec3(start_coord + aniso_offsets[i]);
        ivec3 coord = ApplyMipFaceTexCoordOffset(local_coord, face_index, mip_dim);
        uint current_face_index = uint(coord.z) / mip_dim;
        values[i] = texelFetch(input_mips, coord, u_mip_level - 1);
    }
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

float IsCoordValid(ivec3 coord) {
    return all(lessThan(coord, ivec3(VOXEL_RES))) ? 0.f : 1.f;
}

ivec3 ConstrainCoord(ivec3 coord) {
    return min(coord, ivec3(VOXEL_RES));
}

void main() {
ivec3 sample_coord = ivec3(gl_GlobalInvocationID.xyz);

#ifdef ANISOTROPIC_MIPMAP
    vec4 colour_val = convRGBA8ToVec4(imageLoad(input_image, ivec3(sample_coord)).r) / 255.0;
    vec4 normal = normalize((convRGBA8ToVec4(imageLoad(input_normals, ivec3(sample_coord)).r) - 127));

    // Scale colour by emissive factor stored in alpha component
    colour_val.rgb *= (colour_val.a * 25.5);

    // Alpha stores emissive component, switch it back to an actual opacity value here (1.0 for full voxel, 0 for empty, no inbetween)
    colour_val.a = normal.a > 0.01 ? 1.0 : 0.0;

    float sum = 0.f;
    vec4 final = vec4(0);

    if (colour_val.a > 0.01 && normal.x < 0) {
        sum = abs(normal.x);
        final.rgb = colour_val.rgb *  abs(normal.x);
    }

    final.a = colour_val.a;

    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 0, 256), vec4(final.rgb, final.a));

    sum = 0.f;
    final = vec4(0);

    if (colour_val.a > 0.01 && normal.y < 0) {
        sum = abs(normal.y);
        final.rgb = colour_val.rgb * abs(normal.y);
    }

    final.a = colour_val.a;

    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 1, 256), vec4(final.rgb, final.a));

    sum = 0.f;
    final = vec4(0);

    if (colour_val.a > 0.01 && normal.z < 0) {
        sum = abs(normal.z);
        final.rgb = colour_val.rgb * abs(normal.z);
    }

    final.a += colour_val.a;

    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 2, 256), vec4(final.rgb, final.a));

        sum = 0.f;
    final = vec4(0);

    if (colour_val.a > 0.01 && normal.x > 0) {
        sum = normal.x;
        final.rgb = colour_val.rgb * normal.x;
    }

    final.a = colour_val.a;

    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 3, 256), vec4(final.rgb, final.a));


        sum = 0.f;
    final = vec4(0);

    if (colour_val.a > 0.01 && normal.y > 0) {
        sum = normal.y;
        final.rgb = colour_val.rgb * normal.y;
    }

    final.a = colour_val.a;

    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 4, 256), vec4(final.rgb, final.a));

    
    sum = 0.f;
    final = vec4(0);

    if (colour_val.a > 0.01 && normal.z > 0) {
        sum = normal.z;
        final.rgb = colour_val.rgb * normal.z;
    }

    final.a = colour_val.a;

    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 5, 256), vec4(final.rgb, final.a));

  
    // vec4 val = convRGBA8ToVec4(imageLoad(input_image, sample_coord / 2).r) / 255.;
    // // Scale colour by emissive factor stored in alpha component
    // val.rgb *= (val.a * 25.5); 
    
    // // Alpha stores emissive component, switch it back to an actual opacity value here (1.0 for full voxel, 0 for empty, no inbetween)
    // val.a = val.a > 0.01 ? 1.0 : 0.0;
    // vec4 norm = normalize((convRGBA8ToVec4(imageLoad(input_normals, ivec3(sample_coord / 2)).r) - 127));

    // imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 0, 256), val * (norm.x < 0 ? 1.0 : 0.0) );
    // imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 1, 256), val * (norm.y < 0 ? 1.0 : 0.0) );
    // imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 2, 256), val * (norm.z < 0 ? 1.0 : 0.0) );
    // imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 3, 256), val * (norm.x > 0 ? 1.0 : 0.0) );
    // imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 4, 256), val * (norm.y > 0 ? 1.0 : 0.0) );
    // imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 5, 256), val * (norm.z > 0 ? 1.0 : 0.0) );

    
#elif defined(ANISOTROPIC_MIPMAP_CHAIN)
    sample_coord = ivec3(gl_GlobalInvocationID.xyz*2);
    vec4 values[8];
    uint mip_dim = imageSize(output_mips).x;
    float sum = 0.f;

    // X+
    FetchTexels(sample_coord, 0, values, mip_dim * 2);
    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 0, mip_dim), 
        Average8(values)
    );

    // Y+
    FetchTexels(sample_coord, 1, values, mip_dim * 2);
    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 1, mip_dim), 
               Average8(values)

    );

    // Z+
    FetchTexels(sample_coord, 2, values, mip_dim * 2);
    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 2, mip_dim), 
                Average8(values)

    );

    // X-
    FetchTexels(sample_coord, 3, values, mip_dim * 2);
    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 3, mip_dim), 
              Average8(values)

    );

    // Y-
     FetchTexels(sample_coord, 4, values, mip_dim * 2);
    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 4, mip_dim), 
               Average8(values)

    );

    // Z-
    FetchTexels(sample_coord, 5, values, mip_dim * 2);
    imageStore(output_mips, ApplyMipFaceTexCoordOffset(ivec3(gl_GlobalInvocationID.xyz), 5, mip_dim), 
                Average8(values)

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