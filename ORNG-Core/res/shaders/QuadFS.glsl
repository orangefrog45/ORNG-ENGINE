#version 430 core

out vec4 Fragcolour;

in vec2 tex_coords;

layout(binding = 1) uniform sampler2D quad_sampler;



void main() {
	Fragcolour = vec4(texture(quad_sampler, tex_coords).rgb, 1.0);
}