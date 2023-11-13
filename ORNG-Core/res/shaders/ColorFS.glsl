#version 430 core

out vec4 o_col;

uniform vec4 u_color;

void main() {
	o_col = u_color;
}