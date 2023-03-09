#version 420 core

const unsigned int MATRICES_BINDING = 0;

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 TexCoord;
in layout(location = 2) vec3 vertex_normal;
in layout(location = 3) mat4 transform;

layout(row_major, std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
} PVMatrices;


out vec2 TexCoord0;
out vec3 vs_position;
out vec3 vs_normal;

void main() {
	gl_Position = (PVMatrices.projection * PVMatrices.view * transform) * vec4(position, 1.0);
	vs_normal = transpose(inverse(mat3(transform))) * vertex_normal;
	vs_position = vec4(transform * vec4(position, 1.0f)).xyz;
	TexCoord0 = TexCoord;
}
