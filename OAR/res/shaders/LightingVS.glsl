R""(#version 430 core

const int MAX_SPOT_LIGHTS = 8;
const int MAX_POINT_LIGHTS = 8;
const unsigned int NUM_SHADOW_CASCADES = 3;

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 TexCoord;


/*struct DirectionalLight {
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
uniform mat4 dir_light_matrices[NUM_SHADOW_CASCADES];
uniform DirectionalLight directional_light;
uniform bool terrain_mode;
uniform bool normal_sampler_active;



*/




out vec2 tex_coord;


out vec4 dir_frag_pos_light_space_array[NUM_SHADOW_CASCADES]; // normal mapping

void main() {

	gl_Position = vec4(position, 1.0);

	tex_coord = TexCoord;


}
)""