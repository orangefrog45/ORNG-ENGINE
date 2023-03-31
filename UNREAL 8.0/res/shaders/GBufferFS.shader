#version 430 core

layout(location = 0) out vec4 g_position;

in vec3 vs_world_pos;

void main() {
	g_position = vec4(vs_world_pos, 1);
}