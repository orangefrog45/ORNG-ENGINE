#version 430 core

ORNG_INCLUDE "UtilINCL.glsl"


#define PI 3.1415926538
#define MAX_REFLECTION_LOD 4.0
#define LIGHT_ZNEAR 0.01 // true for all light types

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

const unsigned int NUM_SHADOW_CASCADES = 3;
ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy);


layout(binding = 1) uniform sampler2D albedo_sampler;
layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 4) uniform sampler2DArray spot_depth_sampler;
layout(binding = 16) uniform sampler2D view_depth_sampler;
layout(binding = 7) uniform sampler2D normal_sampler;
layout(binding = 11) uniform sampler2D blue_noise_sampler;
layout(binding = 12) uniform usampler2D shader_id_sampler;
layout(binding = 19) uniform sampler2D roughness_metallic_ao_sampler;
layout(binding = 20) uniform samplerCube diffuse_prefilter_sampler;
layout(binding = 21) uniform samplerCube specular_prefilter_sampler;
layout(binding = 22) uniform sampler2D brdf_lut_sampler;
layout(binding = 26) uniform samplerCubeArray pointlight_depth_sampler;
layout(binding = 1, rgba16f) writeonly uniform image2D u_output_texture;

#define DIR_DEPTH_SAMPLER dir_depth_sampler
#define POINTLIGHT_DEPTH_SAMPLER pointlight_depth_sampler
#define SPOTLIGHT_DEPTH_SAMPLER spot_depth_sampler
ORNG_INCLUDE "ShadowsINCL.glsl"

unsigned int shader_id = texelFetch(shader_id_sampler, tex_coords, 0).r;
vec3 sampled_world_pos = WorldPosFromDepth(texelFetch(view_depth_sampler, tex_coords, 0).r, tex_coords / vec2(imageSize(u_output_texture)));
vec3 sampled_normal = normalize(texelFetch(normal_sampler, tex_coords, 0).xyz);
vec3 sampled_albedo = texelFetch(albedo_sampler, tex_coords, 0).xyz;

vec3 roughness_metallic_ao = texelFetch(roughness_metallic_ao_sampler, tex_coords, 0).rgb;
float roughness = roughness_metallic_ao.r;
float metallic = roughness_metallic_ao.g;
float ao = roughness_metallic_ao.b;


vec3 FresnelSchlick(float cos_theta, vec3 f0) {
	return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 h) {
	float a = roughness * roughness;
	float a2 = a * a;
	float n_dot_h = max(dot(sampled_normal, h), 0.0);
	float n_dot_h2 = n_dot_h * n_dot_h;

	float num = a2;
	float denom = (n_dot_h2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
}

float GeometrySchlickGGX(float n_dot_v) {
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	return n_dot_v / (n_dot_v * (1.0 - k) + k);
}

float GeometrySmith(vec3 v, vec3 l) {
	float n_dot_v = max(dot(sampled_normal, v), 0.0);
	float n_dot_l = max(dot(sampled_normal, l), 0.0);
	float ggx2 = GeometrySchlickGGX(n_dot_v);
	float ggx1 = GeometrySchlickGGX(n_dot_l);

	return ggx2 * ggx1;
}



vec3 FresnelSchlickRoughness(float cos_theta, vec3 F0)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}



vec3 CalcPointLight(PointLight light, vec3 v, vec3 f0, int index) {
	vec3 frag_to_light = light.pos.xyz - sampled_world_pos.xyz;

	float distance = length(frag_to_light);
	float attenuation = light.constant +
		light.a_linear * distance +
		light.exp * pow(distance, 2);

	vec3 l = normalize(frag_to_light);
	vec3 h = normalize(v + l);

	vec3 f = FresnelSchlick(max(dot(h, v), 0.0), f0);
	float ndf = DistributionGGX(h);
	float g = GeometrySmith(v, l);

	vec3 num = ndf * g * f;
	float denom = 4.0 * max(dot(sampled_normal, v), 0.0) * max(dot(sampled_normal, l), 0.0) + 0.0001;
	vec3 specular = num / denom;


	vec3 kd = vec3(1.0) - f;
	kd *= 1.0 - metallic;

	float n_dot_l = max(dot(sampled_normal, l), 0.0);


	return (kd * sampled_albedo.xyz / PI + specular) * n_dot_l * (light.color.xyz / attenuation);
}


vec3 CalcSpotLight(SpotLight light, vec3 v, vec3 f0, int index) {
	vec3 frag_to_light = light.pos.xyz - sampled_world_pos.xyz;
	float spot_factor = dot(normalize(frag_to_light), -light.dir.xyz);

	vec3 color = vec3(0);

	if (spot_factor < 0.0001)
		return color;

	float distance = length(frag_to_light);
	float attenuation = light.constant +
		light.a_linear * distance +
		light.exp * pow(distance, 2);

	vec3 l = normalize(frag_to_light);
	vec3 h = normalize(v + l);

	vec3 f = FresnelSchlick(max(dot(h, v), 0.0), f0);
	float ndf = DistributionGGX(h);
	float g = GeometrySmith(v, l);

	vec3 num = ndf * g * f;
	float denom = 4.0 * max(dot(sampled_normal, v), 0.0) * max(dot(sampled_normal, l), 0.0) + 0.0001;
	vec3 specular = num / denom;


	vec3 kd = vec3(1.0) - f;
	kd *= 1.0 - metallic;

	float n_dot_l = max(dot(sampled_normal, l), 0.0);

	float spotlight_intensity = (1.0 - (1.0 - spot_factor) / (1.0 - light.aperture));
	return max((kd * sampled_albedo.xyz / PI + specular) * n_dot_l * (light.color.xyz / attenuation) * spotlight_intensity, vec3(0.0, 0.0, 0.0));
}


vec3 CalcDirectionalLight(vec3 v, vec3 f0) {
	vec3 l = ubo_global_lighting.directional_light.direction.xyz;
	vec3 h = normalize(v + l);
	vec3 f = FresnelSchlick(max(dot(h, v), 0.0), f0);

	float ndf = DistributionGGX(h);
	float g = GeometrySmith(v, l);

	vec3 num = ndf * g * f;
	float denom = 4.0 * max(dot(sampled_normal, v), 0.0) * max(dot(sampled_normal, l), 0.0) + 0.0001;
	vec3 specular = num / denom;


	vec3 kd = vec3(1.0) - f;
	kd *= 1.0 - metallic;

	float n_dot_l = max(dot(sampled_normal, l), 0.0);
	return (kd * sampled_albedo.xyz / PI + specular) * n_dot_l * ubo_global_lighting.directional_light.color.xyz;
}




void main()
{
	if (shader_id != 1) { // 1 = default lighting shader id
		imageStore(u_output_texture, tex_coords, vec4(sampled_albedo.rgb, 1.0));
		return;
	}


	vec3 total_light = vec3(0.0, 0.0, 0.0);
	vec3 v = normalize(ubo_common.camera_pos.xyz - sampled_world_pos);
	vec3 r = reflect(-v, sampled_normal);
	float n_dot_v = max(dot(sampled_normal, v), 0.0);

	//reflection amount
	vec3 f0 = vec3(0.04);
	f0 = mix(f0, sampled_albedo.xyz, metallic);

	// Directional light
	float shadow = ShadowCalculationDirectional(normalize(ubo_global_lighting.directional_light.direction.xyz), sampled_world_pos.xyz);
	total_light += CalcDirectionalLight(v, f0) * (1.0 - shadow);

	// Ambient
	vec3 ks = FresnelSchlickRoughness(n_dot_v, f0);
	vec3 kd = 1.0 - ks;
	kd *= 1.0 - metallic;;
	vec3 diffuse = texture(diffuse_prefilter_sampler, sampled_normal).rgb * sampled_albedo.xyz;

	vec3 prefiltered_colour = textureLod(specular_prefilter_sampler, r, roughness * MAX_REFLECTION_LOD).rgb;
	vec2 env_brdf = texture(brdf_lut_sampler, vec2(n_dot_v, roughness)).rg;
	vec3 specular = prefiltered_colour * (ks * env_brdf.x + env_brdf.y);

	vec3 ambient_light = (kd * diffuse + specular) * ao;
	total_light += ambient_light;


	// Pointlights
	for (int i = 0; i < ubo_point_lights_shadow.lights.length(); i++) {
		total_light += (CalcPointLight(ubo_point_lights_shadow.lights[i], v, f0, i)) * (1.0 - ShadowCalculationPointlight(ubo_point_lights_shadow.lights[i], i, sampled_world_pos.xyz));
	}

	for (int i = 0; i < ubo_point_lights_shadowless.lights.length(); i++) {
		total_light += (CalcPointLight(ubo_point_lights_shadowless.lights[i], v, f0, i));
	}

	 //Spotlights
	for (int i = 0; i < ubo_spot_lights_shadowless.lights.length(); i++) {
		total_light += (CalcSpotLight(ubo_spot_lights_shadowless.lights[i], v, f0, i));
	}

	for (int i = 0; i < ubo_spot_lights_shadow.lights.length(); i++) {
		float shadow = ShadowCalculationSpotlight(ubo_spot_lights_shadow.lights[i], i, sampled_world_pos.xyz);
		if (shadow >= 0.99)
			continue;

		total_light += (CalcSpotLight(ubo_spot_lights_shadow.lights[i], v, f0, i)) * (1.0 - shadow);
	}

	vec3 light_color = max(vec3(total_light), vec3(0.0, 0.0, 0.0));

	imageStore(u_output_texture, tex_coords, vec4(light_color , 1.0));
};