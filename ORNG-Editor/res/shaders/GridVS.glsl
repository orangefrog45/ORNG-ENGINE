#version 430 core

layout(location = 0) in vec3 position;

layout(std140, binding = 0) uniform Matrices{
	mat4 projection;
	mat4 view;
	mat4 proj_view;
} PVMatrices;

layout(std140, binding = 0) buffer transforms {
	mat4 transforms[];
} transform_ssbo;




out vec3 vs_position;


void main() {
	vec4 world_pos = transform_ssbo.transforms[gl_InstanceID] * vec4(position, 1.0);
	vs_position = vec3(world_pos);
	vec4 pos = PVMatrices.proj_view * vec4(world_pos.xyz, 1.0);
	gl_Position = pos;
}
