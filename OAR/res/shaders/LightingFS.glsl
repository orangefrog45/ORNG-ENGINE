#version 430 core

#define PI 3.1415926538
#define MAX_REFLECTION_LOD 4.0

out vec4 FragColor;
in vec2 tex_coord;
const unsigned int NUM_SHADOW_CASCADES = 3;



struct DirectionalLight {
	vec4 direction;
	vec4 color;

	//stored in vec3 instead of array due to easier alignment
	vec3 cascade_ranges;
};


struct PointLight {
	vec4 color; //vec4s used to prevent implicit padding
	vec4 pos;
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
	float max_distance;
	//attenuation
	float constant;
	float a_linear;
	float exp;
	//ap
	float aperture;
};




layout(std140, binding = 1) buffer PointLights {
	PointLight lights[];
} point_lights;

layout(std140, binding = 2) buffer SpotLights {
	SpotLight lights[];
} spot_lights;

layout(std140, binding = 1) uniform GlobalLighting{
	DirectionalLight directional_light;
} ubo_global_lighting;


layout(std140, binding = 2) uniform commons{
	vec4 camera_pos;
	vec4 camera_target;
	float time_elapsed;
} ubo_common;


uniform mat4 u_dir_light_matrices[NUM_SHADOW_CASCADES];

layout(binding = 1) uniform sampler2D albedo_sampler;
layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 4) uniform sampler2DArray spot_depth_sampler;
layout(binding = 6) uniform sampler2D world_position_sampler;
layout(binding = 7) uniform sampler2D normal_sampler;
layout(binding = 11) uniform sampler2D blue_noise_sampler;
layout(binding = 12) uniform usampler2D shader_material_id_sampler;
layout(binding = 19) uniform sampler2D roughness_metallic_ao_sampler;
layout(binding = 20) uniform samplerCube diffuse_prefilter_sampler;
layout(binding = 21) uniform samplerCube specular_prefilter_sampler;
layout(binding = 22) uniform sampler2D brdf_lut_sampler;

unsigned int shader_id = texture(shader_material_id_sampler, tex_coord).r;
unsigned int material_id = texture(shader_material_id_sampler, tex_coord).g;
vec3 sampled_world_pos = texture(world_position_sampler, tex_coord).xyz;
vec3 sampled_normal = normalize(texture(normal_sampler, tex_coord).xyz);
vec3 sampled_albedo = texture(albedo_sampler, tex_coord).xyz;

vec3 roughness_metallic_ao = texture(roughness_metallic_ao_sampler, tex_coord).rgb;
float roughness = roughness_metallic_ao.r;
float metallic = roughness_metallic_ao.g;
float ao = roughness_metallic_ao.b;

float ShadowCalculationSpotlight(SpotLight light, float light_index) {
	vec4 frag_pos_light_space = light.light_transform_matrix * vec4(sampled_world_pos, 1.0);

	//perspective division
	vec3 proj_coords = (frag_pos_light_space / frag_pos_light_space.w).xyz;

	//depth map in range 0,1 proj in -1,1 (NDC), so convert
	proj_coords = proj_coords * 0.5 + 0.5;

	//make fragments out of shadow bounds appear not in shadow
	if (proj_coords.z > 1.0) {
		return 0.0; // early return, fragment out of range
	}

	//actual depth for comparison
	float current_depth = proj_coords.z;

	float bias = max(0.0001f * (1.0f - dot(normalize(sampled_normal), light.dir.xyz)), 0.0000005f);

	float shadow = 0.0f;

	vec2 texel_size = 1.0 / textureSize(spot_depth_sampler, 0).xy;
	for (float x = -1.0; x <= 1.0; x++)
	{
		for (float y = -1.0; y <= 1.0; y++)
		{
			float pcf_depth = texture(spot_depth_sampler, vec3(vec2(proj_coords.xy + vec2(x, y) * texel_size * 2), light_index)).r;
			shadow += current_depth - bias > pcf_depth ? 1.0 : 0.0;
		}
	}


	// Take average of PCF
	shadow /= 9.0;

	return shadow;
}

float ShadowCalculationDirectional(vec3 light_dir) {
	float shadow = 0.0f;
	//perspective division
	vec3 proj_coords = vec3(0);
	float frag_distance_from_cam = length(sampled_world_pos.xyz - ubo_common.camera_pos.xyz);

	if (frag_distance_from_cam > ubo_global_lighting.directional_light.cascade_ranges[2]) // out of range of cascades
		return 0.f;

	vec4 frag_pos_light_space = vec4(0);

	unsigned int depth_map_index = 0;

	if (frag_distance_from_cam < ubo_global_lighting.directional_light.cascade_ranges[0]) { // cascades
		frag_pos_light_space = u_dir_light_matrices[0] * vec4(sampled_world_pos, 1);
	}
	else if (frag_distance_from_cam < ubo_global_lighting.directional_light.cascade_ranges[1]) {
		frag_pos_light_space = u_dir_light_matrices[1] * vec4(sampled_world_pos, 1);
		depth_map_index = 1;
	}
	else if (frag_distance_from_cam < ubo_global_lighting.directional_light.cascade_ranges[2]) {
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

	vec2 texel_size = vec2(1.0) / textureSize(dir_depth_sampler, 0).xy;

	if (frag_distance_from_cam <= ubo_global_lighting.directional_light.cascade_ranges[0]) {
		for (float x = -1.0; x <= 1.0; x++)
		{
			for (float y = -1.0; y <= 1.0; y++)
			{
				float pcf_depth = texture(dir_depth_sampler, vec3(vec2(proj_coords.xy + vec2(x, y) * texel_size), depth_map_index)).r;
				shadow += current_depth > pcf_depth ? 1.0f : 0.0f;
			}
		}

		/*Take average of PCF*/
		return shadow / 9.0;
	}
	else {
		float sampled_depth = texture(dir_depth_sampler, vec3(vec2(proj_coords.xy), depth_map_index)).r;
		shadow = current_depth > sampled_depth ? 1.0f : 0.0f;
		return shadow;
	}

}


















vec3 FresnelSchlick(float cos_theta, vec3 f0) {
	return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 h) {
	float a = roughness * roughness;
	float a2 = a * a;
	float n_dot_h = max(dot(sampled_normal, h), 0.0);
	float n_dot_h2 = n_dot_h * n_dot_h;

	float num = a2;
	float denom = (n_dot_h2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
}

float GeometrySchlickGGX(float n_dot_v) {
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	return n_dot_v / (n_dot_v * (1.0 - k) + k);
}

float GeometrySmith(vec3 v, vec3 l) {
	float n_dot_v = max(dot(sampled_normal, v), 0.0);
	float n_dot_l = max(dot(sampled_normal, l), 0.0);
	float ggx2 = GeometrySchlickGGX(n_dot_v);
	float ggx1 = GeometrySchlickGGX(n_dot_l);

	return ggx2 * ggx1;
}



vec3 FresnelSchlickRoughness(float cos_theta, vec3 F0)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}



vec3 CalcPointLight(PointLight light, vec3 v, vec3 f0) {
	vec3 frag_to_light = light.pos.xyz - sampled_world_pos.xyz;

	float distance = length(frag_to_light);
	float attenuation = light.constant +
		light.a_linear * distance +
		light.exp * pow(distance, 2);

	vec3 l = normalize(frag_to_light);
	vec3 h = normalize(v + l);

	vec3 f = FresnelSchlick(max(dot(h, v), 0.0), f0);
	float ndf = DistributionGGX(h);
	float g = GeometrySmith(v, l);

	vec3 num = ndf * g * f;
	float denom = 4.0 * max(dot(sampled_normal, v), 0.0) * max(dot(sampled_normal, l), 0.0) + 0.0001;
	vec3 specular = num / denom;


	vec3 kd = vec3(1.0) - f;
	kd *= 1.0 - metallic;

	float n_dot_l = max(dot(sampled_normal, l), 0.0);


	return (kd * sampled_albedo.xyz / PI + specular) * n_dot_l * (light.color.xyz / attenuation);

}


vec3 CalcSpotLight(SpotLight light, vec3 v, vec3 f0, int index) {

	vec3 frag_to_light = light.pos.xyz - sampled_world_pos.xyz;


	float spot_factor = dot(normalize(frag_to_light), -light.dir.xyz);

	//SHADOW
	vec3 color = vec3(0);
	float shadow = ShadowCalculationSpotlight(light, index);

	if (shadow >= 0.99 || spot_factor <= 0.0) {
		return color; // early return as no light will reach this spot
	}

	float distance = length(frag_to_light);
	float attenuation = light.constant +
		light.a_linear * distance +
		light.exp * pow(distance, 2);

	vec3 l = normalize(frag_to_light);
	vec3 h = normalize(v + l);

	vec3 f = FresnelSchlick(max(dot(h, v), 0.0), f0);
	float ndf = DistributionGGX(h);
	float g = GeometrySmith(v, l);

	vec3 num = ndf * g * f;
	float denom = 4.0 * max(dot(sampled_normal, v), 0.0) * max(dot(sampled_normal, l), 0.0) + 0.0001;
	vec3 specular = num / denom;


	vec3 kd = vec3(1.0) - f;
	kd *= 1.0 - metallic;

	float n_dot_l = max(dot(sampled_normal, l), 0.0);

	float spotlight_intensity = (1.0 - (1.0 - spot_factor) / (1.0 - light.aperture));
	return max((kd * sampled_albedo.xyz / PI + specular) * n_dot_l * (light.color.xyz / attenuation) * spotlight_intensity, vec3(0.0, 0.0, 0.0));
}


vec3 CalcDirectionalLight(vec3 v, vec3 f0) {

	vec3 l = ubo_global_lighting.directional_light.direction.xyz;
	vec3 h = normalize(v + l);
	vec3 f = FresnelSchlick(max(dot(h, v), 0.0), f0);

	float ndf = DistributionGGX(h);
	float g = GeometrySmith(v, l);

	vec3 num = ndf * g * f;
	float denom = 4.0 * max(dot(sampled_normal, v), 0.0) * max(dot(sampled_normal, l), 0.0) + 0.0001;
	vec3 specular = num / denom;


	vec3 kd = vec3(1.0) - f;
	kd *= 1.0 - metallic;

	float n_dot_l = max(dot(sampled_normal, l), 0.0);
	return (kd * sampled_albedo.xyz / PI + specular) * n_dot_l * ubo_global_lighting.directional_light.color.xyz;
}





void main()
{
	if (shader_id != 1) { // 1 = default lighting shader id
		FragColor = vec4(sampled_albedo, 1);
		return;
	}


	vec2 adj_tex_coord = tex_coord; // USE PARALLAX ONCE FIXED

	vec3 total_light = vec3(0.0, 0.0, 0.0);
	vec3 v = normalize(ubo_common.camera_pos.xyz - sampled_world_pos);
	vec3 r = reflect(-v, sampled_normal);
	float n_dot_v = max(dot(sampled_normal, v), 0.0);

	//reflection amount
	vec3 f0 = vec3(0.04);
	f0 = mix(f0, sampled_albedo.xyz, metallic);

	// Directional light
	float shadow = ShadowCalculationDirectional(normalize(ubo_global_lighting.directional_light.direction.xyz));
	total_light += CalcDirectionalLight(v, f0) * (1.0 - shadow);

	// Ambient 
	vec3 ks = FresnelSchlickRoughness(n_dot_v, f0);
	vec3 kd = 1.0 - ks;
	vec3 diffuse = texture(diffuse_prefilter_sampler, sampled_normal).rgb * sampled_albedo.xyz;

	vec3 prefiltered_colour = textureLod(specular_prefilter_sampler, r, roughness * MAX_REFLECTION_LOD).rgb;
	vec2 env_brdf = texture(brdf_lut_sampler, vec2(n_dot_v, roughness)).rg;
	vec3 specular = prefiltered_colour * (ks * env_brdf.x + env_brdf.y);

	vec3 ambient_light = (kd * diffuse + specular) * ao;
	total_light += ambient_light;


	// Pointlights
	for (int i = 0; i < point_lights.lights.length(); i++) {
		total_light += (CalcPointLight(point_lights.lights[i], v, f0));
	}

	// Spotlights
	for (int i = 0; i < spot_lights.lights.length(); i++) {
		total_light += (CalcSpotLight(spot_lights.lights[i], v, f0, i));
	}

	vec3 light_color = max(vec3(total_light), vec3(0.0, 0.0, 0.0));

	FragColor = vec4(light_color, 1);
};