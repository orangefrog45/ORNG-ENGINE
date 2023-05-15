#version 430 core

out vec4 FragColor;

in vec2 tex_coords;

layout(binding = 1) uniform sampler2D quad_sampler;
layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 6) uniform sampler2D world_position_sampler;
layout(binding = 11) uniform sampler2D blue_noise_sampler;

uniform vec3 camera_pos;
uniform float time;



void main() {
	vec3 frag_world_pos = texture(world_position_sampler, tex_coords).xyz;
	vec3 camera_to_pos = frag_world_pos - camera_pos;
	float camera_to_pos_dist = length(camera_to_pos);


	float step_count = 16.f;//clamp to stop banding at low step counts
	float step_distance = camera_to_pos_dist / step_count;

	vec3 step_pos = camera_pos + (normalize(camera_to_pos) * step_distance); // give initial offset to smooth banding

	float fog_hardness = 0.01f;
	float fog_intensity = 0.001f;
	float total_fog = 0;
	float average_fog_density = 0;

	float old_fog_density = 0;

	for (int i = 0; i < step_count; i++) {
		float fog_density = max(exp(-step_pos.y * fog_hardness), 0);

		old_fog_density = fog_density;

		total_fog += (fog_density * fog_intensity);

		step_pos += normalize(camera_to_pos) * step_distance;
	}

	average_fog_density = ((total_fog) / step_count);
	total_fog = average_fog_density * (camera_to_pos_dist + fract(step_count));

	//float total_fog = (abs(integrated_y - integrated_yc)) * 100.0;
	//total_fog += camera_to_pos.x * 0.0015; //linear distance fog
	//total_fog += camera_to_pos_dist * 0.00015; //linear distance fog

	float fog_exp = 1.0 - exp(-total_fog);

	float gamma = 2.2;
	vec4 fog_mix_color = mix(texture(quad_sampler, tex_coords), (vec4(0.8, 0.9, 1.0, 1)), clamp(0, 0.f, 1.f));
	//FragColor = vec4(texture(dir_depth_sampler, vec3(tex_coords, 0)).rrr, 1);
	FragColor = vec4(pow(fog_mix_color.rgb, vec3(1.0 / gamma)), 1.0);

}