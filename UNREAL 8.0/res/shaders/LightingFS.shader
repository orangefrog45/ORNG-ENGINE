#version 430 core
const int MAX_POINT_LIGHTS = 8;
const int MAX_SPOT_LIGHTS = 8;

const unsigned int POINT_LIGHT_BINDING = 1;
const unsigned int SPOT_LIGHT_BINDING = 2;

//LIGHT IDENTIFIERS
const unsigned int LIGHT_TYPE_DIRECTIONAL = 0;
const unsigned int LIGHT_TYPE_SPOTLIGHT = 1;

in LightTransformedPositions{
	vec4 spot_light_positions[MAX_SPOT_LIGHTS];
} in_light_positions;

in vec2 TexCoord0;
in vec4 vs_position;
in vec3 vs_normal;
in vec4 gl_FragCoord;
in vec4 dir_light_frag_pos_light_space;

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
layout(binding = 3) uniform sampler2D dir_depth_map;
layout(binding = 4) uniform sampler2DArray spot_depth_map;

uniform bool specular_sampler_active; // FALSE IF NO SHININESS TEXTURE FOUND

float ShadowCalculation(vec4 t_frag_pos_light_space, vec3 light_dir, int depth_map_index, unsigned int type) {
	float shadow = 0.0f;
	//perspective division
	vec3 proj_coords = (t_frag_pos_light_space / t_frag_pos_light_space.w).xyz;

	//depth map in range 0,1 proj in -1,1 (NDC), so convert
	proj_coords = proj_coords * 0.5 + 0.5;

	//make fragments out of shadow bounds appear not in shadow
	if (proj_coords.z > 1.0) {
		return 0.0; // early return, fragment out of range
	}

	//actual depth for comparison
	float current_depth = proj_coords.z;


	float closest_depth = 0.0f;

	//dir light bias
	//sample closest depth from lights pov from depth map
	float bias = 0.0f;
	if (type == LIGHT_TYPE_DIRECTIONAL) {
		bias = 0.001 * tan(acos(clamp(dot(normalize(vs_normal), light_dir), 0, 1)));

		vec2 texel_size = 1.0 / textureSize(dir_depth_map, 0);
		for (int x = -1; x <= 1; ++x)
		{
			for (int y = -1; y <= 1; ++y)
			{
				float pcf_depth = texture(dir_depth_map, proj_coords.xy + vec2(x, y) * texel_size * 2).r;
				shadow += current_depth - bias > pcf_depth ? 1.0 : 0.0;
			}
		}
	}
	else {
		bias = max(0.00001f * (1.0f - dot(normalize(vs_normal), light_dir)), 0.0000005f);


		vec2 texel_size = 1.0 / textureSize(spot_depth_map, 0).xy;
		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				float pcf_depth = texture(spot_depth_map, vec3(vec2(proj_coords.xy + vec2(x, y) * texel_size * 2), depth_map_index)).r;
				shadow += current_depth - bias > pcf_depth ? 1.0 : 0.0;
			}
		}
	}


	/*Take average of PCF*/
	shadow /= 9.0;

	return shadow;
}

vec3 CalcPhongLight(vec3 light_color, float light_diffuse_intensity, vec3 normalized_light_dir, vec3 norm) {

	//diffuse
	float diffuse = clamp(dot(normalized_light_dir, norm), 0, 1);

	vec3 diffuse_final = vec3(0, 0, 0);
	vec3 specular_final = vec3(0, 0, 0);

	if (diffuse > 0) {
		diffuse_final = light_color * diffuse * light_diffuse_intensity; // ADD BACK MATERIAL DIFFUSE COLOR AT SOME STAGE

		float specular_strength = 0.5;
		vec3 view_dir = normalize(view_pos - vs_position.xyz);
		vec3 reflect_dir = reflect(-normalized_light_dir, norm);
		float specular_factor = dot(view_dir, reflect_dir);

		if (specular_factor > 0) {
			float specular_exponent = specular_sampler_active ? texture2D(specular_sampler, TexCoord0).r : 64;
			float spec = pow(specular_factor, specular_exponent);
			specular_final = specular_strength * spec * light_color * g_material.specular_color;
		}
	};


	return (diffuse_final + specular_final);
}


vec3 CalcPointLight(PointLight light, vec3 normal, float distance) {
	vec3 light_dir = normalize(light.pos.xyz - vs_position.xyz);
	vec3 color = CalcPhongLight(light.color.xyz, light.diffuse_intensity, light_dir, normal);

	float attenuation = light.constant +
		light.a_linear * distance +
		light.exp * pow(distance, 2);

	return color / attenuation;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, float distance, int index) {
	vec3 pixel_to_light_dir = normalize(light.pos.xyz - vs_position.xyz);
	float spot_factor = dot(pixel_to_light_dir, -light.dir.xyz);

	if (spot_factor > light.aperture) {
		//SHADOW
		vec3 color = vec3(0);
		vec4 light_space_pos = light.light_transform_matrix * vs_position; // ----------------OPTIMIZE-------------------
		float shadow = ShadowCalculation(light_space_pos, light.dir.xyz, index, LIGHT_TYPE_SPOTLIGHT);

		if (shadow == 1.0) {
			return color; // early return as no light will reach this spot
		}

		color = CalcPhongLight(light.color.xyz, light.diffuse_intensity, pixel_to_light_dir, normal) * (1 - shadow);


		//ATTENUATION
		float attenuation = light.constant +
			light.a_linear * distance +
			light.exp * pow(distance, 2);

		color /= attenuation;

		//SPOTLIGHT APERTURE
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
		float distance = length(point_lights.lights[i].pos.xyz - vs_position.xyz);
		if (distance <= point_lights.lights[i].max_distance) {
			total_light += (CalcPointLight(point_lights.lights[i], normal, distance));
		}
	}

	//spotlights
	for (int i = 0; i < g_num_spot_lights; i++) {
		float distance = length(spot_lights.lights[i].pos.xyz - vs_position.xyz);
		if (distance <= spot_lights.lights[i].max_distance) {
			total_light += (CalcSpotLight(spot_lights.lights[i], normal, distance, i));
		}
	}

	//directional light
	float shadow = ShadowCalculation(dir_light_frag_pos_light_space, directional_light.direction.xyz, 0, LIGHT_TYPE_DIRECTIONAL);


	total_light += CalcPhongLight(directional_light.color, directional_light.diffuse_intensity, normalize(directional_light.direction), normal) * (1 - shadow);

	total_light += ambient_light;


	vec3 color = max(vec3(total_light), vec3(0.0, 0.0, 0.0));

	FragColor = vec4(color.xyz, 1.0) * texture2D(gSampler, TexCoord0);


};