#version 460 core

layout(binding = 1) uniform sampler2D diffuse_sampler;
layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 4) uniform sampler2DArray spot_depth_sampler;
layout(binding = 7) uniform sampler2D normal_sampler;
layout(binding = 19) uniform sampler2D roughness_metallic_ao_sampler;
layout(binding = 25) uniform sampler2D emissive_sampler;
layout(binding = 26) uniform samplerCubeArray pointlight_depth_sampler;


layout(binding = 0, rgba16f) writeonly uniform image3D voxel_image;

in vec4 vs_position;
in vec3 vs_normal;
in vec3 vs_tex_coord;
in vec3 vs_tangent;
in mat4 vs_transform;
in vec3 vs_original_normal;
in vec3 vs_view_dir_tangent_space;

ORNG_INCLUDE "CommonINCL.glsl"
ORNG_INCLUDE "ParticleBuffersINCL.glsl"
ORNG_INCLUDE "BuffersINCL.glsl"

#define DIR_DEPTH_SAMPLER dir_depth_sampler
#define POINTLIGHT_DEPTH_SAMPLER pointlight_depth_sampler
#define SPOTLIGHT_DEPTH_SAMPLER spot_depth_sampler
ORNG_INCLUDE "ShadowsINCL.glsl"

uniform uint u_shader_id;
uniform bool u_normal_sampler_active;
uniform bool u_roughness_sampler_active;
uniform bool u_metallic_sampler_active;
uniform bool u_displacement_sampler_active;
uniform bool u_emissive_sampler_active;
uniform bool u_ao_sampler_active;
uniform bool u_terrain_mode;
uniform bool u_skybox_mode;
uniform float u_parallax_height_scale;
uniform uint u_num_parallax_layers;
uniform float u_bloom_threshold;
uniform Material u_material;

uniform vec3 u_aligned_camera_pos;


vec3 CalculatePointlight(PointLight light) {
    vec3 dir = light.pos.xyz - vs_position.xyz;
    const float l = length(dir);
    dir = normalize(dir);

    float attenuation = light.constant +
		light.a_linear * l +
		light.exp * pow(l, 2);

    const float r = max(dot(normalize(vs_normal), dir), 0.f);
    return r / attenuation * light.color.xyz;
}

vec3 CalculateSpotlight(SpotLight light) {
    vec3 dir = light.pos.xyz - vs_position.xyz;
	float spot_factor = dot(normalize(dir), -light.dir.xyz);

    vec3 color = vec3(0);

	if (spot_factor < 0.0001 || spot_factor < light.aperture)
		return color;

    const float l = length(dir);
    dir = normalize(dir);

    float attenuation = light.constant +
		light.a_linear * l +
		light.exp * pow(l, 2);

	float spotlight_intensity = (1.0 - (1.0 - spot_factor) / max((1.0 - light.aperture), 1e-5));
    const float r = max(dot(normalize(vs_normal), dir), 0.f);

    return r * spotlight_intensity / attenuation * light.color.xyz;
}

vec4 CalculateAlbedoAndEmissive(vec2 tex_coord) {
	vec4 sampled_albedo = texture(diffuse_sampler, tex_coord.xy);
	vec4 albedo_col = vec4(sampled_albedo.rgb * u_material.base_color.rgb, sampled_albedo.a);
	albedo_col *= u_material.emissive ? vec4(vec3(u_material.emissive_strength), 1.0) : vec4(1.0);
#ifdef PARTICLE
#define EMITTER ssbo_particle_emitters.emitters[ssbo_particles.particles[vs_particle_index].emitter_index]
#define PTCL ssbo_particles.particles[vs_particle_index]

	float interpolation = 1.0 - clamp(PTCL.velocity_life.w, 0.0, EMITTER.lifespan) / EMITTER.lifespan;

	albedo_col *= vec4(InterpolateV3(interpolation, EMITTER.colour_over_life), 1.0);

#endif
	if (u_emissive_sampler_active) {
		vec4 sampled_col = texture(emissive_sampler, tex_coord);
		albedo_col += vec4(sampled_col.rgb * u_material.emissive_strength * sampled_col.w, 0.0);
	}

	return albedo_col;
}


void main() {
    vec3 n = normalize(vs_normal);
    vec3 col = vec3(0);
    
    if (u_material.emissive) {
        col = CalculateAlbedoAndEmissive(vs_tex_coord.xy).rgb;
    } else {
        for (int i = 0; i < ubo_point_lights_shadow.lights.length(); i++) {
            col += CalculatePointlight(ubo_point_lights_shadow.lights[i]) * (1.0 - ShadowCalculationPointlight(ubo_point_lights_shadow.lights[i], i, vs_position.xyz ));
        }
        col += ubo_global_lighting.directional_light.color.xyz * max(dot(ubo_global_lighting.directional_light.direction.xyz, n), 0.0) * (1.0 - CheapShadowCalculationDirectional(vs_position.xyz));
        col *= CalculateAlbedoAndEmissive(vs_tex_coord.xy).rgb;
    }
    imageStore(voxel_image, ivec3((vs_position.xyz - u_aligned_camera_pos  ) * 5.0  + vec3(128)), vec4(col, 1));

}