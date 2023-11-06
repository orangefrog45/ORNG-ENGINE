ORNG_INCLUDE "BuffersINCL.glsl"

#ifndef DIR_DEPTH_SAMPLER
#error "No directional depth sampler defined for shadow calculations"
#endif

#ifndef SPOTLIGHT_DEPTH_SAMPLER
#error "No spotlight depth sampler defined for shadow calculations"
#endif

#ifndef POINTLIGHT_DEPTH_SAMPLER
#error "No pointlight depth sampler defined for shadow calculations"
#endif

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



vec2 CalcBlockerDistanceDirectional(vec3 proj_coord, vec2 texel_size, int sampler_index, vec3 world_pos) {
	float avg_blocker = 0.0f;
	int blockers = 0;

	const int pcss_sample_count = 16;
	float rnd = random(world_pos) * 2.0 * PI;

	for (int i = 0; i < pcss_sample_count; i++) {
		vec2 offset = vec2(
			cos(rnd) * poisson_disk_kernel_2[i].x - sin(rnd) * poisson_disk_kernel_2[i].y,
			sin(rnd) * poisson_disk_kernel_2[i].x + cos(rnd) * poisson_disk_kernel_2[i].y
		);

		float z = texture(DIR_DEPTH_SAMPLER, vec3(vec2(proj_coord.xy + offset * texel_size * ubo_global_lighting.directional_light.blocker_search_size), sampler_index)).r;

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



float ShadowCalculationSpotlight(SpotLight light, float light_index, vec3 world_pos) {
	vec4 frag_pos_light_space = light.light_transform_matrix * vec4(world_pos, 1.0);

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
	shadow += ((current_depth - adaptive_epsilon) > texture(SPOTLIGHT_DEPTH_SAMPLER, vec3(vec2(proj_coords.xy), light_index)).r) ? 1.0 : 0.0;

	return shadow;
}

float ShadowCalculationPointlight(PointLight light, int light_index, vec3 world_pos) {
	vec3 light_to_frag = world_pos - light.pos.xyz;
	float closest_depth = texture(POINTLIGHT_DEPTH_SAMPLER, vec4(light_to_frag, light_index)).r * light.max_distance; // Transform normalized depth to range 0, zFar
	float current_depth = length(light_to_frag);
	float bias = 0.1;
	float shadow = float((current_depth - bias) > closest_depth);
	return shadow;
}



float ShadowCalculationDirectional(vec3 light_dir, vec3 world_pos) {
	float shadow = 0.0f;
	//perspective division
	vec3 proj_coords = vec3(0);
	float frag_distance_from_cam = length(world_pos - ubo_common.camera_pos.xyz);

	int index = 3;
	index -= int(frag_distance_from_cam <= ubo_global_lighting.directional_light.cascade_ranges[2]);
	index -= int(frag_distance_from_cam <= ubo_global_lighting.directional_light.cascade_ranges[1]);
	index -= int(frag_distance_from_cam <= ubo_global_lighting.directional_light.cascade_ranges[0]);
	vec4 frag_pos_light_space = ubo_global_lighting.directional_light.light_space_transforms[index] * vec4(world_pos, 1);

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
	vec2 avg_blocker_distance = CalcBlockerDistanceDirectional(proj_coords, texel_size, index, world_pos);


	float penumbra_size = max(ubo_global_lighting.directional_light.size * (current_depth - avg_blocker_distance.x) / avg_blocker_distance.x, 1.0);

	float rnd = random(world_pos) * 2.0 * PI;
	for (int i = 0; i < 16; i++) {
		vec2 offset = vec2(
			cos(rnd) * poisson_disk_kernel[i].x - sin(rnd) * poisson_disk_kernel[i].y,
			sin(rnd) * poisson_disk_kernel[i].x + cos(rnd) * poisson_disk_kernel[i].y
		) * penumbra_size;

		float pcf_depth = texture(DIR_DEPTH_SAMPLER, vec3(vec2(proj_coords.xy + offset * texel_size), index)).r;
		shadow += current_depth > pcf_depth ? 1.0f : 0.0f;
	}

	/*Take average of PCF*/
	return shadow / 16.0;
}

