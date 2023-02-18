#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 TexCoord;
in layout(location = 3) mat4 transform;

uniform mat4 camera;
uniform mat4 projection;

out vec2 TexCoord0;

void main() {
	gl_Position = (projection * camera * transform) * vec4(position, 1.0);
	TexCoord0 = TexCoord;
}
