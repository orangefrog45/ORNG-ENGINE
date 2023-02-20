#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 TexCoord;
in layout(location = 2) vec3 vertex_normal;
in layout(location = 3) mat4 transform;

uniform mat4 camera;
uniform mat4 projection;

out vec2 TexCoord0;
out vec3 vs_position;
out vec3 vs_normal;

void main() {
	gl_Position = (projection * camera * transform) * vec4(position, 1.0);
	vs_normal = mat3(transform) * vertex_normal;
	vs_position = vec4(transform * vec4(position, 1.0f)).xyz;
	TexCoord0 = TexCoord;
}
