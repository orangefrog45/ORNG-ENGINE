#version 430 core

//LIGHT IDENTIFIERS
const unsigned int LIGHT_TYPE_DIRECTIONAL = 0;
const unsigned int LIGHT_TYPE_SPOT = 1;
const unsigned int LIGHT_TYPE_POINT = 2;

const unsigned int NUM_SHADOW_CASCADES = 3;



struct BaseLight {
	vec4 color;
	float ambient_intensity;
	float diffuse_intensity;
};

struct DirectionalLight {
	vec3 color;
	vec3 direction;
	float ambient_intensity;
	float diffuse_intensity;
};

struct PointLight {
	vec4 color; //vec4s used to prevent implicit padding
	vec4 pos;
	float ambient_intensity;
	float diffuse_intensity;
	float max_distance;
	//attenuation
	float constant;
	float a_linear;
	float exp;
};

struct SpotLight { //140 BYTES
	vec4 color; //vec4s used to prevent implicit padding
	vec4 pos;
	vec4 dir;
	mat4 light_transform_matrix;
	float ambient_intensity;
	float diffuse_intensity;
	float max_distance;
	//attenuation
	float constant;
	float a_linear;
	float exp;
	//ap
	float aperture;
};


struct Material {
	vec4 ambient_color;
	vec4 diffuse_color;
	vec4 specular_color;
	bool using_normal_maps;
};


layout(std140, binding = 1) buffer PointLights {
	PointLight lights[];
} point_lights;

layout(std140, binding = 2) buffer SpotLights {
	SpotLight lights[];
} spot_lights;

const unsigned int MAX_MATERIALS = 128;

layout(std140, binding = 3) uniform Materials{
	Material materials[128];
} u_materials;


uniform BaseLight u_ambient_light; // ambient
uniform Material u_material;
uniform DirectionalLight u_directional_light;
uniform vec3 u_view_pos;
uniform mat4 u_dir_light_matrices[NUM_SHADOW_CASCADES];


/*in TangentSpacePositions{
	vec3 u_view_pos;
	vec3 frag_pos;
	vec3 dir_light_dir;
	vec3 spot_positions[MAX_SPOT_LIGHTS];
	vec3 point_positions[MAX_SPOT_LIGHTS];
} fs_in_tangent_positions;*/


in vec2 tex_coord;

out vec4 FragColor;

layout(binding = 1) uniform sampler2D albedo_sampler;
layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 4) uniform sampler2DArray spot_depth_sampler;
layout(binding = 6) uniform sampler2D world_position_sampler;
layout(binding = 7) uniform sampler2D normal_sampler;
layout(binding = 11) uniform sampler2D blue_noise_sampler;
layout(binding = 12) uniform usampler2D shader_material_id_sampler;

unsigned int shader_id = texture(shader_material_id_sampler, tex_coord).r;
unsigned int material_id = texture(shader_material_id_sampler, tex_coord).g;
Material sampled_material = u_materials.materials[material_id];
vec3 sampled_world_pos = texture(world_position_sampler, tex_coord).xyz;
vec3 sampled_normal = normalize(texture(normal_sampler, tex_coord).xyz);
vec3 sampled_albedo = texture(albedo_sampler, tex_coord).xyz;


/*float ShadowCalculation(vec4 t_frag_pos_light_space, vec3 light_dir, int depth_map_index, unsigned int type) {
	float shadow = 0.0f;
	//perspective division
	vec3 proj_coords = (t_frag_pos_light_space / t_frag_pos_light_space.w).xyz;

	//depth map in range 0,1 proj in -1,1 (NDC), so convert
	proj_coords = proj_coords * 0.5 + 0.5;

	//make fragments out of shadow bounds appear not in shadow
	if (proj_coords.z > 1.0) {
		return 0.0; // early return, fragment out of range
	}

	//actual depth for comparison
	float current_depth = proj_coords.z;


	float closest_depth = 0.0f;

	//sample closest depth from lights pov from depth map
	float bias = 0.0f;

	if (type == LIGHT_TYPE_SPOT) {
		bias = max(0.0001f * (1.0f - dot(normalize(sampled_normal), light_dir)), 0.0000005f);


		vec2 texel_size = 1.0 / textureSize(spot_depth_sampler, 0).xy;
		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				float pcf_depth = texture(spot_depth_sampler, vec3(vec2(proj_coords.xy + vec2(x, y) * texel_size * 2), depth_map_index)).r;
				shadow += current_depth - bias > pcf_depth ? 1.0 : 0.0;
			}
		}
	}


	// Take average of PCF
	shadow /= 9.0;

	return shadow;
}
*/

float ShadowCalculationDirectional(vec3 light_dir) {
	float shadow = 0.0f;
	//perspective division
	vec3 proj_coords = vec3(0);
	float frag_distance_from_cam = length(sampled_world_pos.xyz - u_view_pos);

	if (frag_distance_from_cam > 2000.f) // out of range of cascades
		return 0.f;

	vec4 frag_pos_light_space = vec4(0);

	unsigned int depth_map_index = 0;

	if (frag_distance_from_cam <= 200.f) { // cascades
		frag_pos_light_space = u_dir_light_matrices[0] * vec4(sampled_world_pos, 1);
	}
	else if (frag_distance_from_cam <= 500.f) {
		frag_pos_light_space = u_dir_light_matrices[1] * vec4(sampled_world_pos, 1);
		depth_map_index = 1;
	}
	else if (frag_distance_from_cam <= 1200.f) {
		frag_pos_light_space = u_dir_light_matrices[2] * vec4(sampled_world_pos, 1);
		depth_map_index = 2;
	}

	proj_coords = (frag_pos_light_space / frag_pos_light_space.w).xyz;

	//depth map in range 0,1 proj in -1,1 (NDC), so convert
	proj_coords = proj_coords * 0.5f + 0.5f;

	//make fragments out of shadow bounds appear not in shadow
	if (proj_coords.z > 1.0f) {
		return 0.0f; // early return, fragment out of range
	}

	//actual depth for comparison
	float current_depth = proj_coords.z;

	//sample closest depth from lights pov from depth map
	float bias = 0.0f;
	bias = 0.001f * tan(acos(clamp(dot(normalize(sampled_normal), light_dir), 0, 1))); // slope bias

	vec2 texel_size = 1.0 / textureSize(dir_depth_sampler, 0).xy;

	if (frag_distance_from_cam <= 200.f) {
		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				float pcf_depth = texture(dir_depth_sampler, vec3(vec2(proj_coords.xy + vec2(x, y) * texel_size), depth_map_index)).r;
				shadow += current_depth - bias > pcf_depth ? 1.0f : 0.0f;
			}
		}

		/*Take average of PCF*/
		shadow /= 9.0f;
	}
	else {
		float sampled_depth = texture(dir_depth_sampler, vec3(vec2(proj_coords.xy), depth_map_index)).r;
		shadow = current_depth - bias > sampled_depth ? 1.0f : 0.0f;
	}

	return shadow;
}





vec3 CalcPhongLight(vec3 light_color, float light_diffuse_intensity, vec3 normalized_light_dir, vec3 norm) {

	//diffuse
	float diffuse_factor = clamp(dot(normalized_light_dir, norm), 0, 1);

	vec3 diffuse_final = vec3(0, 0, 0);
	vec3 specular_final = vec3(0, 0, 0);

	diffuse_final = light_color * diffuse_factor * light_diffuse_intensity * sampled_material.diffuse_color.rgb;
	//diffuse_final = light_color * diffuse_factor * light_diffuse_intensity * u_material.diffuse_color;

	float specular_strength = 5.f;
	vec3 view_dir = normalize(u_view_pos - sampled_world_pos);
	vec3 reflect_dir = reflect(-normalized_light_dir, norm);
	float specular_factor = max(dot(view_dir, reflect_dir), 0.0);

	float specular_exponent = 32; ///-------------------------------------------------------- SWITCH TO SAMPLING --------------------------------------------------------;
	float spec_highlight = pow(specular_factor, specular_exponent);
	specular_final = specular_strength * spec_highlight * light_color * sampled_material.specular_color.rgb;
	//specular_final = specular_strength * spec_highlight * light_color * u_material.specular_color;

	return (diffuse_final + specular_final);
}






vec3 CalcPointLight(PointLight light, vec3 normal, float distance, int index) {
	vec3 frag_to_light_dir = vec3(0);

	frag_to_light_dir = normalize(light.pos.xyz - sampled_world_pos.xyz);

	vec3 color = CalcPhongLight(light.color.xyz, light.diffuse_intensity, frag_to_light_dir, normal);

	float attenuation = light.constant +
		light.a_linear * distance +
		light.exp * pow(distance, 2);

	return color / attenuation;
}






vec3 CalcSpotLight(SpotLight light, vec3 normal, float distance, int index) {
	vec3 frag_to_light_dir = vec3(0);

	frag_to_light_dir = normalize(light.pos.xyz - sampled_world_pos.xyz);

	float spot_factor = dot(frag_to_light_dir, -light.dir.xyz);

	if (spot_factor > light.aperture) {
		//SHADOW
		vec3 color = vec3(0);
		vec4 light_space_pos = light.light_transform_matrix * vec4(sampled_world_pos, 1.0);
		//float shadow = ShadowCalculation(light_space_pos, light.dir.xyz, index, LIGHT_TYPE_SPOT);

		//if (shadow == 1.0) {
			//return color; // early return as no light will reach this spot
		//}

		color = CalcPhongLight(light.color.xyz, light.diffuse_intensity, frag_to_light_dir, normal);

		//ATTENUATION
		float attenuation = light.constant +
			light.a_linear * distance +
			light.exp * pow(distance, 2);

		color /= attenuation;

		//SPOTLIGHT APERTURE
		float spotlight_intensity = (1.0 - (1.0 - spot_factor) / (1.0 - light.aperture));
		return color * spotlight_intensity;
	}
	else {
		return vec3(0, 0, 0);
	}
}
/*
vec2 ParallaxMap() {
	const float num_layers = 10;
	float layer_depth = 1.0 / num_layers;
	float current_layer_depth = 0.0f;
	vec2 current_tex_coords = tex_coord0;
	float current_depth_map_value = texture(displacement_sampler, tex_coord0).r;
	vec3 view_dir = normalize(fs_in_tangent_positions.u_view_pos - fs_in_tangent_positions.frag_pos);
	vec2 p = view_dir.xy * 0.1;
	vec2 delta_tex_coords = p / num_layers;

	while (current_layer_depth < current_depth_map_value) {
		current_tex_coords -= delta_tex_coords;
		current_depth_map_value = texture(displacement_sampler, current_tex_coords).r;
		current_layer_depth += layer_depth;
	}

	return current_tex_coords;
}*/




void main()
{
	if (shader_id != 1) { // 1 = default lighting shader id
		FragColor = vec4(sampled_albedo, 1);
		return;
	}


	vec2 adj_tex_coord = tex_coord; // USE PARALLAX ONCE FIXED

	vec3 total_light = vec3(0.0, 0.0, 0.0);

	// Directional light
	float shadow = ShadowCalculationDirectional(normalize(u_directional_light.direction));
	total_light += CalcPhongLight(u_directional_light.color, u_directional_light.diffuse_intensity, normalize(u_directional_light.direction), normalize(sampled_normal)) * (1.f - shadow);

	// Ambient 
	vec3 ambient_light = u_ambient_light.ambient_intensity * u_ambient_light.color.xyz * sampled_material.ambient_color.rgb;
	total_light += ambient_light;


	// Pointlights
	for (int i = 0; i < point_lights.lights.length(); i++) {
		float distance = length(point_lights.lights[i].pos.xyz - sampled_world_pos.xyz);
		if (distance <= point_lights.lights[i].max_distance) {
			total_light += (CalcPointLight(point_lights.lights[i], sampled_normal, distance, i));
		}
	}

	// Spotlights
	for (int i = 0; i < spot_lights.lights.length(); i++) {
		float distance = length(spot_lights.lights[i].pos.xyz - sampled_world_pos.xyz);
		if (distance <= spot_lights.lights[i].max_distance) {
			total_light += (CalcSpotLight(spot_lights.lights[i], sampled_normal, distance, i));
		}
	}



	vec3 light_color = max(vec3(total_light), vec3(0.0, 0.0, 0.0));

	FragColor = vec4(light_color * sampled_albedo.xyz, 1);
};