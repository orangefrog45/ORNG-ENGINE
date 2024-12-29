#version 460 core
// This shader takes the normal triangle geometry from the scene and voxelizes it, storing the luminance results and normals of the geometry in two 3D textures
// Uses GbufferVS.glsl as vertex shader

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

in flat mat4 vs_transform;

ORNG_INCLUDE "CommonINCL.glsl"
ORNG_INCLUDE "ParticleBuffersINCL.glsl"
ORNG_INCLUDE "BuffersINCL.glsl"
ORNG_INCLUDE "VoxelCommonINCL.glsl"

#define DIR_DEPTH_SAMPLER dir_depth_sampler
#define POINTLIGHT_DEPTH_SAMPLER pointlight_depth_sampler
#define SPOTLIGHT_DEPTH_SAMPLER spot_depth_sampler
ORNG_INCLUDE "ShadowsINCL.glsl"

uniform uint u_shader_id;
uniform bool u_emissive_sampler_active;
uniform Material u_material;

uniform float u_voxel_size;
uniform uint u_cascade_idx;



vec3 CalculatePointlight(PointLight light) {
    vec3 dir = light.pos.xyz - vert_data.position.xyz;
    const float l = length(dir);
    dir = normalize(dir);

    float attenuation = light.constant +
		light.a_linear * l +
		light.exp * pow(l, 2);

    const float r = max(dot(normalize(vert_data.normal), dir), 0.f);
    return r / attenuation * light.colour.xyz;
}

vec3 CalculateSpotlight(SpotLight light) {
    vec3 dir = light.pos.xyz - vert_data.position.xyz;
	float spot_factor = dot(normalize(dir), -light.dir.xyz);

    vec3 colour = vec3(0);

	if (spot_factor < 0.0001 || spot_factor < light.aperture)
		return colour;

    const float l = length(dir);
    dir = normalize(dir);

    float attenuation = light.constant +
		light.a_linear * l +
		light.exp * pow(l, 2);

	float spotlight_intensity = (1.0 - (1.0 - spot_factor) / max((1.0 - light.aperture), 1e-5));
    const float r = max(dot(normalize(vert_data.normal), dir), 0.f);

    return r * spotlight_intensity / attenuation * light.colour.xyz;
}

vec4 CalculateAlbedoAndEmissive(vec2 tex_coord) {
	vec4 sampled_albedo = texture(diffuse_sampler, tex_coord.xy);
	vec4 albedo_col = vec4(sampled_albedo.rgb * u_material.base_colour.rgb, sampled_albedo.a);
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

void main() {
    uint current_cascade_tex_size = imageSize(voxel_image).x;

    ivec3 coord = ivec3((vert_data.position.xyz - ubo_common.voxel_aligned_cam_positions[u_cascade_idx].xyz) / u_voxel_size + current_cascade_tex_size * 0.5);

    if (any(greaterThan(coord, vec3(current_cascade_tex_size))) || any(lessThan(coord, vec3(0))))
        discard;

    vec3 n = normalize(vert_data.normal);
    vec3 col = vec3(0);

    // Calculate luminance value
    if (bool(u_material.flags & MAT_FLAG_EMISSIVE)) {
        col = CalculateAlbedoAndEmissive(vert_data.tex_coord.xy).rgb;
    } else {
        for (int i = 0; i < ubo_point_lights.lights.length(); i++) {
            col += CalculatePointlight(ubo_point_lights.lights[i]) * (1.0 - ShadowCalculationPointlight(ubo_point_lights.lights[i], i, vert_data.position.xyz));
        }
        col += ubo_global_lighting.directional_light.colour.xyz * max(dot(ubo_global_lighting.directional_light.direction.xyz, n), 0.0) * (1.0 - CheapShadowCalculationDirectional(vert_data.position.xyz));
        col *= CalculateAlbedoAndEmissive(vert_data.tex_coord.xy).rgb;
    }

    // Blend accumulated voxel luminance from previous frames (previous value has already been decremented to 95% of its value by a previous pass)
    vec4 original = convRGBA8ToVec4(imageLoad(voxel_image, coord).r);
    original.rgb /= 255.0; // Scale down to 0-1
    original.rgb *= original.a * 0.1; // Apply emissive
    col.rgb = (original.rgb + col.rgb * 0.05) ;

    // Create an emissive component to brighten colours if needed for HDR
    float l = length(col.rgb);
    
    float emissive = clamp(l, 1.0, 25.5);
    col.rgb /= emissive;

    // Use AtomicMax to prevent flickering from multiple threads trying to write into one voxel
    uint col_packed = convVec4ToRGBA8(vec4(col * 255, emissive * 10.0));

    if (imageAtomicMax(voxel_image, coord, col_packed) < col_packed) {
        // Set successfully, set normal too
        imageAtomicExchange(voxel_image_normals, coord, convVec4ToRGBA8(vec4(n * 127 + 127, 255)));
    }

    discard;
}