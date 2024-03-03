#version 430 core

#if defined(PARTICLE) || defined(VOXELIZATION)
out layout(location = 0) vec4 normal;
out layout(location = 1) vec4 albedo;
out layout(location = 2) vec4 roughness_metallic_ao;
out layout(location = 3) uint shader_id;
#else
out vec4 o_col;
#endif

uniform vec4 u_color;

ORNG_INCLUDE "BuffersINCL.glsl"

#ifdef VOXELIZATION
layout(binding = 0, rgba16f) uniform readonly image3D voxel_tex;

in flat ivec3 vs_lookup_coord;
#endif

void main() {
#ifdef PARTICLE
	shader_id = 0;
	albedo = u_color;
	albedo.w = 1.0;
#elif defined(VOXELIZATION)
	albedo.xyzw = imageLoad(voxel_tex, vs_lookup_coord);
	if (length(albedo.w) < 0.00001 )
		discard;
	albedo.w = 1.0;
	shader_id = 0;

#else
	o_col = u_color;
#endif

}