#version 430 core

in vec3 cam_pos;
in vec3 vs_position;

out vec4 FragColor;

void main()
{
	float distance = length(vec2(cam_pos.xz - vs_position.xz)) * 0.05;
	FragColor = vec4(0.8f, 0.8f, 0.8f, (1.0f * ((1 / distance) - 0.15)));
};