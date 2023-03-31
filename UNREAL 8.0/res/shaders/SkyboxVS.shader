#version 430 core

layout(location = 0) in vec3 position;

layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
} PVMatrices;

out vec3 TexCoord0;

void main() {
	vec4 view_pos = vec4((mat3(PVMatrices.view) * position), 1.0);
	vec4 pos = PVMatrices.projection * view_pos;
	gl_Position = pos.xyww;
	TexCoord0 = position;
}
