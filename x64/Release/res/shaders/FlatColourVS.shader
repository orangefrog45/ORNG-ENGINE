#version 430 core
const unsigned int MATRICES_BINDING = 0;


layout(location = 0) in vec3 pos;
layout(location = 3) in mat4 world_transform;

layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
	mat4 proj_view;
} PVMatrices;

void main() {
	vec4 position = PVMatrices.proj_view * world_transform * vec4(pos, 1.0);
	gl_Position = position;
}