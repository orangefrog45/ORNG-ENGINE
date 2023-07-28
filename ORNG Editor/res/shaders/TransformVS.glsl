#version 430 core

in layout(location = 0) vec3 pos;

layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
	mat4 proj_view;
} PVMatrices;

uniform mat4 transform;

void main() {
	gl_Position = PVMatrices.proj_view * transform * vec4(pos * 1.01f, 1.f);
}