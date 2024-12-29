#version 460 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec2 tex_coord;
in layout(location = 2) vec3 vertex_normal;
in layout(location = 3) vec3 in_tangent;

layout(binding = 0, rgba16f) uniform readonly image3D voxel_tex;

ORNG_INCLUDE "BuffersINCL.glsl"
out flat ivec3 vs_lookup_coord;
uniform vec3 u_aligned_camera_pos;

void main() {
    int size = imageSize(voxel_tex).x;
    vs_lookup_coord = ivec3(gl_InstanceID % size, (gl_InstanceID / size) / size, (gl_InstanceID / size) % size);
    vec3 pos = (vec3(vs_lookup_coord.xyz) + position - vec3(size / 2, size / 2, size / 2)) * 0.4 * (256/size) * 0.5;

    gl_Position = PVMatrices.proj_view * vec4( u_aligned_camera_pos + pos, 1.0);
    vs_lookup_coord.z += size * 4;
}