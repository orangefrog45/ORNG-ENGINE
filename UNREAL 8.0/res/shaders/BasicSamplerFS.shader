#version 430 core

out vec4 FragColor;

in vec2 tex_coords;

layout(binding = 1) uniform sampler2D texture1;
layout(binding = 3) uniform sampler2D depth_map;
layout(binding = 6) uniform sampler2D world_positions;

uniform vec3 camera_pos;
uniform vec3 normalized_camera_dir;


void main() {
	//float color = texture(texture1, vec3(tex_coords, 0.0)).r;
	float depth_map_z = texture(depth_map, tex_coords).r;
	vec3 frag_world_pos = texture(world_positions, tex_coords).xyz;
	vec3 camera_to_pos = frag_world_pos - camera_pos;
	float camera_to_pos_dist = length(camera_to_pos);

	float y_ref = 80 / exp2(frag_world_pos.y); // fog density functions, only used as a reference for the integral solution

	float fx = 1.0;
	float fxc = 1.0;

	float adj_y = max(frag_world_pos.y, 10.0);
	float adj_cy = max(camera_pos.y, 10.0);

	float integrated_y = -20.0 / adj_y;
	float integrated_yc = -20.0 / adj_cy;


	float total_fog = (abs(integrated_y - integrated_yc)) * 2.0; //analytical solution of integral, modified by some constant
	total_fog += abs(frag_world_pos.x - camera_pos.x) * 0.001; //simple linear distance-based fog

	//total_fog += exp(-max(abs(camera_pos.y - frag_world_pos.y), 0.5)) * 1;
	float fog_exp = 1.0 - exp(-total_fog);


	FragColor = mix(texture(texture1, tex_coords), (vec4(1, 0.0, 0.0, 1)), 0);
	//FragColor = vec4(vec3(1.0 - texture(screen_texture, tex_coords)), 1.0); - colour inversion
}