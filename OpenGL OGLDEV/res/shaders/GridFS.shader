#version 430 core

layout(location = 0) in vec2 TexCoord0;
layout(location = 1) in vec3 cam_pos;
layout(location = 3) in vec3 vs_position;

out vec4 FragColor;

void main()
{
	float distance = length(vec2(cam_pos.xz - vs_position.xz)) * 0.05;
	FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f) * ((1 / distance) - 0.15);
};