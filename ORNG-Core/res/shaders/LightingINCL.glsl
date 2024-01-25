ORNG_INCLUDE "BuffersINCL.glsl"
ORNG_INCLUDE "ShadowsINCL.glsl"

#ifndef SPECULAR_PREFILTER_SAMPLER
#error "No specular prefilter sampler defined for lighting calculations"
#endif

#ifndef DIFFUSE_PREFILTER_SAMPLER
#error "No diffuse prefilter sampler defined for lighting calculations"
#endif

#ifndef BRDF_LUT_SAMPLER
#error "No brdf LUT sampler defined for lighting calculations"
#endif

#define MAX_REFLECTION_LOD 4.0
#define LIGHT_ZNEAR 0.01 // true for all light types

vec3 FresnelSchlick(float cos_theta, vec3 f0) {
	return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 h, vec3 n, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float n_dot_h = max(dot(n, h), 0.0);
	float n_dot_h2 = n_dot_h * n_dot_h;

	float num = a2;
	float denom = (n_dot_h2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
}

float GeometrySchlickGGX(float n_dot_v, float roughness) {
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	return n_dot_v / (n_dot_v * (1.0 - k) + k);
}

float GeometrySmith(vec3 v, vec3 l, vec3 n, float roughness) {
	float n_dot_v = max(dot(n, v), 0.0);
	float n_dot_l = max(dot(n, l), 0.0);
	float ggx2 = GeometrySchlickGGX(n_dot_v, roughness);
	float ggx1 = GeometrySchlickGGX(n_dot_l, roughness);

	return ggx2 * ggx1;
}



vec3 FresnelSchlickRoughness(float cos_theta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}



vec3 CalcPointLight(PointLight light, vec3 v, vec3 f0, int index, vec3 world_pos, vec3 n, float roughness, float metallic, vec3 albedo) {
	vec3 frag_to_light = light.pos.xyz - world_pos.xyz;

	float distance = length(frag_to_light);
	float attenuation = light.constant +
		light.a_linear * distance +
		light.exp * pow(distance, 2);

	if (attenuation > 1000)
		return vec3(0);

	vec3 l = normalize(frag_to_light);
	vec3 h = normalize(v + l);

	vec3 f = FresnelSchlick(max(dot(h, v), 0.0), f0);
	float ndf = DistributionGGX(h, n, roughness);
	float g = GeometrySmith(v, l, n, roughness);

	vec3 num = ndf * g * f;
	float denom = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.0001;
	vec3 specular = num / denom;


	vec3 kd = vec3(1.0) - f;
	kd *= 1.0 - metallic;

	float n_dot_l = max(dot(n, l), 0.0);


	return (kd * albedo / PI + specular) * n_dot_l * (light.color.xyz / attenuation);
}


vec3 CalcSpotLight(SpotLight light, vec3 v, vec3 f0, int index, vec3 world_pos, vec3 n, float roughness, float metallic, vec3 albedo) {
	vec3 frag_to_light = light.pos.xyz - world_pos;
	float spot_factor = dot(normalize(frag_to_light), -light.dir.xyz);

	vec3 color = vec3(0);

	if (spot_factor < 0.0001 || spot_factor < light.aperture)
		return color;

	float distance = length(frag_to_light);
	float attenuation = light.constant +
		light.a_linear * distance +
		light.exp * pow(distance, 2);

		if (attenuation > 1000)
		return vec3(0);

	vec3 l = normalize(frag_to_light);
	vec3 h = normalize(v + l);

	vec3 f = FresnelSchlick(max(dot(h, v), 0.0), f0);
	float ndf = DistributionGGX(h, n, roughness);
	float g = GeometrySmith(v, l, n, roughness);

	float n_dot_l = max(dot(n, l), 0.0);

	vec3 num = ndf * g * f;
	float denom = 4.0 * max(dot(n, v), 0.0) * max(n_dot_l, 0.0) + 0.0001;
	vec3 specular = num / denom;


	vec3 kd = vec3(1.0) - f;
	kd *= 1.0 - metallic;


	float spotlight_intensity = (1.0 - (1.0 - spot_factor) / max((1.0 - light.aperture), 1e-5));
	return max((kd * albedo / PI + specular) * n_dot_l * (light.color.xyz / attenuation) * spotlight_intensity, vec3(0.0, 0.0, 0.0));
}


vec3 CalcDirectionalLight(vec3 v, vec3 f0, vec3 n, float roughness, float metallic, vec3 albedo) {
	vec3 l = ubo_global_lighting.directional_light.direction.xyz;
	vec3 h = normalize(v + l);
	vec3 f = FresnelSchlick(max(dot(h, v), 0.0), f0);

	float ndf = DistributionGGX(h, n, roughness);
	float g = GeometrySmith(v, l, n, roughness);

	vec3 num = ndf * g * f;
	float denom = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.0001;
	vec3 specular = num / denom;


	vec3 kd = vec3(1.0) - f;
	kd *= 1.0 - metallic;

	float n_dot_l = max(dot(n, l), 0.0);
	return (kd * albedo / PI + specular) * n_dot_l * ubo_global_lighting.directional_light.color.xyz;
}




vec3 CalculateDirectLightContribution(vec3 v, vec3 f0, vec3 world_pos, vec3 n, float roughness, float metallic, vec3 albedo) {
	vec3 total_light = vec3(0);
		// Directional light
	float shadow = ShadowCalculationDirectional(normalize(ubo_global_lighting.directional_light.direction.xyz), world_pos);
	total_light += CalcDirectionalLight(v, f0, n, roughness, metallic, albedo) * (1.0 - shadow);

		// Pointlights
	for (int i = 0; i < ubo_point_lights_shadow.lights.length(); i++) {
		total_light += (CalcPointLight(ubo_point_lights_shadow.lights[i], v, f0, i, world_pos, n, roughness, metallic, albedo)) * (1.0 - ShadowCalculationPointlight(ubo_point_lights_shadow.lights[i], i, world_pos));
	}

	for (int i = 0; i < ubo_point_lights_shadowless.lights.length(); i++) {
		total_light += (CalcPointLight(ubo_point_lights_shadowless.lights[i], v, f0, i, world_pos, n, roughness, metallic, albedo));
	}

	//Spotlights
	for (int i = 0; i < ubo_spot_lights_shadowless.lights.length(); i++) {
		total_light += (CalcSpotLight(ubo_spot_lights_shadowless.lights[i], v, f0, i, world_pos, n, roughness, metallic, albedo));
	}

	for (int i = 0; i < ubo_spot_lights_shadow.lights.length(); i++) {
		float shadow = ShadowCalculationSpotlight(ubo_spot_lights_shadow.lights[i], i, world_pos);
		if (shadow >= 0.99)
			continue;

		total_light += (CalcSpotLight(ubo_spot_lights_shadow.lights[i], v, f0, i, world_pos, n, roughness, metallic, albedo)) * (1.0 - shadow);
	}

	return total_light;
}


vec3 CalculateAmbientLightContribution(float n_dot_v, vec3 f0, vec3 r, float roughness, vec3 n, float ao, float metallic, vec3 albedo) {
	vec3 total_light = vec3(0);
	// Ambient
	vec3 ks = FresnelSchlickRoughness(n_dot_v, f0, roughness);
	vec3 kd = 1.0 - ks;
	kd *= 1.0 - metallic;
	vec3 diffuse = texture(DIFFUSE_PREFILTER_SAMPLER, n).rgb * albedo;

	vec3 prefiltered_colour = textureLod(SPECULAR_PREFILTER_SAMPLER, r, roughness * MAX_REFLECTION_LOD).rgb;
	vec2 env_brdf = texture(BRDF_LUT_SAMPLER, vec2(n_dot_v, roughness)).rg;
	vec3 specular = prefiltered_colour * (ks * env_brdf.x + env_brdf.y);

	vec3 ambient_light = (kd * diffuse + specular) * ao;
	total_light += ambient_light;
	return total_light;
}
