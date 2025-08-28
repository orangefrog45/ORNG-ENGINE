#version 430 core

out vec4 frag_colour;

in vec2 tex_coords;

layout(binding = 1) uniform sampler2D quad_sampler;

void main() {
	frag_colour = vec4(texture(quad_sampler, tex_coords).rgb, 1.0);
}