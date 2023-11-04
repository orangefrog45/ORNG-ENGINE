#version 430 core

const unsigned int MATRICES_BINDING = 0;

layout(location = 0) in vec3 position;
in layout(location = 2) vec3 vertex_normal;
in layout(location = 3) mat4 transform;

layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
	mat4 proj_view;
	mat4 inv_projection;
	mat4 inv_view;
} PVMatrices;

out vec3 vs_position;
out vec3 vs_normal;


void main() {
	gl_Position = (PVMatrices.proj_view * transform) * vec4(position, 1.0);
	vs_normal = transpose(inverse(mat3(transform))) * vertex_normal;
	vs_position = vec3(transform * vec4(position, 1.0f));
}
