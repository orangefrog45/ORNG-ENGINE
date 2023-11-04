#version 460 core
layout(location = 0) in vec3 pos;

out vec3 vs_local_pos;

uniform mat4 projection;
uniform mat4 view;

void main()
{
	vs_local_pos = pos;
	gl_Position = projection * view * vec4(vs_local_pos, 1.0);
}