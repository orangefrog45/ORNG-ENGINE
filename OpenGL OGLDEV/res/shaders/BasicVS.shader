#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 TexCoord;
in layout(location = 3) mat4 transform;

out vec2 TexCoord0;


void main() {
	TexCoord0 = TexCoord;
	gl_Position = transform * vec4(position, 1.0);
}
