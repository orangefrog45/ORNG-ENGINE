#version 430 core

uniform uvec3 comp_id;
out uvec3 id_val;

void main() {
	id_val = comp_id;
}