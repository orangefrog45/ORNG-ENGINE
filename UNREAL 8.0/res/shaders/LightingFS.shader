#version 420 core
const int MAX_POINT_LIGHTS = 108;
const int MAX_SPOT_LIGHTS = 128;
const unsigned int POINT_LIGHT_BINDING = 1;
const unsigned int SPOT_LIGHT_BINDING = 2;


in vec2 TexCoord0;
in vec3 vs_position;
in vec3 vs_normal;

out vec4 FragColor;

struct BaseLight {
	vec4 color;
	float ambient_intensity;
	float diffuse_intensity;
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

struct SpotLight { //76 BYTES
	vec4 color; //vec4s used to prevent implicit padding
	vec4 pos;
	vec4 dir;
	float ambient_intensity;
	float diffuse_intensity;
	float max_distance;
	float constant;
	float a_linear;
	float exp;
	float aperture;
	//attenuation
};
layout(std140, binding = 1) uniform PointLights{
	PointLight lights[MAX_POINT_LIGHTS];
} point_lights;

layout(std140, binding = 2) uniform SpotLights{
	SpotLight lights[MAX_SPOT_LIGHTS];
} spot_lights;


struct Material {
	vec3 ambient_color;
	vec3 diffuse_color;
	vec3 specular_color;
};

uniform int g_num_point_lights;
uniform int g_num_spot_lights;
uniform SpotLight g_spot_lights[MAX_SPOT_LIGHTS];
uniform BaseLight g_ambient_light; // ambient
uniform Material g_material;
uniform vec3 view_pos;

uniform sampler2D gSampler;
uniform sampler2D specular_sampler;
uniform bool specular_sampler_active; // FALSE IF NO SHININESS TEXTURE FOUND

vec3 CalcPhongLight(vec3 light_color, float light_diffuse_intensity, vec3 light_pos, vec3 norm) {
	//diffuse

	vec3 pos_to_light_dir_vec = normalize(light_pos - vs_position);
	float diffuse = clamp(dot(pos_to_light_dir_vec, norm), 0, 1);

	vec3 diffuse_final = vec3(0, 0, 0);
	vec3 specular_final = vec3(0, 0, 0);

	if (diffuse > 0) {
		diffuse_final = light_color * diffuse * g_material.diffuse_color * light_diffuse_intensity;

		float specular_strength = 0.5;
		vec3 view_dir = normalize(view_pos - vs_position);
		vec3 reflect_dir = reflect(-pos_to_light_dir_vec, norm);
		float specular_factor = dot(view_dir, reflect_dir);

		if (specular_factor > 0) {
			float specular_exponent = specular_sampler_active ? texture2D(specular_sampler, TexCoord0).r : 32;
			float spec = pow(specular_factor, specular_exponent);
			specular_final = specular_strength * spec * light_color * g_material.specular_color;
		}
	};

	return (diffuse_final + specular_final);
}


vec3 CalcPointLight(PointLight light, vec3 normal, float distance) {
	vec3 color = CalcPhongLight(light.color.xyz, light.diffuse_intensity, light.pos.xyz, normal);

	float attenuation = light.constant +
		light.a_linear * distance +
		light.exp * pow(distance, 2);

	return color / attenuation;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, float distance) {
	vec3 light_to_pixel_dir = normalize(vs_position - light.pos.xyz);
	float spot_factor = dot(light_to_pixel_dir, light.dir.xyz);

	if (spot_factor > light.aperture) {
		vec3 color = CalcPhongLight(light.color.xyz, light.diffuse_intensity, light.pos.xyz, normal);

		float attenuation = light.constant +
			light.a_linear * distance +
			light.exp * pow(distance, 2);

		color /= attenuation;
		float spotlight_intensity = (1.0 - (1.0 - spot_factor) / (1.0 - light.aperture));
		return color * spotlight_intensity;
	}
	else {
		return vec3(0, 0, 0);
	}
}

void main()
{
	vec3 normal = normalize(vs_normal);
	vec3 total_light = vec3(0.0, 0.0, 0.0);
	vec3 ambient_light = g_ambient_light.color.xyz * g_ambient_light.ambient_intensity * g_material.ambient_color;

	for (int i = 0; i < g_num_point_lights; i++) {
		float distance = length(point_lights.lights[i].pos.xyz - vs_position);
		if (distance <= point_lights.lights[i].max_distance) {
			total_light += (CalcPointLight(point_lights.lights[i], normal, distance));
		}
	}

	for (int i = 0; i < g_num_spot_lights; i++) {
		float distance = length(spot_lights.lights[i].pos.xyz - vs_position);
		if (distance <= spot_lights.lights[i].max_distance) {
			total_light += (CalcSpotLight(spot_lights.lights[i], normal, distance));
		}
	}

	vec3 color = max(vec3(total_light), vec3(0.0, 0.0, 0.0)) + ambient_light;

	FragColor = vec4(color.xyz, 1.0) * texture2D(gSampler, TexCoord0);
};