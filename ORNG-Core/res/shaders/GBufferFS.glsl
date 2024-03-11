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


#ifdef TESSELLATE
in flat int ts_instance_id_out;

layout(std140, binding = 0) buffer transforms {
	mat4 transforms[];
} transform_ssbo;

#define TRANSFORM transform_ssbo.transforms[ts_instance_id_out]

in TSVertData {
	vec4 position;
	vec3 normal;
	vec2 tex_coord;
	vec3 tangent;
	vec3 original_normal;
	vec3 view_dir_tangent_space;
} vert_data;
#else
in VSVertData {
    vec4 position;
    vec3 normal;
    vec2 tex_coord;
    vec3 tangent;
    vec3 original_normal;
    vec3 view_dir_tangent_space;
} vert_data;

in flat mat4 vs_transform;

#define TRANSFORM vs_transform
#endif



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

vec2 ParallaxMap()
{
	float layer_depth = 1.0 / float(u_num_parallax_layers);
	float current_layer_depth = 0.0f;
	vec2 current_tex_coords = vert_data.tex_coord.xy * u_material.tile_scale;
	float current_depth_map_value = 1.0 - texture(displacement_sampler, current_tex_coords).r;
	vec2 p = normalize(vert_data.view_dir_tangent_space).xy * u_parallax_height_scale;

	vec2 delta_tex_coords = p / u_num_parallax_layers * u_material.tile_scale;
	float delta_depth = layer_depth / u_num_parallax_layers;

	while (current_layer_depth < current_depth_map_value) {
		current_tex_coords -= delta_tex_coords;
		current_depth_map_value = 1.0 - texture(displacement_sampler, current_tex_coords).r;
		current_layer_depth += layer_depth;
	}

	vec2 prev_tex_coords = current_tex_coords + delta_tex_coords;

	float after_depth = current_depth_map_value - current_layer_depth;
	float before_depth = (1.0 - texture(displacement_sampler, prev_tex_coords).r) - current_layer_depth + layer_depth;

	float weight = after_depth / (after_depth - before_depth);

	return prev_tex_coords * weight + current_tex_coords * (1.0 - weight);

}
mat3 CalculateTbnMatrixTransform() {
#ifdef PARTICLE
	vec3 t = normalize(-vec3(PVMatrices.view[0][0], PVMatrices.view[1][0], PVMatrices.view[2][0]));
	vec3 n = normalize(vert_data.normal);
#else
	vec3 t = normalize(vec3(mat3(TRANSFORM) * vert_data.tangent));
	vec3 n = normalize(vec3(mat3(TRANSFORM) * vert_data.original_normal));
#endif

	t = normalize(t - dot(t, n) * n);
	vec3 b = cross(n, t);

	mat3 tbn = mat3(t, b, n);

	return tbn;
}


mat3 CalculateTbnMatrix() {
	vec3 t = normalize(vert_data.tangent);
	vec3 n = normalize(vert_data.original_normal);

	t = normalize(t - dot(t, n) * n);
	vec3 b = cross(n, t);

	mat3 tbn = mat3(t, b, n);

	return tbn;
}


vec4 CalculateAlbedoAndEmissive(vec2 tex_coord) {
	vec4 sampled_albedo = texture(diffuse_sampler, tex_coord.xy);
	vec4 albedo_col = vec4(sampled_albedo.rgb * u_material.base_color.rgb, sampled_albedo.a);
	albedo_col *= bool(u_material.flags & MAT_FLAG_EMISSIVE) ? vec4(vec3(u_material.emissive_strength), 1.0) : vec4(1.0);
#ifdef PARTICLE
#define EMITTER ssbo_particle_emitters.emitters[ssbo_particles.particles[vs_particle_index].emitter_index]
#define PTCL ssbo_particles.particles[vs_particle_index]

	float interpolation = 1.0 - clamp(PTCL.velocity_life.w, 0.0, EMITTER.lifespan) / EMITTER.lifespan;

	albedo_col *= vec4(InterpolateV3(interpolation, EMITTER.colour_over_life), 1.0);

#endif
	if (u_emissive_sampler_active) {
		vec4 sampled_col = texture(emissive_sampler, tex_coord);
		albedo_col += vec4(sampled_col.rgb * u_material.emissive_strength * sampled_col.w, 0.0);
	}

	return albedo_col;
}

#endif // ifndef SKYBOX_MODE



void main() {
#ifndef SKYBOX_MODE
	vec2 adj_tex_coord = bool(u_material.flags & MAT_FLAG_PARALLAX_MAPPED) ? ParallaxMap() : vert_data.tex_coord.xy * u_material.tile_scale;
	roughness_metallic_ao.r = texture(roughness_sampler, adj_tex_coord.xy).r * float(u_roughness_sampler_active) + u_material.roughness * float(!u_roughness_sampler_active);
	roughness_metallic_ao.g = texture(metallic_sampler, adj_tex_coord.xy).r * float(u_metallic_sampler_active) + u_material.metallic * float(!u_metallic_sampler_active);
	roughness_metallic_ao.b = texture(ao_sampler, adj_tex_coord.xy).r * float(u_ao_sampler_active) + u_material.ao * float(!u_ao_sampler_active);
	roughness_metallic_ao.a = 1.f;
#endif

#ifdef TERRAIN_MODE
	shader_id = u_shader_id;
	albedo = CalculateAlbedoAndEmissive(adj_tex_coord);

#elif defined SKYBOX_MODE
	shader_id = uint(0);
	albedo = vec4(texture(cube_color_sampler, normalize(vert_data.position.xyz)).rgb, 1.f);
#else
	albedo = CalculateAlbedoAndEmissive(adj_tex_coord);
	if (albedo.w < 0.25)
		discard;

		albedo.rgb *= albedo.w;
	if (u_normal_sampler_active) {
		mat3 tbn = CalculateTbnMatrixTransform();
		vec3 sampled_normal = normalize(texture(normal_map_sampler, adj_tex_coord.xy).rgb * 2.0 - 1.0);
		normal = vec4(normalize(tbn * sampled_normal * (gl_FrontFacing ? 1.0 : -1.0)).rgb, 1.0);
	}
	else {
		normal = vec4(normalize(vert_data.normal * (gl_FrontFacing ? 1.0 : -1.0)), 1.0);
	}

	shader_id = u_shader_id;
#endif

}