#version 430 core

#ifdef PARTICLE
out layout(location = 0) vec4 normal;
out layout(location = 1) vec4 albedo;
out layout(location = 2) vec4 roughness_metallic_ao;
out layout(location = 3) uint shader_id;
#else
out vec4 o_col;
#endif

uniform vec4 u_color;

void main() {
#ifdef PARTICLE
	shader_id = 0;
	albedo = u_color;
	albedo.w = 1.0;
#else
	o_col = u_color;
#endif

}