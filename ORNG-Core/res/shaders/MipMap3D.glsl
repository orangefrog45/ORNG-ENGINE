#version 460 core

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;


#ifdef ANISOTROPIC_MIPMAP
layout(binding = 0, r32ui) readonly uniform uimage3D input_image;
layout(binding = 1, rgba16f) writeonly uniform image3D output_mips[6]; // [pos_x, pos_y, pos_z, neg_x, neg_y, neg_z]
layout(binding = 2, r32ui) readonly uniform uimage3D input_normals;
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

vec4 convRGBA8ToVec4(uint val) {
  return vec4(float((val & 0x000000FF)),
      float((val & 0x0000FF00) >> 8U),
      float((val & 0x00FF0000) >> 16U),
      float((val & 0xFF000000) >> 24U));
}

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

vec4 WeightValues(vec4 values[8], vec4 values_normals[8], int face) {
    float sum = 0.f;
    vec4 final = vec4(0);

    for (int i = 0; i < 8; i++) {
        if (values_normals[i].x < 0) {
            sum++;
            final += values[i];
        }
    }

    return final / sum;
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

#define WEIGHT_VALUE(i, dir) values[i] * max(dot(values_normals[i], dir), 0)
    const bool DIR_CHECK = false;
    //vec4 neg_x = (AlphaBlend(ioo * max(dot(vec3(-1, 0, 0), ioo_normal)), ooi) + AlphaBlend(iio, oii) + AlphaBlend(ioi, oio) + AlphaBlend(iii, ooo)) * 0.25;

    float sum = 0.f;
    vec4 final = vec4(0);

    for (int i = 0; i < 8; i++) {
        if (values_normals[i].x < 0) {
            sum++;
            final += values[i];
        }
    }

    imageStore(output_mips[0], ivec3(gl_GlobalInvocationID.xyz), final / max(sum, 1.f));

    sum = 0.f;
    final = vec4(0);

     for (int i = 0; i < 8; i++) {
        if (values_normals[i].y < 0) {
            sum++;
            final += values[i];
        }
    }

    imageStore(output_mips[1], ivec3(gl_GlobalInvocationID.xyz), final / max(sum, 1.f));

    sum = 0.f;
    final = vec4(0);

     for (int i = 0; i < 8; i++) {
        if (values_normals[i].z < 0) {
            sum++;
            final += values[i];
        }
    }

    imageStore(output_mips[2], ivec3(gl_GlobalInvocationID.xyz), final / max(sum, 1.f));

        sum = 0.f;
    final = vec4(0);

     for (int i = 0; i < 8; i++) {
        if (values_normals[i].x > 0) {
            sum++;
            final += values[i];
        }
    }

    imageStore(output_mips[3], ivec3(gl_GlobalInvocationID.xyz), final / max(sum, 1.f));


        sum = 0.f;
    final = vec4(0);

     for (int i = 0; i < 8; i++) {
        if (values_normals[i].y > 0) {
            sum++;
            final += values[i];
        }
    }

    imageStore(output_mips[4], ivec3(gl_GlobalInvocationID.xyz), final / max(sum, 1.f));

    
        sum = 0.f;
    final = vec4(0);

     for (int i = 0; i < 8; i++) {
        if (values_normals[i].z > 0) {
            sum++;
            final += values[i];
        }
    }

    imageStore(output_mips[5], ivec3(gl_GlobalInvocationID.xyz), final / max(sum, 1.f));


    
#elif defined(ANISOTROPIC_MIPMAP_CHAIN)
    vec4 values[8];

    // X+
    FetchTexels(sample_coord, 0, values);
    imageStore(output_mips[0], ivec3(gl_GlobalInvocationID.xyz), 
        Average8(values)
    );

    // Y+
    FetchTexels(sample_coord, 1, values);
    imageStore(output_mips[1], ivec3(gl_GlobalInvocationID.xyz), 
               Average8(values)

    );

    // Z+
    FetchTexels(sample_coord, 2, values);
    imageStore(output_mips[2], ivec3(gl_GlobalInvocationID.xyz), 
                Average8(values)

    );

    // X-
    FetchTexels(sample_coord, 3, values);
    imageStore(output_mips[3], ivec3(gl_GlobalInvocationID.xyz), 
              Average8(values)

    );

    // Y-
    FetchTexels(sample_coord, 4, values);
    imageStore(output_mips[4], ivec3(gl_GlobalInvocationID.xyz), 
               Average8(values)

    );

    // Z-
    FetchTexels(sample_coord, 5, values);
    imageStore(output_mips[5], ivec3(gl_GlobalInvocationID.xyz), 
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