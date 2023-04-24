#version 430 core

out vec4 FragColor;

in vec2 tex_coords;

layout(binding = 1) uniform sampler2D quad_sampler;
layout(binding = 6) uniform sampler2D world_position_sampler;

uniform vec3 camera_pos;
uniform float time;


void main() {
	vec3 frag_world_pos = texture(world_position_sampler, tex_coords).xyz;
	vec3 camera_to_pos = frag_world_pos - camera_pos;
	float camera_to_pos_dist = length(camera_to_pos);

	float adj_y = frag_world_pos.y;
	float adj_cy = camera_pos.y;

	float total_fog = 0;
	vec3 step_pos = frag_world_pos;

	float step_count = clamp(camera_to_pos_dist, 1.f, 100.f);

	for (int i = 0; i < step_count; i++) {
		total_fog += exp(-step_pos.y * 0.04f) * 0.0001f * length(step_pos - camera_pos);
		step_pos += -camera_to_pos * 0.01f;
	}

	//float total_fog = (abs(integrated_y - integrated_yc)) * 100.0;
	total_fog += abs(frag_world_pos.x - camera_pos.x) * 0.00015; //linear distance fog
	total_fog += abs(frag_world_pos.z - camera_pos.z) * 0.00015; //linear distance fog

	float fog_exp = 1.0 - exp(-total_fog);

	float gamma = 2.2;
	vec4 fog_mix_color = mix(texture(quad_sampler, tex_coords), (vec4(0.7, 0.9, 1.0, 1)), 0);
	FragColor = vec4(pow(fog_mix_color.rgb, vec3(1.0 / gamma)), 1.0);

}