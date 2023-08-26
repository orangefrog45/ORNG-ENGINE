#version 430 core

in vec3 vs_position;

out layout(location = 0) vec4 FragColor;



layout(std140, binding = 2) uniform commons{
	vec4 camera_pos;
	vec4 camera_target;
	float time_elapsed;
	float render_resolution_x;
	float render_resolution_y;
	float cam_zfar;
	float cam_znear;
} ubo_common;

void main()
{
	float distance = length(vec2(ubo_common.camera_pos.xz - vs_position.xz)) * 0.05;
	FragColor = vec4(1, 1, 1, max(1.f / distance - 0.25, 0.f));
};