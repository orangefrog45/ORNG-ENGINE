#version 460 core

#define PI 3.1415926538

layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in; // invocations

layout(binding = 1, rgba32f) writeonly uniform image2D fog_texture;
layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 4) uniform sampler2DArray spot_depth_sampler;
layout(binding = 6) uniform sampler2D world_position_sampler;
layout(binding = 11) uniform sampler2D blue_noise_sampler;
layout(binding = 15) uniform sampler3D fog_noise_sampler;
layout(binding = 16) uniform sampler2D gbuffer_depth_sampler;

uniform float u_scattering_anistropy;
uniform float u_absorption_coef;
uniform float u_scattering_coef;
uniform float u_density_coef;
uniform float u_time;
uniform int u_step_count;
uniform vec3 u_fog_color;

layout(std140, binding = 2) uniform commons{
	vec4 camera_pos;
	vec4 camera_target;
	float time_elapsed;
} ubo_common;

struct BaseLight {
	vec4 color;
	float ambient_intensity;
	float diffuse_intensity;
};

struct DirectionalLight {
	vec4 direction;
	BaseLight base;

	//stored in vec3 instead of array due to easier alignment
	vec3 cascade_ranges;
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

layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
	mat4 proj_view;
} PVMatrices;

layout(std140, binding = 1) uniform GlobalLighting{
	DirectionalLight directional_light;
	BaseLight ambient_lighting;
} ubo_global_lighting;


layout(std140, binding = 1) buffer PointLights {
	PointLight lights[];
} point_lights;

layout(std140, binding = 2) buffer SpotLights {
	SpotLight lights[];
} spot_lights;

uniform mat4 u_dir_light_matrices[3];


float ShadowCalculationSpotlight(SpotLight light, float light_index, vec3 sampled_world_pos) {
	float shadow = 0.0f;
	vec4 frag_pos_light_space = light.light_transform_matrix * vec4(sampled_world_pos, 1.0);

	//perspective division
	vec3 proj_coords = (frag_pos_light_space / frag_pos_light_space.w).xyz;

	//depth map in range 0,1 proj in -1,1 (NDC), so convert
	proj_coords = proj_coords * 0.5 + 0.5;

	if (proj_coords.z > 1.0) {
		return 0.0; // early return, fragment out of range
	}

	float current_depth = proj_coords.z;

	float closest_depth = 0.0f;


	float sampled_depth = texture(spot_depth_sampler, vec3(proj_coords.xy, light_index)).r;
	shadow += current_depth > sampled_depth ? 1.0 : 0.0;

	return shadow;
}



float ShadowCalculationDirectional(vec3 light_dir, vec3 world_pos) {


	vec3 proj_coords = vec3(0);
	float frag_distance_from_cam = length(world_pos.xyz - ubo_common.camera_pos.xyz);

	if (frag_distance_from_cam > ubo_global_lighting.directional_light.cascade_ranges[2]) // out of range of cascades
		return 0.f;

	vec4 frag_pos_light_space = vec4(0);

	unsigned int depth_map_index = 0;

	if (frag_distance_from_cam <= ubo_global_lighting.directional_light.cascade_ranges[0]) { // cascades
		frag_pos_light_space = u_dir_light_matrices[0] * vec4(world_pos, 1);
	}
	else if (frag_distance_from_cam <= ubo_global_lighting.directional_light.cascade_ranges[1]) {
		frag_pos_light_space = u_dir_light_matrices[1] * vec4(world_pos, 1);
		depth_map_index = 1;
	}
	else if (frag_distance_from_cam <= ubo_global_lighting.directional_light.cascade_ranges[2]) {
		frag_pos_light_space = u_dir_light_matrices[2] * vec4(world_pos, 1);
		depth_map_index = 2;
	}

	proj_coords = (frag_pos_light_space / frag_pos_light_space.w).xyz;

	//depth map in range 0,1 proj in -1,1 (NDC), so convert
	proj_coords = proj_coords * 0.5f + 0.5f;

	if (proj_coords.z > 1.0f) {
		return 0.0f; // early return, fragment out of range
	}

	float current_depth = proj_coords.z;
	float sampled_depth = texture(dir_depth_sampler, vec3(vec2(proj_coords.xy), depth_map_index)).r;

	float shadow = current_depth > sampled_depth ? 1.0f : 0.0f;

	return shadow;
}



vec3 CalcPointLight(PointLight light, vec3 world_pos) {

	float distance = length(light.pos.xyz - world_pos);

	if (distance > light.max_distance)
		return vec3(0.0);

	vec3 diffuse_color = light.color.xyz * light.diffuse_intensity;

	float attenuation = light.constant +
		light.a_linear * distance +
		light.exp * pow(distance, 2);


	return diffuse_color / attenuation;
}


vec3 CalcSpotLight(SpotLight light, vec3 world_pos, uint index) {

	vec3 frag_to_light = light.pos.xyz - world_pos.xyz;
	float distance = length(frag_to_light);

	if (distance > light.max_distance)
		return vec3(0.0);

	vec3 frag_to_light_dir = normalize(frag_to_light);

	float spot_factor = dot(frag_to_light_dir, -light.dir.xyz);

	if (spot_factor > light.aperture) {
		float shadow = ShadowCalculationSpotlight(light, index, world_pos);

		if (shadow >= 0.99) {
			return vec3(0); // early return as no light will reach this spot
		}

		vec3 diffuse_color = light.color.xyz * light.diffuse_intensity * (1.0 - shadow);

		float attenuation = light.constant +
			light.a_linear * distance +
			light.exp * pow(distance, 2);

		diffuse_color /= attenuation;

		float spotlight_intensity = (1.0 - (1.0 - spot_factor) / (1.0 - light.aperture));
		return diffuse_color * spotlight_intensity;
	}
	else {
		return vec3(0, 0, 0);
	}
}



vec3 GetRayDirWorldSpace(vec2 tex_coords, float depth) {
	// Convert to clip-space coordinates
	vec3 ray_dir = vec3((tex_coords.x * 2.f - 1.f), (tex_coords.y * 2.f - 1.f), depth);
	ray_dir = (inverse(PVMatrices.projection) * vec4(ray_dir, 1.f)).xyz;
	ray_dir = normalize(inverse(mat3(PVMatrices.view)) * ray_dir);
	return ray_dir;
}

uniform float u_noise_coord_scale_fac;


float phase(vec3 march_dir, vec3 light_dir) {
	float cos_theta = dot(normalize(march_dir), normalize(light_dir));
	return (1.f / (4.f * PI)) * ((1 - pow(u_scattering_anistropy, 2)) / pow((1.f - u_scattering_anistropy * cos_theta), 2));
}

float noise(vec3 pos) {
	float f = texture(fog_noise_sampler, pos).r * u_density_coef;

	return f;
}

void main() {
	// Tex coords in range (0, 0), (screen width, screen height)
	ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy) * 2; //multiplication by 2 as fog texture at half resolution

	const float noise_coord_scale_factor = 0.5f;
	float noise_offset = texture(blue_noise_sampler, tex_coords.xy / (textureSize(blue_noise_sampler, 0) + 0.000001f) * noise_coord_scale_factor).r; // division as for some reason wrapping isn't working, so have to normalize coords manually
	vec3 frag_world_pos = texelFetch(world_position_sampler, tex_coords, 0).xyz;

	vec3 CameraComponent_to_frag = frag_world_pos - ubo_common.camera_pos.xyz;
	float CameraComponent_to_pos_dist = length(CameraComponent_to_frag);
	float step_distance = CameraComponent_to_pos_dist / float(u_step_count);


	float fragment_depth = texelFetch(gbuffer_depth_sampler, tex_coords, 0).r;
	vec3 ray_dir = normalize(frag_world_pos - ubo_common.camera_pos.xyz);
	vec3 step_pos = ubo_common.camera_pos.xyz + ray_dir * noise_offset * step_distance;


	float transmittance = 1.0;
	vec3 luminance = vec3(0.0f);
	float extinction_coef = u_absorption_coef + u_scattering_coef;

	// Raymarching
	for (int i = 0; i < u_step_count; i++) {
		vec3 fog_sampling_coords = vec3(step_pos.x, step_pos.y, step_pos.z) / 500.f;
		float fog_density = noise(fog_sampling_coords);
		fog_density += 0.5f * u_density_coef;
		fog_density *= exp(-max(step_pos.y * 0.001f, 1.f));


		vec3 in_scattering = vec3(0);
		in_scattering += ubo_global_lighting.ambient_lighting.ambient_intensity * ubo_global_lighting.ambient_lighting.color.xyz;

		for (unsigned int i = 0; i < point_lights.lights.length(); i++) {
			in_scattering += CalcPointLight(point_lights.lights[i], step_pos) * phase(ray_dir, normalize(point_lights.lights[i].pos.xyz - step_pos));
		}

		for (unsigned int i = 0; i < spot_lights.lights.length(); i++) {
			in_scattering += CalcSpotLight(spot_lights.lights[i], step_pos, i) * phase(ray_dir, -spot_lights.lights[i].dir.xyz);
		}

		float dir_shadow = ShadowCalculationDirectional(ubo_global_lighting.directional_light.direction.xyz, step_pos);
		in_scattering += ubo_global_lighting.directional_light.base.color.xyz * ubo_global_lighting.directional_light.base.diffuse_intensity * (1.0 - dir_shadow) * phase(ray_dir, ubo_global_lighting.directional_light.direction.xyz);

		float out_scattering = u_scattering_coef * fog_density;
		transmittance *= exp(-fog_density * extinction_coef * step_distance);
		luminance += transmittance * in_scattering * out_scattering * step_distance;

		step_pos += ray_dir * step_distance;
	}

	vec4 fog_color = vec4(u_fog_color * luminance, 1.0 - transmittance);

	imageStore(fog_texture, tex_coords / 2, fog_color);



}