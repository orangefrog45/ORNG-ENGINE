#version 430 core

in layout(location = 0) vec2 pos;
in layout(location = 1) vec2 itex_coords;

uniform mat3 screen_transform;

out vec2 tex_coords;

void main() {
	gl_Position = vec4((screen_transform * vec3(pos.x, pos.y, 1.0)).xy, 0.0f, 1.0f);
	tex_coords = itex_coords;
}