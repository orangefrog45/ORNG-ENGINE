#version 460 core

ORNG_INCLUDE "UtilINCL.glsl"


layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in; // invocations

layout(binding = 1, rgba16f) writeonly uniform image2D fog_texture;
layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 4) uniform sampler2DArray spot_depth_sampler;
layout(binding = 11) uniform sampler2D blue_noise_sampler;
layout(binding = 16) uniform sampler2D gbuffer_depth_sampler;
layout(binding = 21) uniform samplerCube diffuse_prefilter_sampler;
layout(binding = 26) uniform samplerCubeArray pointlight_depth_sampler;

#define DIR_DEPTH_SAMPLER dir_depth_sampler
#define POINTLIGHT_DEPTH_SAMPLER pointlight_depth_sampler
#define SPOTLIGHT_DEPTH_SAMPLER spot_depth_sampler

ORNG_INCLUDE "ShadowsINCL.glsl"

uniform float u_scattering_anisotropy;
uniform float u_absorption_coef;
uniform float u_scattering_coef;
uniform float u_density_coef;
uniform float u_time;
uniform float u_emissive;
uniform int u_step_count;
uniform vec3 u_fog_colour;





vec3 CalcPointLight(PointLight light, vec3 world_pos) {
	float distance = length(light.pos.xyz - world_pos);

	if (distance > light.shadow_distance)
		return vec3(0.0);

	vec3 diffuse_colour = light.colour.xyz;

	float attenuation = light.constant +
		light.a_linear * distance +
		light.exp * pow(distance, 2);


	return diffuse_colour / attenuation;
}


vec3 CalcSpotLight(SpotLight light, vec3 world_pos, uint index) {
	vec3 frag_to_light = light.pos.xyz - world_pos.xyz;
	float distance = length(frag_to_light);

	if (distance > light.shadow_distance)
		return vec3(0.0);

	vec3 frag_to_light_dir = normalize(frag_to_light);

	float spot_factor = dot(frag_to_light_dir, -light.dir.xyz);
	
	if (spot_factor > light.aperture) {
		float shadow = light.shadow_distance > 0.f ? ShadowCalculationSpotlight(light, index, world_pos) : 0.f;

		if (shadow >= 0.99) {
			return vec3(0); // early return as no light will reach this spot
		}

		vec3 diffuse_colour = light.colour.xyz;

		float attenuation = light.constant +
			light.a_linear * distance +
			light.exp * pow(distance, 2);

		diffuse_colour /= attenuation;

		float spotlight_intensity = (1.0 - (1.0 - spot_factor) / (1.0 - light.aperture));
		return diffuse_colour * spotlight_intensity;
	}
	else {
		return vec3(0, 0, 0);
	}
}



float phase(vec3 march_dir, vec3 light_dir) {
	float cos_theta = dot(normalize(march_dir), normalize(light_dir));
	return (1.0 - u_scattering_anisotropy * u_scattering_anisotropy) / (4.0 * PI * pow(1.0 - u_scattering_anisotropy * cos_theta, 2.0));
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

//-------------------------------------------------------------------------------------
// otaviogood's noise from https://www.shadertoy.com/view/ld2SzK
//--------------------------------------------------------------
// This spiral noise works by successively adding and rotating sin waves while increasing frequency.
// It should work the same on all computers since it's not based on a hash function like some other noises.
// It can be much faster than other noise functions if you're ok with some repetition.
const float nudge = 2000.;	// size of perpendicular vector
float normalizer = 1.0 / sqrt(1.0 + nudge*nudge);	// pythagorean theorem on that perpendicular to maintain scale
float SpiralNoiseC(vec3 p, vec4 id) {
    float iter = 2., n = 2.-id.x; // noise amount
    for (int i = 0; i < 6; i++) {
        n += -abs(sin(p.y*iter) + cos(p.x*iter)) / iter; // add sin and cos scaled inverse with the frequency (abs for a ridged look)
        p.xy += vec2(p.y, -p.x) * nudge; // rotate by adding perpendicular and scaling down
        p.xy *= normalizer;
        p.xz += vec2(p.z, -p.x) * nudge; // rotate on other axis
        p.xz *= normalizer;  
        iter *= id.y + .733733;          // increase the frequency
    }
    return n;
}

float opSubtraction( float d1, float d2 )
{
    return max(-d1,d2);
}


// hash function              
float hash(float n)
{
    return fract(cos(n) * 114514.1919);
}

// 3d noise function
float noise(in vec3 x)
{
    vec3 p = floor(x);
    vec3 f = smoothstep(0.0, 1.0, fract(x));
        
    float n = p.x + p.y * 10.0 + p.z * 100.0;
    
    return mix(
        mix(mix(hash(n + 0.0), hash(n + 1.0), f.x),
            mix(hash(n + 10.0), hash(n + 11.0), f.x), f.y),
        mix(mix(hash(n + 100.0), hash(n + 101.0), f.x),
            mix(hash(n + 110.0), hash(n + 111.0), f.x), f.y), f.z);
}

mat3 m = mat3(0.00, 1.60, 1.20, -1.60, 0.72, -0.96, -1.20, -0.96, 1.28);

float fbm(vec3 p)
{
    float f = 0.5000 * noise(p);
    p = m * p;
    f += 0.2500 * noise(p);
    p = m * p;
    f += 0.1666 * noise(p);
    p = m * p;
    f += 0.0834 * noise(p);
    return f;
}


float FogDensityGyro(vec3 step_pos) {
	float gyro_frequency = 0.01;
	float o = sin(step_pos.x * 0.1+step_pos.y*0.05)*sin(step_pos.y * 0.01)*sin(step_pos.z * 0.031);
	float gyro_d = dot(sin(step_pos*gyro_frequency), cos(step_pos.yzx*gyro_frequency));
	return exp(-clamp(fbm(step_pos*0.01) + gyro_d*0.2, 0.0, 1.0)*40) * 300000;
}

vec4 Galaxy(vec3 step_pos) {
	float fog_density = 0.0;
	vec3 slice_light = vec3(0);

	float d_min = 100000;
	for (int i = 3; i < 6; i++) {
		float theta = atan(step_pos.z, step_pos.x) + 2 * PI * i;
		float theta_2 = atan(-step_pos.z, -step_pos.x) + 2 * PI * i;
		vec3 p = vec3(cos(theta), 0, sin(theta));
		vec3 p_neg = vec3(cos(theta_2), 0, sin(theta_2));
		p *= pow(theta, 4.0)*0.00025;
		p_neg *= pow(theta_2, 4.0)*0.00025;
		d_min = min(length(step_pos - p), d_min);
		d_min = min(length(-step_pos - p_neg), d_min);
	}

	float ns = fbm(step_pos*0.03);
	float l = length(step_pos);
	fog_density = exp(-d_min*0.09) * exp(-clamp(ns, 0.0, 1.0)*3)*3;	
	slice_light += vec3(1.0, 0.2, 0.08) *  exp(-clamp(ns, 0.0, 1.0)*5)*fog_density*exp(-l * 0.02)*20;
	slice_light += vec3(1.0, 0.3, 0.4) * exp(-clamp(noise(-step_pos*0.1), 0.0, 1.0)*500)*fog_density*1000;
	slice_light += vec3(0.3, 0.5, 1.0) * exp(-l * 0.04);

	return vec4(slice_light, fog_density);
}

void main() {
	// Tex coords in range (0, 0), (screen width, screen height) / 2
	ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy);

	float noise_offset = texelFetch(blue_noise_sampler, ivec2(tex_coords % textureSize(blue_noise_sampler, 0).xy), 0).r;

	float fragment_depth = texelFetch(gbuffer_depth_sampler, tex_coords * 2, 0).r;
	vec3 frag_world_pos = WorldPosFromDepth(fragment_depth, tex_coords / vec2(imageSize(fog_texture)));
	vec3 cam_to_frag = frag_world_pos - ubo_common.camera_pos.xyz;
	float cam_to_frag_dist = min(length(cam_to_frag), 4000.0);
	float step_distance = cam_to_frag_dist / float(u_step_count);

	vec3 ray_dir = normalize(cam_to_frag);
	vec3 step_pos = ubo_common.camera_pos.xyz + ray_dir * noise_offset * step_distance;
	vec4 accum = vec4(0, 0, 0, 1);
	
		// Raymarching
	for (int i = 0; i < u_step_count; i++) {
		float fog_density = u_density_coef * abs(sin(step_pos.y*step_pos.z*step_pos.x*0.001));

		vec3 slice_light = vec3(0);

		for (uint i = 0; i < ubo_point_lights.lights.length(); i++) {
			vec3 point_to_light = ubo_point_lights.lights[i].pos.xyz - step_pos;
			slice_light += 
			CalcPointLight(ubo_point_lights.lights[i], step_pos) * phase(ray_dir, normalize(ubo_point_lights.lights[i].pos.xyz - step_pos)) * 
			(1.0 - ShadowCalculationPointlight(ubo_point_lights.lights[i], int(i), step_pos));
		}

		for (uint i = 0; i < ubo_spot_lights.lights.length(); i++) {
			slice_light += CalcSpotLight(ubo_spot_lights.lights[i], step_pos, i) * phase(ray_dir, -ubo_spot_lights.lights[i].dir.xyz);
		}

		float dir_shadow = CheapShadowCalculationDirectional(step_pos);
		slice_light += ubo_global_lighting.directional_light.colour.xyz  * phase(ray_dir, ubo_global_lighting.directional_light.direction.xyz);
		accum = Accumulate(accum.rgb, accum.a, slice_light, fog_density, step_distance, u_absorption_coef + u_scattering_coef);
		step_pos += ray_dir * step_distance;
	}
	

	vec4 fog_colour = vec4(u_fog_colour * accum.rgb + textureLod(diffuse_prefilter_sampler, ray_dir, 4).rgb * u_emissive, 1.0 - accum.a );
	imageStore(fog_texture, tex_coords, fog_colour);
}