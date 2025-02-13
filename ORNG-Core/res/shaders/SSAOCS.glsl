#version 460 core
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 1, rgba16f) writeonly uniform image2D u_output_image;

layout(binding = 1) uniform sampler2D normal_sampler;
layout(binding = 2) uniform sampler2D depth_sampler;
layout(binding = 11) uniform sampler2D blue_noise_sampler;

ORNG_INCLUDE "BuffersINCL.glsl"
ORNG_INCLUDE "UtilINCL.glsl"

mat3 CreateCoordinateSystem(vec3 n) {
    vec3 nt, nb;
    if (abs(n.x) > abs(n.y))
        nt = vec3(n.z, 0, -n.x) / length(n.xz);
    else
        nt = vec3(0, -n.z, n.y) / length(n.yz);

    nb = cross(n, nt);
    
    return mat3(nt, nb, n);
}

vec3 UniformSampleHemisphere(float r1, float r2) {
  float sin_theta = sqrt(1 - r1 * r1);
  float phi = 2 * PI * r2;
  return vec3(sin_theta * cos(phi), sin_theta * sin(phi), r1);
}

const float PHI = 1.61803398874989484820459; // Φ = Golden Ratio 

float gold_noise(in vec2 xy, in float seed)
{
return fract(tan(distance(xy*PHI, xy)*seed)*xy.x);
}

float LinearizeDepth(float depth) {
    return ubo_common.cam_znear * ubo_common.cam_zfar / ((depth * (ubo_common.cam_zfar - ubo_common.cam_znear)) - ubo_common.cam_zfar);
}

void main() {
    ivec2 sample_coord = ivec2(gl_GlobalInvocationID.xy);

    vec3 normal = texelFetch(normal_sampler, sample_coord, 0).rgb;
    float depth = texelFetch(depth_sampler, sample_coord, 0).r;
    vec3 world_pos = WorldPosFromDepth(depth, vec2(sample_coord) / vec2(textureSize(depth_sampler, 0)));

    mat3 tangent_to_world = CreateCoordinateSystem(normal);

    const float radius = 0.075;
    float occlusion = 0.0;

    for (int i = 0; i < 32; i++) {
        const float r1 = gold_noise(vec2(sample_coord), fract(abs(dot(world_pos, vec3((i + 1) / 32.f)))));
        const float r2 = gold_noise(vec2(sample_coord), fract(depth + i / 32.f));
        const vec3 offset = UniformSampleHemisphere(r1, r2);

        vec3 sample_pos = (tangent_to_world * (offset * radius)) + world_pos;

        vec4 proj = PVMatrices.proj_view * vec4(sample_pos, 1.0);
        proj.xyz /= proj.w;
        proj.xyz = proj.xyz * 0.5 + 0.5;
        float sample_depth = texelFetch(depth_sampler, ivec2(proj.xy * textureSize(depth_sampler, 0)), 0).r;
        occlusion += (sample_depth >= depth ? 1.0 : 0.0);
    }
    imageStore(u_output_image, sample_coord, vec4(vec3(occlusion / 32.f), 1.0));
}

