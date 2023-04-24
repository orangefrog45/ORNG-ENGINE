#version 430 core

const int MAX_SPOT_LIGHTS = 8;
const int MAX_POINT_LIGHTS = 8;

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 TexCoord;
in layout(location = 2) vec3 vertex_normal;
in layout(location = 3) mat4 transform;
in layout(location = 7) vec3 tangent;

struct DirectionalLight {
	vec3 color;
	vec3 direction;
	float ambient_intensity;
	float diffuse_intensity;
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

layout(std140, binding = 1) uniform PointLights{
	PointLight lights[MAX_POINT_LIGHTS];
} point_lights;

layout(std140, binding = 2) uniform SpotLights{
	SpotLight lights[MAX_SPOT_LIGHTS];
} spot_lights;


layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
	mat4 proj_view;
} PVMatrices;


uniform int g_num_spot_lights;
uniform int g_num_point_lights;
uniform vec3 view_pos;
uniform mat4 dir_light_matrix;
uniform DirectionalLight directional_light;
uniform bool terrain_mode;
uniform bool normal_sampler_active;


out TangentSpacePositions{
	vec3 view_pos;
	vec3 frag_pos;
	vec3 dir_light_dir;
	vec3 spot_positions[MAX_SPOT_LIGHTS];
	vec3 point_positions[MAX_POINT_LIGHTS];
} vs_out_tangent_space;

out vec2 tex_coord0;
out vec4 vs_position;
out vec3 vs_normal;
out vec4 dir_light_frag_pos_light_space;


void main() {

	gl_Position = (PVMatrices.proj_view * transform) * vec4(position, 1.0);
	vs_normal = transpose(inverse(mat3(transform))) * vertex_normal;
	vs_position = terrain_mode ? vec4(position, 1.0f) : transform * vec4(position, 1.0f);
	dir_light_frag_pos_light_space = dir_light_matrix * vs_position;
	tex_coord0 = TexCoord;

	if (normal_sampler_active) {
		vec3 t = normalize(vec3(transform * vec4(tangent, 0.0)));
		vec3 n = normalize(vec3(transform * vec4(vs_normal, 0.0)));

		t = normalize(t - dot(t, n) * n);
		vec3 b = normalize(vec3(transform * vec4(cross(vs_normal, tangent), 0.0)));

		mat3 tbn = transpose(mat3(t, b, n));
		vs_out_tangent_space.view_pos = tbn * view_pos;
		vs_out_tangent_space.frag_pos = tbn * vec3(vs_position);

		vs_out_tangent_space.dir_light_dir = tbn * normalize(directional_light.direction);

		for (int i = 0; i < g_num_spot_lights; i++) {
			vs_out_tangent_space.spot_positions[i] = tbn * spot_lights.lights[i].pos.xyz;
		}

		for (int i = 0; i < g_num_point_lights; i++) {
			vs_out_tangent_space.point_positions[i] = tbn * point_lights.lights[i].pos.xyz;
		}


	}
}
