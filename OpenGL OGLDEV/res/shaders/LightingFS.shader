#version 330 core

in vec2 TexCoord0;
in vec3 vs_position;
in vec3 vs_normal;

out vec4 FragColor;

struct BaseLight {
	vec3 color;
	float ambient_intensity;
};

struct PointLight {
	vec3 color;
	vec3 pos;
};

struct Material {
	vec3 ambient_color;
};

uniform PointLight g_point_light;
uniform BaseLight g_light;
//uniform Material g_material;


uniform sampler2D gSampler;

void main()
{
	//ambient
	vec4 ambient_light = vec4(g_light.color, 1.0f) * g_light.ambient_intensity;
	//diffuse
	vec3 pos_to_light_dir_vec = normalize(g_point_light.pos - vs_position);
	float diffuse = clamp(dot(pos_to_light_dir_vec, vs_normal), 0, 1);
	vec3 diffuse_final = g_point_light.color * diffuse;

	FragColor = texture2D(gSampler, TexCoord0) * vec4(g_light.color, 1.0f) * g_light.ambient_intensity + vec4(diffuse_final, 1.0f);

	//FragColor = texture2D(gSampler, TexCoord0) * vec4(g_material.ambient_color, 1.0f) * vec4(g_light.color, 1.0f) * g_light.ambient_intensity;
};