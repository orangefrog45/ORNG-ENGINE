#version 430 core

uniform uint comp_id;
out uint id_val;

void main() {
	id_val = comp_id;
}