#version 430 core

uniform uvec2 comp_id;
out uvec2 id_val;

void main() {
	id_val = comp_id;
}