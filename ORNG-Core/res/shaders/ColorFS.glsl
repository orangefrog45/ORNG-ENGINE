R"(#version 430 core

out vec4 o_col;

uniform vec3 u_color;
in vec3 col;

void main() {
	o_col = vec4(col, 1.0);
})"