#version 430 core

out layout(location = 0) vec4 g_position;
out layout(location = 1) vec4 normal;
out layout(location = 2) vec4 albedo;
out layout(location = 3) vec4 roughness_metallic_ao;
out layout(location = 4) uint shader_id;


layout(binding = 1) uniform sampler2D diffuse_sampler;
layout(binding = 2) uniform sampler2D roughness_sampler;
layout(binding = 7) uniform sampler2D normal_map_sampler;
layout(binding = 9) uniform sampler2D displacement_sampler;
layout(binding = 13) uniform samplerCube cube_color_sampler;
layout(binding = 17) uniform sampler2D metallic_sampler;
layout(binding = 18) uniform sampler2D ao_sampler;


in vec4 vs_position;
in vec3 vs_normal;
in vec3 vs_tex_coord;
in vec3 vs_tangent;
in mat4 vs_transform;
in vec3 vs_original_normal;
in vec3 vs_view_dir_tangent_space;


struct Material {
	vec4 base_color_and_metallic;
	float roughness;
	float ao;
	vec2 tile_scale;
};


layout(std140, binding = 2) uniform commons{
	vec4 camera_pos;
	vec4 camera_target;
	float time_elapsed;
} ubo_common;


uniform uint u_material_id;
uniform uint u_shader_id;
uniform bool u_normal_sampler_active;
uniform bool u_roughness_sampler_active;
uniform bool u_metallic_sampler_active;
uniform bool u_displacement_sampler_active;
uniform bool u_ao_sampler_active;
uniform bool u_terrain_mode;
uniform bool u_skybox_mode;
uniform float u_parallax_height_scale;
uniform uint u_num_parallax_layers;
uniform Material u_material;


vec2 ParallaxMap()
{
	float layer_depth = 1.0 / float(u_num_parallax_layers);
	float current_layer_depth = 0.0f;
	vec2 current_tex_coords = vs_tex_coord.xy * u_material.tile_scale;
	float current_depth_map_value = 1.0 - texture(displacement_sampler, vs_tex_coord.xy).r;
	vec2 p = normalize(vs_view_dir_tangent_space).xy * u_parallax_height_scale;

	vec2 delta_tex_coords = p / u_num_parallax_layers;
	float delta_depth = layer_depth / u_num_parallax_layers;

	while (current_layer_depth < current_depth_map_value) {
		current_tex_coords -= delta_tex_coords;
		current_depth_map_value = 1.0 - texture(displacement_sampler, current_tex_coords).r;
		current_layer_depth += layer_depth;
	}

	vec2 prev_tex_coords = current_tex_coords + delta_tex_coords;

	float after_depth = current_depth_map_value - current_layer_depth;
	float before_depth = texture(displacement_sampler, prev_tex_coords).r - current_layer_depth + layer_depth;

	float weight = after_depth / (after_depth - before_depth);

	return prev_tex_coords * weight + current_tex_coords * (1.0 - weight);

}

mat3 CalculateTbnMatrix() {
	vec3 t = normalize(vs_tangent);
	vec3 n = normalize(vs_original_normal);

	t = normalize(t - dot(t, n) * n);
	vec3 b = cross(n, t);

	mat3 tbn = mat3(t, b, n);

	return tbn;
}


mat3 CalculateTbnMatrixTransform() {
	vec3 t = normalize(vec3(mat3(vs_transform) * vs_tangent));
	vec3 n = normalize(vec3(mat3(vs_transform) * vs_original_normal));

	t = normalize(t - dot(t, n) * n);
	vec3 b = cross(n, t);

	mat3 tbn = mat3(t, b, n);

	return tbn;
}



void main() {
	shader_id = u_shader_id;
	vec2 adj_tex_coord = u_displacement_sampler_active ? ParallaxMap() : vs_tex_coord.xy * u_material.tile_scale;

	roughness_metallic_ao.r = u_roughness_sampler_active ? texture(roughness_sampler, adj_tex_coord.xy).r : u_material.roughness;
	roughness_metallic_ao.g = u_metallic_sampler_active ? texture(metallic_sampler, adj_tex_coord.xy).r : u_material.base_color_and_metallic.a;
	roughness_metallic_ao.b = u_ao_sampler_active ? texture(ao_sampler, adj_tex_coord.xy).r : u_material.ao;
	roughness_metallic_ao.a = 1.f;


	if (u_terrain_mode) {
		mat3 tbn = CalculateTbnMatrix();
		normal = normalize(texture(normal_map_sampler, adj_tex_coord.xy) * 2.0 - 1.0);
		g_position = vs_position;
		albedo = vec4(texture(diffuse_sampler, adj_tex_coord.xy).rgb * u_material.base_color_and_metallic.rgb, 1.f);

	}
	else if (u_skybox_mode) {
		g_position = vec4(ubo_common.camera_pos.xyz + normalize(vs_position.xyz) * 5000.f, 1.f); // give spherical appearance (used for fog)
		albedo = vec4(texture(cube_color_sampler, vs_tex_coord).rgb, 1.f);
	}
	else {
		g_position = vs_position;

		if (u_normal_sampler_active) {
			mat3 tbn = CalculateTbnMatrixTransform();
			vec3 sampled_normal = texture(normal_map_sampler, adj_tex_coord.xy).rgb * 2.0 - 1.0;
			normal = vec4(normalize(tbn * sampled_normal).rgb, 1);
		}
		else {
			normal = vec4(normalize(vs_normal), 1.f);
		}
		albedo = vec4(texture(diffuse_sampler, adj_tex_coord.xy).rgb * u_material.base_color_and_metallic.rgb, 1.f);
	}
}