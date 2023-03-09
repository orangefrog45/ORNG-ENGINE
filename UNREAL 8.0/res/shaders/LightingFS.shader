#version 420 core
const int MAX_POINT_LIGHTS = 112;
const int MAX_SPOT_LIGHTS = 1;
const unsigned int POINT_LIGHT_BINDING = 1;


in vec2 TexCoord0;
in vec3 vs_position;
in vec3 vs_normal;

out vec4 FragColor;

struct BaseLight {
	vec3 color;
	float ambient_intensity;
	float diffuse_intensity;
};

struct Attenuation {
	float constant;
	float a_linear;
	float exp;
};

struct PointLight {
	BaseLight base;
	vec3 pos;
	float max_distance;
	Attenuation atten;
};

//layout(std140, binding = POINT_LIGHT_BINDING) PointLights { // 56 BYTES
	//PointLight lights[MAX_POINT_LIGHTS];
//} PointLights;

struct SpotLight {
	PointLight base;
	vec3 dir;
	float aperture;
};


struct Material {
	vec3 ambient_color;
	vec3 diffuse_color;
	vec3 specular_color;
};

uniform int g_num_point_lights;
uniform PointLight g_point_lights[MAX_POINT_LIGHTS];
uniform int g_num_spot_lights;
uniform SpotLight g_spot_lights[MAX_SPOT_LIGHTS];
uniform BaseLight g_ambient_light; // ambient
uniform Material g_material;
uniform vec3 view_pos;

uniform sampler2D gSampler;
uniform sampler2D specular_sampler;
uniform bool specular_sampler_active; // FALSE IF NO SHININESS TEXTURE FOUND

vec3 CalcPhongLight(PointLight p_light, vec3 norm) {
	//diffuse

	vec3 pos_to_light_dir_vec = normalize(p_light.pos - vs_position);
	float diffuse = clamp(dot(pos_to_light_dir_vec, norm), 0, 1);

	vec3 diffuse_final = vec3(0, 0, 0);
	vec3 specular_final = vec3(0, 0, 0);

	if (diffuse > 0) {
		diffuse_final = p_light.base.color * diffuse * g_material.diffuse_color * p_light.base.diffuse_intensity;

		float specular_strength = 0.5;
		vec3 view_dir = normalize(view_pos - vs_position);
		vec3 reflect_dir = reflect(-pos_to_light_dir_vec, norm);
		float specular_factor = dot(view_dir, reflect_dir);

		if (specular_factor > 0) {
			float specular_exponent = specular_sampler_active ? texture2D(specular_sampler, TexCoord0).r : 32;
			float spec = pow(specular_factor, specular_exponent);
			specular_final = specular_strength * spec * p_light.base.color * g_material.specular_color;
		}
	};

	return (diffuse_final + specular_final);
}


vec3 CalcPointLight(PointLight light, vec3 normal) {
	vec3 color = CalcPhongLight(light, normal);
	float distance = length(light.pos - vs_position);

	float attenuation = light.atten.constant +
		light.atten.a_linear * distance +
		light.atten.exp * pow(distance, 2);

	return color / attenuation;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal) {
	vec3 light_to_pixel_dir = normalize(vs_position - light.base.pos);
	float spot_factor = dot(light_to_pixel_dir, light.dir);

	if (spot_factor > light.aperture) {
		vec3 color = CalcPointLight(light.base, normal);
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
	vec3 ambient_light = g_ambient_light.color * g_ambient_light.ambient_intensity * g_material.ambient_color;

	//THESE TWO LOOPS NEED TO BE OPTIMISED
	for (int i = 0; i < g_num_point_lights; i++) {
		float distance = length(g_point_lights[i].pos - vs_position);
		if (distance <= g_point_lights[i].max_distance) {
			total_light += (CalcPointLight(g_point_lights[i], normal));
		}
	}

	for (int i = 0; i < g_num_spot_lights; i++) {
		total_light += (CalcSpotLight(g_spot_lights[i], normal));
	}

	vec3 color = max(vec3(total_light), vec3(0.0, 0.0, 0.0)) + ambient_light;

	FragColor = vec4(color.xyz, 1.0) * texture2D(gSampler, TexCoord0);
};