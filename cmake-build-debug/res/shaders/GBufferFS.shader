#version 430 core

out layout(location = 0) vec4 g_position;
out layout(location = 1) vec4 normal;
out layout(location = 2) vec4 albedo;
out layout(location = 3) vec4 tangent;
out layout(location = 4) uint material_id;


layout(binding = 1) uniform sampler2D diffuse_sampler;
layout(binding = 2) uniform sampler2D specular_sampler;
layout(binding = 7) uniform sampler2D normal_map_sampler;
layout(binding = 8) uniform sampler2DArray diffuse_array_sampler;
layout(binding = 10) uniform sampler2DArray normal_array_sampler;
layout(binding = 9) uniform sampler2D displacement_sampler;


in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_tex_coord;
in vec3 vs_tangent;

uniform uint u_material_id;
uniform bool normal_sampler_active;
uniform bool terrain_mode;

/*vec2 ParallaxMap() {
	const float num_layers = 10;
	float layer_depth = 1.0 / num_layers;
	float current_layer_depth = 0.0f;
	vec2 current_tex_coords = tex_coord0;
	float current_depth_map_value = texture(displacement_sampler, tex_coord0).r;
	vec3 view_dir = normalize(fs_in_tangent_positions.view_pos - fs_in_tangent_positions.frag_pos);
	vec2 p = view_dir.xy * 0.1;
	vec2 delta_tex_coords = p / num_layers;

	while (current_layer_depth < current_depth_map_value) {
		current_tex_coords -= delta_tex_coords;
		current_depth_map_value = texture(displacement_sampler, current_tex_coords).r;
		current_layer_depth += layer_depth;
	}

	return current_tex_coords;
}*/


void main() {
	g_position = vec4(vs_position, 1);
	tangent = vec4(vs_tangent, 1);
	material_id = u_material_id;

	vec2 adj_tex_coord = vs_tex_coord; // add logic once parallax works

	float terrain_factor = clamp(dot(vs_normal, vec3(0, 1, 0)), 0.f, 1.f); // slope

	if (terrain_mode) {
		normal = vec4(normalize(vs_normal), 1.f);

		//normal = vec4(normalize(mix(texture(normal_array_sampler, vec3(adj_tex_coord, 1.0)), texture(normal_array_sampler, vec3(adj_tex_coord, 0.0)), terrain_factor).rgb * 2.0 - 1.0), 1.f);
		albedo = vec4(mix(texture(diffuse_array_sampler, vec3(adj_tex_coord, 1.0)).rgb, texture(diffuse_array_sampler, vec3(adj_tex_coord, 0.0)).rgb, terrain_factor), 1.f);
	}
	else {
		normal = vec4(normalize(vs_normal), 1.f);
		//normal = normal_sampler_active ? vec4(normalize(texture(normal_map_sampler, vs_tex_coord).rgb * 2.0 - 1.0), 1) : vec4(normalize(vs_normal), 1.f);
		albedo = vec4(texture(diffuse_sampler, adj_tex_coord).rgb, 1.f);
	}

}