#version 430 core

in layout(location = 0) vec2 pos;
in layout(location = 1) vec2 itex_coords;

out vec2 tex_coords;

void main() {
	gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
	tex_coords = itex_coords;
}