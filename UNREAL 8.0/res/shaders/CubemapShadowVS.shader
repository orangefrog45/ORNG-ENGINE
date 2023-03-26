#version 430 core

layout(location = 0) in vec3 pos;
in layout(location = 3) mat4 world_transform;


layout(row_major, std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
} PVMatrices;


uniform mat4 pv_mat;

out vec3 vs_world_pos;

void main() {
	vec4 world_pos = world_transform * vec4(pos, 1.0);
	gl_Position = PVMatrices.projection * PVMatrices.view * world_pos;
	vs_world_pos = world_pos.xyz;
}
