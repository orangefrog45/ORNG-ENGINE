#version 430 core

out layout(location = 0) vec4 normal;
out layout(location = 1) vec4 albedo;
out layout(location = 2) vec4 roughness_metallic_ao;
out layout(location = 3) uint shader_id;

layout(binding = 1) uniform sampler2D diffuse_sampler;
layout(binding = 2) uniform sampler2D roughness_sampler;
layout(binding = 7) uniform sampler2D normal_map_sampler;
layout(binding = 9) uniform sampler2D displacement_sampler;
layout(binding = 13) uniform samplerCube cube_color_sampler;
layout(binding = 17) uniform sampler2D metallic_sampler;
layout(binding = 18) uniform sampler2D ao_sampler;
layout(binding = 25) uniform sampler2D emissive_sampler;


in vec4 vs_position;
in vec3 vs_normal;
in vec3 vs_tex_coord;
in vec3 vs_tangent;
in mat4 vs_transform;
in vec3 vs_original_normal;
in vec3 vs_view_dir_tangent_space;

#define PARTICLES_DETACHED

ORNG_INCLUDE "ParticleBuffersINCL.glsl"

#ifdef PARTICLE
flat in uint vs_particle_index;
#endif

ORNG_INCLUDE "CommonINCL.glsl"


#ifndef SKYBOX_MODE
	uniform uint u_shader_id;
	uniform bool u_normal_sampler_active;
	uniform bool u_roughness_sampler_active;
	uniform bool u_metallic_sampler_active;
	uniform bool u_displacement_sampler_active;
	uniform bool u_emissive_sampler_active;
	uniform bool u_ao_sampler_active;
	uniform bool u_terrain_mode;
	uniform bool u_skybox_mode;
	uniform float u_parallax_height_scale;
	uniform uint u_num_parallax_layers;
	uniform float u_bloom_threshold;
	uniform Material u_material;



mat3 CalculateTbnMatrixTransform() {
#ifdef PARTICLE
	vec3 t = normalize(-vec3(PVMatrices.view[0][0], PVMatrices.view[1][0], PVMatrices.view[2][0]));
	vec3 n = normalize(vs_normal);
#else
	vec3 t = normalize(vec3(mat3(vs_transform) * vs_tangent));
	vec3 n = normalize(vec3(mat3(vs_transform) * vs_original_normal));
#endif

	t = normalize(t - dot(t, n) * n);
	vec3 b = cross(n, t);

	mat3 tbn = mat3(t, b, n);

	return tbn;
}


mat3 CalculateTbnMatrix() {
	vec3 t = normalize(vs_tangent);
	vec3 n = normalize(vs_original_normal);

	t = normalize(t - dot(t, n) * n);
	vec3 b = cross(n, t);

	mat3 tbn = mat3(t, b, n);

	return tbn;
}


vec4 CalculateAlbedoAndEmissive(vec2 tex_coord) {
	vec4 sampled_albedo = texture(diffuse_sampler, tex_coord.xy);
	vec4 albedo_col = vec4(sampled_albedo.rgb * u_material.base_color.rgb, sampled_albedo.a);
	albedo_col *= u_material.emissive ? vec4(vec3(u_material.emissive_strength), 1.0) : vec4(1.0);

#if defined PARTICLE && !defined(PARTICLES_DETACHED)
#define EMITTER ssbo_particle_emitters.emitters[ssbo_particles.particles[vs_particle_index].emitter_index]
#define PTCL PARTICLE_SSBO.particles[vs_particle_index]

	float interpolation = 1.0 - clamp(PTCL.velocity_life.w, 0.0, EMITTER.lifespan) / EMITTER.lifespan;

	albedo_col *= vec4(InterpolateV3(interpolation, EMITTER.colour_over_life), 1.0);

#endif
	if (u_emissive_sampler_active) {
		vec4 sampled_col = texture(emissive_sampler, tex_coord);
		albedo_col += vec4(sampled_col.rgb * u_material.emissive_strength * sampled_col.w, 0.0);
	}

	return albedo_col;
}

#endif



void main() {
	if (ssbo_particles_detached.particles[vs_particle_index].velocity_life.w < 0)
	discard;
	
	albedo = vec4(ssbo_particles_detached.particles[vs_particle_index].scale.w, ssbo_particles_detached.particles[vs_particle_index].pos.w, ssbo_particles_detached.particles[vs_particle_index].quat.w, 1.0);
	if (albedo.w < 0.99)
		discard;
roughness_metallic_ao = vec4(0.5, 0.5, 0.01, 1.0);
	if (u_normal_sampler_active) {
		mat3 tbn = CalculateTbnMatrixTransform();
		vec3 sampled_normal = texture(normal_map_sampler, vs_tex_coord.xy).rgb * 2.0 - 1.0;
		normal = vec4(normalize(tbn * sampled_normal).rgb, 1.0);
	}
	else {
		normal = vec4(normalize(vs_normal), 1.0);
	}
	shader_id = 10;


}