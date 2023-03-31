#version 430 core

in layout(location = 0) vec3 pos;
in layout(location = 3) mat4 world_transform;

layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
} PVMatrices;


out vec3 vs_world_pos;

void main() {
	vs_world_pos = vec4(world_transform * vec4(pos, 1.0)).xyz;
	gl_Position = PVMatrices.projection * PVMatrices.view * vec4(vs_world_pos, 1);
}