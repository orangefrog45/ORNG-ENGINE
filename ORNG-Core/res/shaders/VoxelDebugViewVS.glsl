#version 460 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec2 tex_coord;
in layout(location = 2) vec3 vertex_normal;
in layout(location = 3) vec3 in_tangent;

ORNG_INCLUDE "BuffersINCL.glsl"
out flat ivec3 vs_lookup_coord;
uniform vec3 u_aligned_camera_pos;

void main() {
    vs_lookup_coord = ivec3(gl_InstanceID % 128, (gl_InstanceID / 128) % 128, gl_InstanceID / (128 * 128));

    vec3 pos = (vec3(vs_lookup_coord.xyz) + position - vec3(64, 64, 64)) * 0.4;
    gl_Position = PVMatrices.proj_view * vec4( u_aligned_camera_pos + pos, 1.0);
}