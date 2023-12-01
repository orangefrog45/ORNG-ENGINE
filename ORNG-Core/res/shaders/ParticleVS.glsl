#version 460 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec2 tex_coord;
in layout(location = 2) vec3 vertex_normal;
in layout(location = 3) vec3 in_tangent;

ORNG_INCLUDE "BuffersINCL.glsl"

layout(std140, binding = 0) buffer transforms {
	mat4 transforms[];
} transform_ssbo;

out vec4 vs_world_pos;

void main() {
    gl_Position = PVMatrices.proj_view * transform_ssbo.transforms[gl_InstanceID] * vec4(position, 1.0);
}