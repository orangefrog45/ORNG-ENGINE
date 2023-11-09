#version 460 core

#define PI 3.1415926538

ORNG_INCLUDE "UtilINCL.glsl"


layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in; // invocations

layout(binding = 1, rgba16f) writeonly uniform image2D fog_texture;
layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 4) uniform sampler2DArray spot_depth_sampler;
layout(binding = 11) uniform sampler2D blue_noise_sampler;
layout(binding = 15) uniform sampler3D fog_noise_sampler;
layout(binding = 16) uniform sampler2D gbuffer_depth_sampler;
layout(binding = 21) uniform samplerCube diffuse_prefilter_sampler;
layout(binding = 26) uniform samplerCubeArray pointlight_depth_sampler;

#define DIR_DEPTH_SAMPLER dir_depth_sampler
#define POINTLIGHT_DEPTH_SAMPLER pointlight_depth_sampler
#define SPOTLIGHT_DEPTH_SAMPLER spot_depth_sampler

ORNG_INCLUDE "ShadowsINCL.glsl"

uniform float u_scattering_anistropy;
uniform float u_absorption_coef;
uniform float u_scattering_coef;
uniform float u_density_coef;
uniform float u_time;
uniform float u_emissive;
uniform int u_step_count;
uniform vec3 u_fog_color;



float CheapShadowCalculationDirectional(vec3 light_dir, vec3 world_pos) {
	float shadow = 0;

	vec3 proj_coords = vec3(0);
	float frag_distance_from_cam = length(world_pos.xyz - ubo_common.camera_pos.xyz);

	// branchless checks for range
	int index = 3;
	index -= int(frag_distance_from_cam <= ubo_global_lighting.directional_light.cascade_ranges[2]);
	index -= int(frag_distance_from_cam <= ubo_global_lighting.directional_light.cascade_ranges[1]);
	index -= int(frag_distance_from_cam <= ubo_global_lighting.directional_light.cascade_ranges[0]);
	vec4 frag_pos_light_space = ubo_global_lighting.directional_light.light_space_transforms[index] * vec4(world_pos, 1);
	proj_coords = (frag_pos_light_space / frag_pos_light_space.w).xyz;
	//depth map in range 0,1 proj in -1,1 (NDC), so convert
	proj_coords = proj_coords * 0.5f + 0.5f;
	if (proj_coords.z > 1.0f || index == 3) {
		return 0.0f; // early return, fragment out of range
	}
	float current_depth = proj_coords.z;
	float sampled_depth = texture(dir_depth_sampler, vec3(vec2(proj_coords.xy), index)).r;
	shadow = current_depth > sampled_depth ? 1.0f : 0.0f;
	return shadow;

}


vec3 CalcPointLight(PointLight light, vec3 world_pos) {
	float distance = length(light.pos.xyz - world_pos);

	if (distance > light.max_distance)
		return vec3(0.0);

	vec3 diffuse_color = light.color.xyz;

	float attenuation = light.constant +
		light.a_linear * distance +
		light.exp * pow(distance, 2);


	return diffuse_color / attenuation;
}


vec3 CalcSpotLight(SpotLight light, vec3 world_pos, uint index) {
	vec3 frag_to_light = light.pos.xyz - world_pos.xyz;
	float distance = length(frag_to_light);

	if (distance > light.max_distance)
		return vec3(0.0);

	vec3 frag_to_light_dir = normalize(frag_to_light);

	float spot_factor = dot(frag_to_light_dir, -light.dir.xyz);

	if (spot_factor > light.aperture) {
		float shadow = ShadowCalculationSpotlight(light, index, world_pos);

		if (shadow >= 0.99) {
			return vec3(0); // early return as no light will reach this spot
		}

		vec3 diffuse_color = light.color.xyz;

		float attenuation = light.constant +
			light.a_linear * distance +
			light.exp * pow(distance, 2);

		diffuse_color /= attenuation;

		float spotlight_intensity = (1.0 - (1.0 - spot_factor) / (1.0 - light.aperture));
		return diffuse_color * spotlight_intensity;
	}
	else {
		return vec3(0, 0, 0);
	}
}



float phase(vec3 march_dir, vec3 light_dir) {
	float cos_theta = dot(normalize(march_dir), normalize(light_dir));
	return (1.0 - u_scattering_anistropy * u_scattering_anistropy) / (4.0 * PI * pow(1.0 - u_scattering_anistropy * cos_theta, 2.0));
}



float NoiseFog(vec3 pos) {
	float f = max(texture(fog_noise_sampler, pos).r, 0.0f);

	return f;
}



//https://github.com/Unity-Technologies/VolumetricLighting/blob/master/Assets/VolumetricFog/Shaders/Scatter.compute
vec4 Accumulate(vec3 accum_light, float accum_transmittance, vec3 slice_light, float slice_density, float step_distance, float extinction_coef) {
	slice_density = max(slice_density, 0.000001);
	float slice_transmittance = exp(-slice_density * step_distance * extinction_coef);
	vec3 slice_light_integral = slice_light * (1.0 - slice_transmittance) / slice_density;

	accum_light += slice_light_integral * accum_transmittance;
	accum_transmittance *= slice_transmittance;

	return vec4(accum_light, accum_transmittance);
}

float fbm(vec3 p)
{
    vec3 q = p;
    //q.xy = rotate(p.xy, ubo_common.time_elapsed);

    p += (NoiseFog(p * 3.0) - 0.5) * 0.3;

    //float v = nse3d(p) * 0.5 + nse3d(p * 2.0) * 0.25 + nse3d(p * 4.0) * 0.125 + nse3d(p * 8.0) * 0.0625;

    //p.y += ubo_common.time_elapsed * 0.2;

    float mtn = ubo_common.time_elapsed * 0.0001;

    float v = 0.0;
    float fq = 1.0, am = 0.5;
    for(int i = 0; i < 6; i++)
    {
        v += NoiseFog(p * fq + mtn * fq) * am;
        fq *= 2.0;
        am *= 0.5;
    }
    return v;
}


vec2 grad(ivec2 z)  // replace this anything that returns a random vector
{
	// 2D to 1D  (feel free to replace by some other)
	int n = z.x + z.y * 11111;

	// Hugo Elias hash (feel free to replace by another one)
	n = (n << 13) ^ n;
	n = (n * (n * n * 15731 + 789221) + 1376312589) >> 16;

#if 0

	// simple random vectors
	return vec2(cos(float(n)), sin(float(n)));

#else

	// Perlin style vectors
	n &= 7;
	vec2 gr = vec2(n & 1, n >> 1) * 2.0 - 1.0;
	return (n >= 6) ? vec2(0.0, gr.x) :
		(n >= 4) ? vec2(gr.x, 0.0) :
		gr;
#endif
}

float PerlinNoise(vec2 p)
{
	ivec2 i = ivec2(floor(p));
	vec2 f = fract(p);

	vec2 u = f * f * (3.0 - 2.0 * f); // feel free to replace by a quintic smoothstep instead

	return mix(mix(dot(grad(i + ivec2(0, 0)), f - vec2(0.0, 0.0)),
		dot(grad(i + ivec2(1, 0)), f - vec2(1.0, 0.0)), u.x),
		mix(dot(grad(i + ivec2(0, 1)), f - vec2(0.0, 1.0)),
			dot(grad(i + ivec2(1, 1)), f - vec2(1.0, 1.0)), u.x), u.y);
}


void main() {
	// Tex coords in range (0, 0), (screen width, screen height) / 2
	ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy) ;

	float noise_offset = texelFetch(blue_noise_sampler, ivec2((tex_coords.xy) % textureSize(blue_noise_sampler, 0).xy), 0).r;

	float fragment_depth = texelFetch(gbuffer_depth_sampler, tex_coords * 2, 0).r;
	vec3 frag_world_pos = WorldPosFromDepth(fragment_depth, tex_coords / (vec2(imageSize(fog_texture))));
	vec3 cam_to_frag = frag_world_pos - ubo_common.camera_pos.xyz;
	float cam_to_frag_dist = min(length(cam_to_frag), 2000.0);
	float step_distance = cam_to_frag_dist / float(u_step_count);


	vec3 ray_dir = normalize(cam_to_frag);

	vec3 step_pos = ubo_common.camera_pos.xyz + ray_dir * noise_offset * step_distance;

	vec4 accum = vec4(0, 0, 0, 1);

	float extinction_coef = (u_absorption_coef + u_scattering_coef);

	// Raymarching
	for (int i = 0; i < u_step_count; i++) {
		vec3 fog_sampling_coords = vec3(step_pos.x, step_pos.y, step_pos.z) / 200.f;
		//float fog_density = fbm(fog_sampling_coords) * u_density_coef;
		float fog_density = NoiseFog(fog_sampling_coords) * u_density_coef;
		fog_density += u_density_coef * 0.5f;
		fog_density *= exp(-step_pos.y * 0.1);

		vec3 slice_light = vec3(0);

		for (unsigned int i = 0; i < ubo_point_lights_shadowless.lights.length(); i++) {
			slice_light += CalcPointLight(ubo_point_lights_shadowless.lights[i], step_pos) * phase(ray_dir, normalize(ubo_point_lights_shadowless.lights[i].pos.xyz - step_pos));
		}

		for (unsigned int i = 0; i < ubo_spot_lights_shadow.lights.length(); i++) {
			slice_light += CalcSpotLight(ubo_spot_lights_shadow.lights[i], step_pos, i) * phase(ray_dir, -ubo_spot_lights_shadow.lights[i].dir.xyz);
		}

		float dir_shadow = CheapShadowCalculationDirectional(ubo_global_lighting.directional_light.direction.xyz, step_pos);
		slice_light += ubo_global_lighting.directional_light.color.xyz * (1.0 - dir_shadow) * phase(ray_dir, ubo_global_lighting.directional_light.direction.xyz);
		slice_light += abs(vec3(sin(PerlinNoise(step_pos.xz * 0.01) * 2.0 * 3.14), i * 0.05, cos(PerlinNoise(step_pos.xz * 0.01) * 2.0 * 3.14))) / u_step_count;
		accum = Accumulate(accum.rgb, accum.a, slice_light, fog_density, step_distance, extinction_coef);
		step_pos += ray_dir * step_distance;
	}

	vec4 fog_color = vec4(u_fog_color * accum.rgb + textureLod(diffuse_prefilter_sampler, ray_dir, 4).rgb * u_emissive, 1.0 - accum.a );
	imageStore(fog_texture, tex_coords, fog_color);
}