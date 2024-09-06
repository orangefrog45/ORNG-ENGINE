
#version 430 core
out vec4 Fragcolour;

in vec3 vs_position;
in vec3 vs_normal;

uniform vec3 camera_pos;
layout(binding = 1) uniform samplerCube skybox;

void main() {
	vec3 I = normalize(vs_position - camera_pos);
	vec3 R = reflect(I, normalize(vs_normal));
	Fragcolour = vec4(texture(skybox, R).rgb, 1.0);
}