#version 430 core

out vec4 FragColor;

in vec2 tex_coords;

layout(binding = 1) uniform sampler2D texture1;

uniform vec3 test;

void main() {
	//float color = texture(texture1, vec3(tex_coords, 0.0)).r;

	FragColor = texture(texture1, tex_coords);
	//FragColor = vec4(vec3(1.0 - texture(screen_texture, tex_coords)), 1.0); - colour inversion
}