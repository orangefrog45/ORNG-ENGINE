#version 460 core

layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in; // invocations

layout(rgba32f, binding = 1) uniform image2D fog_texture;
layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 6) uniform sampler2D world_position_sampler;
layout(binding = 11) uniform sampler2D blue_noise_sampler;
layout(binding = 15) uniform sampler3D fog_noise_sampler;

uniform vec3 u_camera_pos;
uniform float u_fog_hardness;
uniform float u_fog_intensity;
uniform float u_time;
uniform vec3 u_fog_color;



struct BaseLight {
	vec4 color;
	float ambient_intensity;
	float diffuse_intensity;
};

struct DirectionalLight {
	vec4 direction;
	BaseLight base;
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

uniform DirectionalLight u_directional_light;
uniform mat4 u_dir_light_matrices[3];


float ShadowCalculationDirectional(vec3 light_dir, vec3 world_pos) {
	float shadow = 0.0f;
	//perspective division
	vec3 proj_coords = vec3(0);
	float frag_distance_from_cam = length(world_pos.xyz - u_camera_pos);

	if (frag_distance_from_cam > 1200.f) // out of range of cascades
		return 0.f;

	vec4 frag_pos_light_space = vec4(0);

	unsigned int depth_map_index = 0;

	if (frag_distance_from_cam <= 200.f) { // cascades
		frag_pos_light_space = u_dir_light_matrices[0] * vec4(world_pos, 1);
	}
	else if (frag_distance_from_cam <= 500.f) {
		frag_pos_light_space = u_dir_light_matrices[1] * vec4(world_pos, 1);
		depth_map_index = 1;
	}
	else if (frag_distance_from_cam <= 1200.f) {
		frag_pos_light_space = u_dir_light_matrices[2] * vec4(world_pos, 1);
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
	//bias = 0.001f * tan(acos(clamp(dot(normalize(sampled_normal), light_dir), 0, 1))); // slope bias

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

		//Take average of PCF
		shadow /= 9.0f;
	}
	else {
		float sampled_depth = texture(dir_depth_sampler, vec3(vec2(proj_coords.xy), depth_map_index)).r;
		shadow = current_depth - bias > sampled_depth ? 1.0f : 0.0f;
	}

	return shadow;
}



vec3 CalcPhongLight(PointLight light, vec3 world_pos) {

	float distance = length(light.pos.xyz - world_pos);

	if (distance > light.max_distance)
		return vec3(0.0);


	vec3 diffuse_final = vec3(0, 0, 0);

	diffuse_final = light.color.xyz * light.diffuse_intensity;

	float attenuation = light.constant +
		light.a_linear * distance +
		light.exp * pow(distance, 2);


	return diffuse_final / attenuation;
}

vec3 GetRayDirWorldSpace(vec2 tex_coords) {
	vec3 ray_dir = vec3((tex_coords.x * 2.f - 1.f), (tex_coords.y * 2.f - 1.f), 1.f);
	ray_dir = normalize(inverse(mat3(PVMatrices.proj_view)) * ray_dir);
	return ray_dir;
}




void main() {
	ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy) * 2; //multiplication by 2 as fog texture at half resolution

	const float noise_coord_scale_factor = 0.5f; //tweaking this changes the visibility of the noise, sweet spot seems to be 0.1-1.0
	float noise_offset = texture(blue_noise_sampler, (vec2(tex_coords.xy) / (textureSize(blue_noise_sampler, 0) + 0.001f) * noise_coord_scale_factor)).r; // division as for some reason wrapping isn't working, so have to normalize coords manually

	vec3 average_color = vec3(0.0f);
	vec3 frag_world_pos = texelFetch(world_position_sampler, tex_coords, 0).xyz;
	const float max_fog_step_range = 1000.f;
	int step_count = 24;
	float camera_to_pos_dist = length(frag_world_pos - u_camera_pos);
	float step_distance = camera_to_pos_dist / step_count;

	float total_fog = 0.f;
	float average_fog_density = 0.f;

	//float step_distance = max_fog_step_range / step_count;

	vec3 ray_dir = GetRayDirWorldSpace(tex_coords / (textureSize(world_position_sampler, 0) + 0.001f)); // division to convert tex coords to NDC for raycasting calc
	ray_dir = normalize(frag_world_pos - u_camera_pos);
	vec3 step_pos = u_camera_pos + ray_dir * noise_offset * step_distance;
	float average_shadow = 0.f;

	for (int i = 0; i < step_count; i++) {
		vec3 fog_sampling_coords = vec3(step_pos.x + u_time * 30.f, step_pos.y, step_pos.z + u_time * 30.f) / 2000.f;
		float fog_density = (texture(fog_noise_sampler, fog_sampling_coords).r + 1.0f) / max(log(step_pos.y * 0.025), 1) * 10;
		//float height = 300.f;
		//float fog_density = max(-exp(step_pos.y * u_fog_hardness) + height, 0) / height;

		for (unsigned int i = 0; i < point_lights.lights.length(); i++) {
			average_color += CalcPhongLight(point_lights.lights[i], step_pos) * fog_density;
		}

		average_shadow += ShadowCalculationDirectional(ubo_global_lighting.directional_light.direction.xyz, step_pos);
		total_fog += (fog_density * u_fog_intensity);

		//total_shadow += ShadowCalculationDirectional(normalize(vec3(0.0f, 0.5f, 0.5f)), step_pos);
		step_pos += ray_dir * step_distance;
	}

	average_fog_density = (total_fog / step_count);
	//average_fog_density += camera_to_pos_dist * 0.00015; //linear distance fog
	total_fog = average_fog_density * log(camera_to_pos_dist);


	float fog_exp = 1.0 - exp(-total_fog);
	average_color /= step_count;
	average_color += (ubo_global_lighting.directional_light.base.color.xyz * ubo_global_lighting.directional_light.base.diffuse_intensity) * (1 - average_shadow / step_count);
	vec4 fog_color = vec4(u_fog_color + average_color, fog_exp);
	imageStore(fog_texture, tex_coords / 2, fog_color);

}