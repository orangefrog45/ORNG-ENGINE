#version 430 core

in vec3 vs_position;

out layout(location = 0) vec4 FragColor;

ORNG_INCLUDE "BuffersINCL.glsl"


void main()
{
	discard;
	float distance = length(vec2(ubo_common.camera_pos.xz - vs_position.xz)) * 0.05;
	FragColor = vec4(1, 1, 1, max(1.f / distance - 0.25, 0.f));
};