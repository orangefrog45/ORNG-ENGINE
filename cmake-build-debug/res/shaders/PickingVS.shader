#version 430 core

layout(location = 0) in vec3 pos;

layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
	mat4 proj_view;
} PVMatrices;


uniform mat4 world_transform;

void main() {
	gl_Position = PVMatrices.proj_view * world_transform * vec4(pos, 1.0);
}