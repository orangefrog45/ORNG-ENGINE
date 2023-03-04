#version 430 core

layout(location = 0) in vec3 pos;

uniform mat4 wvp;
void main() {
	vec4 position = wvp * vec4(pos, 1.0);
	gl_Position = position;
}