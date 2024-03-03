#version 460 core

#ifndef PREV_CASCADE_WORLD_SIZE
#error PREV_CASCADE_WORLD_SIZE must be defined
#endif

#ifndef CURRENT_CASCADE_TEX_SIZE
#error CURRENT_CASCADE_TEX_SIZE must be defined
#endif

#ifndef VOXEL_SIZE
#error VOXEL_SIZE must be defined
#endif


layout(binding = 1) uniform sampler2D diffuse_sampler;
layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 4) uniform sampler2DArray spot_depth_sampler;
layout(binding = 7) uniform sampler2D normal_sampler;
layout(binding = 19) uniform sampler2D roughness_metallic_ao_sampler;
layout(binding = 25) uniform sampler2D emissive_sampler;
layout(binding = 26) uniform samplerCubeArray pointlight_depth_sampler;


layout(binding = 0, r32ui) uniform coherent uimage3D voxel_image;
layout(binding = 1, r32ui) uniform coherent uimage3D voxel_image_normals;


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
ORNG_INCLUDE "BuffersINCL.glsl"

#define DIR_DEPTH_SAMPLER dir_depth_sampler
#define POINTLIGHT_DEPTH_SAMPLER pointlight_depth_sampler
#define SPOTLIGHT_DEPTH_SAMPLER spot_depth_sampler
ORNG_INCLUDE "ShadowsINCL.glsl"

uniform uint u_shader_id;
uniform bool u_emissive_sampler_active;
uniform Material u_material;

uniform vec3 u_aligned_camera_pos;


vec3 CalculatePointlight(PointLight light) {
    vec3 dir = light.pos.xyz - vert_data.position.xyz;
    const float l = length(dir);
    dir = normalize(dir);

    float attenuation = light.constant +
		light.a_linear * l +
		light.exp * pow(l, 2);

    const float r = max(dot(normalize(vert_data.normal), dir), 0.f);
    return r / attenuation * light.color.xyz;
}

vec3 CalculateSpotlight(SpotLight light) {
    vec3 dir = light.pos.xyz - vert_data.position.xyz;
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
    const float r = max(dot(normalize(vert_data.normal), dir), 0.f);

    return r * spotlight_intensity / attenuation * light.color.xyz;
}

vec4 CalculateAlbedoAndEmissive(vec2 tex_coord) {
	vec4 sampled_albedo = texture(diffuse_sampler, tex_coord.xy);
	vec4 albedo_col = vec4(sampled_albedo.rgb * u_material.base_color.rgb, sampled_albedo.a);
	albedo_col *= bool(u_material.flags & MAT_FLAG_EMISSIVE) ? vec4(vec3(u_material.emissive_strength), 1.0) : vec4(1.0);
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

// Below functions sourced from paper https://www.icare3d.org/research/OpenGLInsights-SparseVoxelization.pdf
uint convVec4ToRGBA8(vec4 val) {
  return (uint(val.w) & 0x000000FF) << 24U
    | (uint(val.z) & 0x000000FF) << 16U
    | (uint(val.y) & 0x000000FF) << 8U
    | (uint(val.x) & 0x000000FF);
}




vec4 convRGBA8ToVec4(uint val) {
  return vec4(float((val & 0x000000FF)),
      float((val & 0x0000FF00) >> 8U),
      float((val & 0x00FF0000) >> 16U),
      float((val & 0xFF000000) >> 24U));
}


bool PointInsideAABB(vec3 extents, vec3 p) {
    return (p.x < extents.x && p.x > -extents.x && p.y < extents.y && p.y > -extents.y && p.z < extents.z && p.z > -extents.z);
}

void main() {
    ivec3 coord = ivec3((vert_data.position.xyz - u_aligned_camera_pos) / VOXEL_SIZE + CURRENT_CASCADE_TEX_SIZE * 0.5);

    if (any(greaterThan(coord, vec3(CURRENT_CASCADE_TEX_SIZE))) || any(lessThan(coord, vec3(0))))
        discard;

    vec3 n = normalize(vert_data.normal);

    vec3 col = vec3(0);
    if (bool(u_material.flags & MAT_FLAG_EMISSIVE)) {
        col = CalculateAlbedoAndEmissive(vert_data.tex_coord.xy).rgb;
    } else {
        for (int i = 0; i < ubo_point_lights.lights.length(); i++) {
            col += CalculatePointlight(ubo_point_lights.lights[i]) * (1.0 - ShadowCalculationPointlight(ubo_point_lights.lights[i], i, vert_data.position.xyz));
        }
        col += ubo_global_lighting.directional_light.color.xyz * max(dot(ubo_global_lighting.directional_light.direction.xyz, n), 0.0) * (1.0 - CheapShadowCalculationDirectional(vert_data.position.xyz));
        col *= CalculateAlbedoAndEmissive(vert_data.tex_coord.xy).rgb;
    }
    imageAtomicMax(voxel_image_normals, coord, convVec4ToRGBA8(vec4(n * 127 + 127, 255)));

    imageAtomicMax(voxel_image, coord, convVec4ToRGBA8(vec4(clamp(col, vec3(0), vec3(1)) * 255, 255)));

    discard;
}