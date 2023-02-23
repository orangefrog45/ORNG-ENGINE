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
uniform BaseLight g_light; // ambient
uniform Material g_material;
uniform vec3 view_pos;


uniform sampler2D gSampler;

void main()
{

	//ambient
	vec3 ambient_light = g_light.color * g_light.ambient_intensity;

	//diffuse
	vec3 norm = normalize(vs_normal);
	vec3 pos_to_light_dir_vec = normalize(g_point_light.pos - vs_position);
	float diffuse = clamp(dot(pos_to_light_dir_vec, norm), 0, 1);
	vec3 diffuse_final = g_point_light.color * diffuse * g_material.ambient_color;

	//specular
	float specular_strength = 0.5;
	vec3 view_dir = normalize(view_pos - vs_position);
	vec3 reflect_dir = reflect(-pos_to_light_dir_vec, norm);
	float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
	vec3 specular = specular_strength * spec * g_point_light.color;

	vec3 result = (ambient_light + diffuse_final + specular);

	FragColor = vec4(result, 1.0) * texture2D(gSampler, TexCoord0);

};