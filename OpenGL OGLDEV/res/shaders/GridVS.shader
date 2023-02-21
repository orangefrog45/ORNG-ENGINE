#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 TexCoord;
in layout(location = 3) mat4 transform;

out vec2 TexCoord0;

uniform mat4 projection;
uniform mat4 camera;
uniform vec3 camera_pos;

out vec3 cam_pos;
out vec3 vs_position;


void main() {
	cam_pos = camera_pos;
	TexCoord0 = TexCoord;
	vec4 world_pos = transform * vec4(position, 1.0);
	vs_position = vec3(world_pos);
	vec4 pos = projection * camera * vec4(world_pos.x, world_pos.y, world_pos.z, 1.0);
	gl_Position = pos;
}
