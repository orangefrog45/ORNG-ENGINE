#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 TexCoord;
in layout(location = 3) mat4 transform;

layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
	mat4 proj_view;
} PVMatrices;



uniform vec3 camera_pos;

out vec3 cam_pos;
out vec3 vs_position;


void main() {
	cam_pos = camera_pos;
	vec4 world_pos = transform * vec4(position, 1.0);
	vs_position = vec3(world_pos);
	vec4 pos = PVMatrices.proj_view * vec4(world_pos.x, world_pos.y, world_pos.z, 1.0);
	gl_Position = pos;
}
