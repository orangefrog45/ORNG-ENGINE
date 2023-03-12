#version 420 core
const int MAX_POINT_LIGHTS = 108;
const int MAX_SPOT_LIGHTS = 128;
const unsigned int POINT_LIGHT_BINDING = 1;
const unsigned int SPOT_LIGHT_BINDING = 2;


in vec2 TexCoord0;
in vec3 vs_position;
in vec3 vs_normal;
in vec4 gl_FragCoord;
in vec4 frag_pos_light_space;

out vec4 FragColor;

struct BaseLight {
	vec4 color;
	float ambient_intensity;
	float diffuse_intensity;
};

struct DirectionalLight {
	vec3 color;
	vec3 direction;
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
uniform BaseLight g_ambient_light; // ambient
uniform Material g_material;
uniform DirectionalLight directional_light;
uniform vec3 view_pos;

layout(binding = 1) uniform sampler2D gSampler;
layout(binding = 2) uniform sampler2D specular_sampler;
layout(binding = 3) uniform sampler2D shadow_map;
uniform bool specular_sampler_active; // FALSE IF NO SHININESS TEXTURE FOUND

float ShadowCalculation(vec4 t_frag_pos_light_space, vec3 light_dir) {
	float shadow = 0.0f;
	//perspective division
	vec3 proj_coords = (t_frag_pos_light_space / t_frag_pos_light_space.w).xyz;

	//depth map in range 0,1 proj in -1,1 (NDC), so convert
	proj_coords = proj_coords * 0.5 + 0.5;

	//sample closest depth from lights pov from depth map
	float closest_depth = texture(shadow_map, proj_coords.xy).r;

	//actual depth for comparison
	float current_depth = proj_coords.z;
	float bias = max(0.0005 * (1.0 - dot(normalize(vs_normal), light_dir)), 0.003);
	shadow = current_depth - bias > closest_depth ? 1.0 : 0.0;

	//make fragments out of shadow bounds appear not in shadow
	if (proj_coords.z > 1.0)
		shadow = 0.0;

	return shadow;
}

vec3 CalcPhongLight(vec3 light_color, float light_diffuse_intensity, vec3 normalized_light_dir, vec3 norm) {

	vec3 ambient_light = g_ambient_light.ambient_intensity * g_ambient_light.color.xyz * g_material.ambient_color;

	//diffuse
	float diffuse = clamp(dot(normalized_light_dir, norm), 0, 1);

	vec3 diffuse_final = vec3(0, 0, 0);
	vec3 specular_final = vec3(0, 0, 0);

	if (diffuse > 0) {
		diffuse_final = light_color * diffuse * g_material.diffuse_color * light_diffuse_intensity;

		float specular_strength = 0.5;
		vec3 view_dir = normalize(view_pos - vs_position);
		vec3 reflect_dir = reflect(-normalized_light_dir, norm);
		float specular_factor = dot(view_dir, reflect_dir);

		if (specular_factor > 0) {
			float specular_exponent = specular_sampler_active ? texture2D(specular_sampler, TexCoord0).r : 32;
			float spec = pow(specular_factor, specular_exponent);
			specular_final = specular_strength * spec * light_color * g_material.specular_color;
		}
	};

	float shadow = ShadowCalculation(frag_pos_light_space, normalized_light_dir);


	return (diffuse_final + specular_final) * (ambient_light + (1.0 - shadow));
}


vec3 CalcPointLight(PointLight light, vec3 normal, float distance) {
	vec3 light_dir = normalize(light.pos.xyz - vs_position);
	vec3 color = CalcPhongLight(light.color.xyz, light.diffuse_intensity, light_dir, normal);

	float attenuation = light.constant +
		light.a_linear * distance +
		light.exp * pow(distance, 2);

	return color / attenuation;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, float distance) {
	vec3 light_to_pixel_dir = normalize(light.pos.xyz - vs_position);
	float spot_factor = dot(light_to_pixel_dir, light.dir.xyz);

	if (spot_factor > light.aperture) {
		vec3 color = CalcPhongLight(light.color.xyz, light.diffuse_intensity, light_to_pixel_dir, normal);

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

	vec3 ambient_light = g_ambient_light.ambient_intensity * g_ambient_light.color.xyz * g_material.ambient_color;


	//pointlights
	for (int i = 0; i < g_num_point_lights; i++) {
		float distance = length(point_lights.lights[i].pos.xyz - vs_position);
		if (distance <= point_lights.lights[i].max_distance) {
			total_light += (CalcPointLight(point_lights.lights[i], normal, distance));
		}
	}

	//spotlights
	for (int i = 0; i < g_num_spot_lights; i++) {
		float distance = length(spot_lights.lights[i].pos.xyz - vs_position);
		if (distance <= spot_lights.lights[i].max_distance) {
			total_light += (CalcSpotLight(spot_lights.lights[i], normal, distance));
		}
	}

	//directional light
	total_light += CalcPhongLight(directional_light.color, directional_light.diffuse_intensity, directional_light.direction, normal) + ambient_light;


	vec3 color = max(vec3(total_light), vec3(0.0, 0.0, 0.0));

	FragColor = vec4(color.xyz, 1.0) * texture2D(gSampler, TexCoord0);


};