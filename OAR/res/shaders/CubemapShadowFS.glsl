R""(#version 430 core

in vec3 vs_world_pos;

uniform vec3 light_pos;

out float light_to_pixel_distance;

void main() {
	light_to_pixel_distance = length(vs_world_pos - light_pos);
})""