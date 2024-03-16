ORNG_INCLUDE "CommonINCL.glsl"

layout(std140, binding = 1) buffer PointLights {
	PointLight lights[];
} ubo_point_lights;


layout(std140, binding = 2) buffer SpotLights {
	SpotLight lights[];
} ubo_spot_lights;

layout(std140, binding = 1) uniform GlobalLighting{
	DirectionalLight directional_light;
} ubo_global_lighting;

layout(std140, binding = 2) uniform commons{
	vec4 camera_pos;
	vec4 camera_target;
	vec4 camera_right;
	vec4 camera_up;
	vec4 voxel_aligned_cam_positions[4];
	float time_elapsed;
	float render_resolution_x;
	float render_resolution_y;
	float cam_zfar;
	float cam_znear;
	float delta_time;
	double time_elapsed_precise;
} ubo_common;

layout(std140, binding = 0) uniform Matrices{
	mat4 projection; 
	mat4 view; 
	mat4 proj_view;
	mat4 inv_projection;
	mat4 inv_view;
	mat4 inv_proj_view;
} PVMatrices;