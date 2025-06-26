#version 460 core
layout(location = 0) out vec4 accum;

layout(location = 1) out float reveal;

layout(binding = 1) uniform sampler2D diffuse_sampler;
layout(binding = 2) uniform sampler2D roughness_sampler;
layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 4) uniform sampler2DArray spot_depth_sampler;
layout(binding = 6) uniform sampler2D view_depth_sampler;
layout(binding = 7) uniform sampler2D normal_map_sampler;
layout(binding = 9) uniform sampler2D displacement_sampler;
layout(binding = 13) uniform samplerCube cube_colour_sampler;
layout(binding = 17) uniform sampler2D metallic_sampler;
layout(binding = 18) uniform sampler2D ao_sampler;
layout(binding = 20) uniform samplerCube diffuse_prefilter_sampler;
layout(binding = 21) uniform samplerCube specular_prefilter_sampler;
layout(binding = 22) uniform sampler2D brdf_lut_sampler;
layout(binding = 25) uniform sampler2D emissive_sampler;
layout(binding = 26) uniform samplerCubeArray pointlight_depth_sampler;


in VSVertData {
    vec4 position;
    vec3 normal;
    vec2 tex_coord;
    vec3 tangent;
    vec3 original_normal;
    vec3 view_dir_tangent_space;
} vert_data;

ORNG_INCLUDE "CommonINCL.glsl"
ORNG_INCLUDE "ParticleBuffersINCL.glsl"

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

#define DIR_DEPTH_SAMPLER dir_depth_sampler
#define POINTLIGHT_DEPTH_SAMPLER pointlight_depth_sampler
#define SPOTLIGHT_DEPTH_SAMPLER spot_depth_sampler
#define SPECULAR_PREFILTER_SAMPLER specular_prefilter_sampler
#define DIFFUSE_PREFILTER_SAMPLER diffuse_prefilter_sampler
#define BRDF_LUT_SAMPLER brdf_lut_sampler
ORNG_INCLUDE "LightingINCL.glsl"

#ifdef PARTICLE
uniform uint u_transform_start_index;

flat in uint vs_particle_index;
#endif

vec4 CalculateAlbedoAndEmissive(vec2 tex_coord) {
vec4 sampled_albedo = texture(diffuse_sampler, tex_coord.xy);
	vec4 albedo_col = vec4(sampled_albedo.rgb * u_material.base_colour.rgb, sampled_albedo.a);
	albedo_col *= bool(u_material.flags & MAT_FLAG_EMISSIVE) ? vec4(vec3(u_material.emissive_strength), 1.0) : vec4(1.0);

#ifdef PARTICLE
#define EMITTER ssbo_particle_emitters.emitters[ssbo_particles.particles[vs_particle_index].emitter_index]
#define PTCL ssbo_particles.particles[vs_particle_index]

	float interpolation = 1.0 - (clamp(PTCL.velocity_life.w, 0.0, EMITTER.lifespan) / EMITTER.lifespan);

	albedo_col *= vec4(InterpolateV3(interpolation, EMITTER.colour_over_life), InterpolateV1(interpolation, EMITTER.alpha_over_life));
#endif

	if (u_emissive_sampler_active) {
		vec4 sampled_col = texture(emissive_sampler, tex_coord);
		albedo_col += vec4(sampled_col.rgb * u_material.emissive_strength * sampled_col.w, 0.0);
	}

	return albedo_col;
}


const float EPSILON = 0.00001f;

void main() {

    vec3 total_light = vec3(0.0, 0.0, 0.0);
    vec3 n = normalize(vert_data.normal);
	vec3 v = normalize(ubo_common.camera_pos.xyz - vert_data.position.xyz);
	vec3 r = reflect(-v, n);
	float n_dot_v = max(dot(n, v), 0.0);

	vec3 f0 = mix( vec3(0.03), u_material.base_colour.rgb, u_material.metallic);

	float roughness = texture(roughness_sampler, vert_data.tex_coord.xy).r * float(u_roughness_sampler_active) + u_material.roughness * float(!u_roughness_sampler_active);
	float metallic = texture(metallic_sampler, vert_data.tex_coord.xy).r * float(u_metallic_sampler_active) + u_material.metallic * float(!u_metallic_sampler_active);
	float ao  = texture(ao_sampler, vert_data.tex_coord.xy).r * float(u_ao_sampler_active) + u_material.ao * float(!u_ao_sampler_active);
    vec4 albedo = CalculateAlbedoAndEmissive(vert_data.tex_coord.xy);

	total_light += CalculateDirectLightContribution(v, f0, vert_data.position.xyz, n, roughness, metallic, albedo.rgb);
	total_light += CalculateAmbientLightContribution(n_dot_v, f0, r, roughness, n, ao, metallic, albedo.rgb);

    vec4 colour = vec4(total_light, u_material.base_colour.a * albedo.w);

    float weight = clamp(pow(min(1.0, colour.a * 10.0) + 0.01, 3.0) * 1e8 * 
                         pow(1.0 - gl_FragCoord.z * 0.9, 3.0), 1e-2, 3e3);

    accum = vec4(colour.rgb * colour.a, colour.a) * weight;

    reveal = colour.a;
}