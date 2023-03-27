#version 430 core

layout(location = 0) in vec3 position;

layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
} PVMatrices;


uniform mat4 gTransform;

out vec3 TexCoord0;

void main() {
	vec4 pos = PVMatrices.projection * gTransform * vec4(position, 1.0);
	gl_Position = pos.xyww;
	TexCoord0 = position;
}
