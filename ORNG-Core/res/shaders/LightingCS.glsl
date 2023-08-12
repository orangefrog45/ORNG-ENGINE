R""(#version 430 core

#define PI 3.1415926538
#define MAX_REFLECTION_LOD 4.0
#define LIGHT_ZNEAR 0.01 // true for all light types

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

const unsigned int NUM_SHADOW_CASCADES = 3;
ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy);


struct DirectionalLight {
	vec4 direction;
	vec4 color;
	vec4 cascade_ranges;
	float size;
	float blocker_search_size;
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
	float render_resolution_x;
	float render_resolution_y;
float cam_zfar;
float cam_znear;
} ubo_common;

layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
	mat4 proj_view;
	mat4 inv_projection;
	mat4 inv_view;
} PVMatrices;


uniform mat4 u_dir_light_matrices[NUM_SHADOW_CASCADES];

layout(binding = 1) uniform sampler2D albedo_sampler;
layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 4) uniform sampler2DArray spot_depth_sampler;
layout(binding = 6) uniform sampler2D view_depth_sampler;
layout(binding = 7) uniform sampler2D normal_sampler;
layout(binding = 11) uniform sampler2D blue_noise_sampler;
layout(binding = 12) uniform usampler2D shader_id_sampler;
layout(binding = 19) uniform sampler2D roughness_metallic_ao_sampler;
layout(binding = 20) uniform samplerCube diffuse_prefilter_sampler;
layout(binding = 21) uniform samplerCube specular_prefilter_sampler;
layout(binding = 22) uniform sampler2D brdf_lut_sampler;
layout(binding = 26) uniform samplerCubeArray pointlight_depth_sampler;
layout(binding = 1, rgba16f) writeonly uniform image2D u_output_texture;

vec3 WorldPosFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;

	vec2 normalized_tex_coords = tex_coords / vec2(imageSize(u_output_texture));
    vec4 clipSpacePosition = vec4(normalized_tex_coords * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = PVMatrices.inv_projection * clipSpacePosition;
	

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = PVMatrices.inv_view * viewSpacePosition;

    return worldSpacePosition.xyz;
}

unsigned int shader_id = texelFetch(shader_id_sampler, tex_coords, 0).r;
vec3 sampled_world_pos = WorldPosFromDepth(texelFetch(view_depth_sampler, tex_coords, 0).r);
vec3 sampled_normal = normalize(texelFetch(normal_sampler, tex_coords, 0).xyz);
vec3 sampled_albedo = texelFetch(albedo_sampler, tex_coords, 0).xyz;

vec3 roughness_metallic_ao = texelFetch(roughness_metallic_ao_sampler, tex_coords, 0).rgb;
float roughness = roughness_metallic_ao.r;
float metallic = roughness_metallic_ao.g;
float ao = roughness_metallic_ao.b;

vec2[16] poisson_disk_kernel = vec2[](
	vec2(-0.94201624, -0.39906216),
	vec2(0.94558609, -0.76890725),
	vec2(-0.094184101, -0.92938870),
	vec2(0.34495938, 0.29387760),
	vec2(-0.91588581, 0.45771432),
	vec2(-0.81544232, -0.87912464),
	vec2(-0.38277543, 0.27676845),
	vec2(0.97484398, 0.75648379),
	vec2(0.44323325, -0.97511554),
	vec2(0.53742981, -0.47373420),
	vec2(-0.26496911, -0.41893023),
	vec2(0.79197514, 0.19090188),
	vec2(-0.24188840, 0.99706507),
	vec2(-0.81409955, 0.91437590),
	vec2(0.19984126, 0.78641367),
	vec2(0.14383161, -0.14100790)
	);


vec2[16] poisson_disk_kernel_2 = vec2[](
	vec2(0.29887093, -0.8307311), vec2(-0.91237664, 0.32449767),
	vec2(0.13720949, 0.96615905), vec2(0.69585735, 0.09507584),
	vec2(0.8504305, 0.52665436), vec2(-0.7125196, -0.3954708),
	vec2(-0.053810835, -0.55401367), vec2(-0.46601325, -0.14818008),
	vec2(0.07721871, -0.2698794), vec2(-0.24067602, 0.5337155),
	vec2(0.46601325, 0.14818008), vec2(-0.13720949, -0.96615905),
	vec2(-0.5296358, -0.72831166), vec2(0.7110144, -0.68961465),
	vec2(-0.7110144, 0.68961465), vec2(-0.69585735, -0.09507584)
	);


float random(vec3 seed) {
	return fract(sin(dot(seed, vec3(12.9898, 78.233, 45.543))) * 43758.5453);
}

vec2 CalcBlockerDistanceDirectional(vec3 proj_coord, vec2 texel_size, int sampler_index) {
	float avg_blocker = 0.0f;
	int blockers = 0;

	const int pcss_sample_count = 16;
	float rnd = random(sampled_world_pos.xyz) * 2.0 * PI;

	for (int i = 0; i < pcss_sample_count; i++) {
		vec2 offset = vec2(
			cos(rnd) * poisson_disk_kernel_2[i].x - sin(rnd) * poisson_disk_kernel_2[i].y,
			sin(rnd) * poisson_disk_kernel_2[i].x + cos(rnd) * poisson_disk_kernel_2[i].y
		);

		float z = texture(dir_depth_sampler, vec3(vec2(proj_coord.xy + offset * texel_size * ubo_global_lighting.directional_light.blocker_search_size), sampler_index)).r;

		int valid = int(z < proj_coord.z);
		blockers += valid;
		avg_blocker += z * valid;

	}

	avg_blocker /= float(blockers);
	return vec2(avg_blocker, float(blockers));
}

float CalcAdaptiveEpsilon(float current_depth, float light_max_distance) {
	float num = pow(light_max_distance - current_depth * (light_max_distance - 0.01), 2.0);
	float denom = light_max_distance * 0.01 * (light_max_distance - 0.01);

	float scene_scale = light_max_distance; // Estimating this just based on camera zFar
	float k = 0.00005;

	return (num / denom) * scene_scale * k;
}

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


	float shadow = 0.0f;

	/*vec2 texel_size = 1.0 / textureSize(spot_depth_sampler, 0).xy;
	for (float x = -1.0; x <= 1.0; x++)
	{
		for (float y = -1.0; y <= 1.0; y++)
		{
			float pcf_depth = texture(spot_depth_sampler, vec3(vec2(proj_coords.xy + vec2(x, y) * texel_size * 2), light_index)).r;
			shadow += current_depth > pcf_depth ? 1.0 : 0.0;
		}
	}


	// Take average of PCF
	shadow /= 9.0;*/


	float adaptive_epsilon = CalcAdaptiveEpsilon(current_depth, light.max_distance);
	shadow += ((current_depth - adaptive_epsilon) > texture(spot_depth_sampler, vec3(vec2(proj_coords.xy), light_index)).r) ? 1.0 : 0.0;

	return shadow;
}

float ShadowCalculationPointlight(PointLight light, int light_index) {
	vec3 light_to_frag = sampled_world_pos.xyz - light.pos.xyz;
	float closest_depth = texture(pointlight_depth_sampler, vec4(light_to_frag, light_index)).r * light.max_distance; // Transform normalized depth to range 0, zFar
	float current_depth = length(light_to_frag);
	float bias = 0.1;
	float shadow = int((current_depth - bias) > closest_depth);
	return shadow;
}



float ShadowCalculationDirectional(vec3 light_dir) {
	float shadow = 0.0f;
	//perspective division
	vec3 proj_coords = vec3(0);
	float frag_distance_from_cam = length(sampled_world_pos.xyz - ubo_common.camera_pos.xyz);

	int index = 3;
	index -= int(frag_distance_from_cam <= ubo_global_lighting.directional_light.cascade_ranges[2]);
	index -= int(frag_distance_from_cam <= ubo_global_lighting.directional_light.cascade_ranges[1]);
	index -= int(frag_distance_from_cam <= ubo_global_lighting.directional_light.cascade_ranges[0]);

	vec4 frag_pos_light_space = u_dir_light_matrices[index] * vec4(sampled_world_pos.xyz, 1);

	proj_coords = (frag_pos_light_space / frag_pos_light_space.w).xyz;

	//depth map in range 0,1 proj in -1,1 (NDC), so convert
	proj_coords = proj_coords * 0.5f + 0.5f;

	//make fragments out of shadow bounds appear not in shadow
	if (proj_coords.z > 1.0f || index == 3) {
		return 0.0f; // early return, fragment out of range
	}

	//actual depth for comparison
	float current_depth = proj_coords.z;


	vec2 texel_size = vec2(1.0) / textureSize(dir_depth_sampler, 0).xy;
	vec2 avg_blocker_distance = CalcBlockerDistanceDirectional(proj_coords, texel_size, index);


	float penumbra_size = max(ubo_global_lighting.directional_light.size * (current_depth - avg_blocker_distance.x) / avg_blocker_distance.x, 1.0);

	float rnd = random(sampled_world_pos.xyz) * 2.0 * PI;
	for (int i = 0; i < 16; i++) {
		vec2 offset = vec2(
			cos(rnd) * poisson_disk_kernel[i].x - sin(rnd) * poisson_disk_kernel[i].y,
			sin(rnd) * poisson_disk_kernel[i].x + cos(rnd) * poisson_disk_kernel[i].y
		) * penumbra_size;

		float pcf_depth = texture(dir_depth_sampler, vec3(vec2(proj_coords.xy + offset * texel_size), index)).r;
		shadow += current_depth > pcf_depth ? 1.0f : 0.0f;

	}

	/*Take average of PCF*/
	return shadow / 16.0;


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



vec3 CalcPointLight(PointLight light, vec3 v, vec3 f0, int index) {
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

	float shadow = ShadowCalculationPointlight(light, index);

	return (kd * sampled_albedo.xyz / PI + specular) * n_dot_l * (light.color.xyz / attenuation) * (1.0 - shadow);

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
	return max((kd * sampled_albedo.xyz / PI + specular) * n_dot_l * (light.color.xyz / attenuation) * spotlight_intensity, vec3(0.0, 0.0, 0.0)) * (1.0 - shadow);
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
		imageStore(u_output_texture, tex_coords, vec4(sampled_albedo.rgb, 1.0));
		return;
	}


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
		total_light += (CalcPointLight(point_lights.lights[i], v, f0, i));
	}

	// Spotlights
	for (int i = 0; i < spot_lights.lights.length(); i++) {
		total_light += (CalcSpotLight(spot_lights.lights[i], v, f0, i));
	}

	vec3 light_color = max(vec3(total_light), vec3(0.0, 0.0, 0.0));

	imageStore(u_output_texture, tex_coords, vec4(light_color * 3.0, 1.0));
};)""