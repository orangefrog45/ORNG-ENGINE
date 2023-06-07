#version 430 core

out layout(location = 0) vec4 g_position;
out layout(location = 1) vec4 normal;
out layout(location = 2) vec4 albedo;
out layout(location = 3) vec4 tangent;
out layout(location = 4) uvec2 shader_material_id;


layout(binding = 1) uniform sampler2D diffuse_sampler;
layout(binding = 2) uniform sampler2D specular_sampler;
layout(binding = 7) uniform sampler2D normal_map_sampler;
layout(binding = 8) uniform sampler2DArray diffuse_array_sampler;
layout(binding = 9) uniform sampler2D displacement_sampler;
layout(binding = 10) uniform sampler2DArray normal_array_sampler;
layout(binding = 13) uniform samplerCube cube_color_sampler;


in vec4 vs_position;
in vec3 vs_normal;
in vec3 vs_tex_coord;
in vec3 vs_tangent;
in mat4 vs_transform;

const unsigned int MAX_MATERIALS = 128;

struct Material {
	vec4 ambient_color;
	vec4 diffuse_color;
	vec4 specular_color;
	bool using_normal_maps;
};

layout(std140, binding = 2) uniform commons{
	vec4 camera_pos;
	vec4 camera_target;
} ubo_common;


layout(std140, binding = 3) uniform Materials{ //change to ssbo
	Material materials[128];
} u_materials;


uniform uint u_material_id;
uniform uint u_shader_id;
uniform bool u_normal_sampler_active;
uniform bool u_terrain_mode;
uniform bool u_skybox_mode;


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

mat3 CalculateTbnMatrix() {
	vec3 t = normalize(vs_tangent);
	vec3 n = normalize(vs_normal);

	t = normalize(t - dot(t, n) * n);
	vec3 b = normalize(vec3(vec4(cross(normalize(vs_normal), normalize(vs_tangent)), 0.0)));

	mat3 tbn = mat3(t, b, n);

	return tbn;
}


mat3 CalculateTbnMatrixTransform() {
	vec3 t = normalize(vec3(vs_transform * vec4(vs_tangent, 0.0f)));
	vec3 n = normalize(vec3(vs_transform * vec4(vs_normal, 0.0f)));

	t = normalize(t - dot(t, n) * n);
	vec3 b = cross(vs_normal, vs_tangent);

	mat3 tbn = mat3(t, b, n);

	return tbn;
}

vec4 Slerp(vec4 p0, vec4 p1, float t)
{
	float dotp = dot(normalize(p0), normalize(p1));
	if ((dotp > 0.9999) || (dotp < -0.9999))
	{
		if (t <= 0.5)
			return p0;
		return p1;
	}
	float theta = acos(dotp);
	vec4 P = ((p0 * sin((1 - t) * theta) + p1 * sin(t * theta)) / sin(theta));
	P.w = 1;
	return P;
}


void main() {
	tangent = vec4(vs_tangent, 1);
	shader_material_id = uvec2(u_shader_id, u_material_id);

	vec2 adj_tex_coord = vs_tex_coord.xy; // add parallax logic

	if (u_terrain_mode) {
		mat3 tbn = CalculateTbnMatrix();
		vec3 sampled_normal_1 = texture(normal_array_sampler, vec3(adj_tex_coord, 0.0)).rgb * 2.0 - 1.0;
		vec3 sampled_normal_2 = texture(normal_array_sampler, vec3(adj_tex_coord, 1.0)).rgb * 2.0 - 1.0;

		float terrain_factor = clamp(dot(vs_normal.xyz, vec3(0, 1.f, 0) * 0.5f), 0.f, 1.f); // slope
		vec3 mixed_terrain_normal = tbn * normalize(Slerp(vec4(sampled_normal_1, 0.0), vec4(sampled_normal_2, 0.0), terrain_factor)).xyz;
		normal = vec4(normalize(mixed_terrain_normal), 1.f);
		g_position = vs_position;

		albedo = vec4(mix(texture(diffuse_array_sampler, vec3(adj_tex_coord, 0.0)).rgb, texture(diffuse_array_sampler, vec3(adj_tex_coord, 1.0)).rgb, terrain_factor), 1.f);

	}
	else if (u_skybox_mode) {
		g_position = vec4(ubo_common.camera_pos.xyz + normalize(vs_position.xyz) * 2000.f, 1.f); // give spherical appearance (used for fog)
		albedo = vec4(texture(cube_color_sampler, vs_tex_coord).rgb, 1.f);
	}
	else {
		g_position = vs_position;

		if (u_normal_sampler_active) {
			mat3 tbn = CalculateTbnMatrixTransform();
			normal = vec4(tbn * normalize(texture(normal_map_sampler, adj_tex_coord).rgb * 2.0 - 1.0).rgb, 1);
		}
		else {
			normal = vec4(normalize(vs_normal), 1.f);
		}
		albedo = vec4(texture(diffuse_sampler, adj_tex_coord).rgb, 1.f);
	}

}