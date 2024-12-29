#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 1) uniform sampler2D albedo_sampler;
layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 4) uniform sampler2DArray spot_depth_sampler;
layout(binding = 5) uniform sampler3D voxel_mip_sampler;
layout(binding = 6) uniform sampler3D voxel_mip_sampler_cascade_1;
layout(binding = 7) uniform sampler2D normal_sampler;
layout(binding = 8) uniform usampler3D voxel_sampler_base_0;
layout(binding = 9) uniform usampler3D voxel_sampler_base_1;

layout(binding = 11) uniform sampler2D blue_noise_sampler;
layout(binding = 12) uniform usampler2D shader_id_sampler;
layout(binding = 16) uniform sampler2D view_depth_sampler;
layout(binding = 19) uniform sampler2D roughness_metallic_ao_sampler;
layout(binding = 20) uniform samplerCube diffuse_prefilter_sampler;
layout(binding = 21) uniform samplerCube specular_prefilter_sampler;
layout(binding = 22) uniform sampler2D brdf_lut_sampler;
layout(binding = 26) uniform samplerCubeArray pointlight_depth_sampler;
layout(binding = 27) uniform usampler3D voxel_grid_sampler;


layout(binding = 1, rgba16f) writeonly uniform image2D u_output_texture;

uniform vec3 u_aligned_camera_pos;
uniform vec3 u_aligned_camera_pos_c1;


#define DIR_DEPTH_SAMPLER dir_depth_sampler
#define POINTLIGHT_DEPTH_SAMPLER pointlight_depth_sampler
#define SPOTLIGHT_DEPTH_SAMPLER spot_depth_sampler
#define SPECULAR_PREFILTER_SAMPLER specular_prefilter_sampler
#define DIFFUSE_PREFILTER_SAMPLER diffuse_prefilter_sampler
#define BRDF_LUT_SAMPLER brdf_lut_sampler


ORNG_INCLUDE "LightingINCL.glsl"
ORNG_INCLUDE "VoxelCommonINCL.glsl"

ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy * 2);

uint shader_id = texelFetch(shader_id_sampler, tex_coords, 0).r;
vec3 sampled_world_pos = WorldPosFromDepth(texelFetch(view_depth_sampler, tex_coords, 0).r, (tex_coords / 2) / vec2(imageSize(u_output_texture) ));
vec3 sampled_normal = normalize(texelFetch(normal_sampler, tex_coords, 0).xyz);
vec3 sampled_albedo = texelFetch(albedo_sampler, tex_coords, 0).xyz;

vec3 roughness_metallic_ao = texelFetch(roughness_metallic_ao_sampler, tex_coords, 0).rgb;

const vec3 diffuse_cone_dirs[] =
{
    vec3(0.0f, 1.0f, 0.0f),
    vec3(0.0f, 0.5f, 0.866025f),
    vec3(0.823639f, 0.5f, 0.267617f),
    vec3(0.509037f, 0.5f, -0.7006629f),
    vec3(-0.50937f, 0.5f, -0.7006629f),
    vec3(-0.823639f, 0.5f, 0.267617f)
};

#define DIFFUSE_APERTURE_MAX PI / 3.0
#define DIFFUSE_APERTURE_MIN PI / 6.0

#define CONE_TRACE_SKELETON(x) 	vec3 step_pos = sampled_world_pos + cone_dir * d   ; \
		\
		vec3 coord = vec3(((step_pos - ubo_common.voxel_aligned_cam_positions[current_cascade].xyz + sampled_normal*0.15) / (VOXEL_SIZE * (current_cascade + 1)) + vec3(VOXEL_RES * 0.5)) / VOXEL_RES); \
		if (any(greaterThan(coord, vec3(1.0))) || any(lessThan(coord, vec3(0.0)))) { \
			current_cascade++; \
			break; \
		} \
		coord.z /= 6.0; \
		\
		float diam = 2.0 * tan_half_angle * (d);\
		vec4 mip_col = vec4(0);\
		x \
		vec4 voxel = mip_col;\
		\
		if (voxel.a > 0.0) {\
			float a = 1.0 - col.a;\
			col.rgb += a  * voxel.rgb;\
			col.a += a * voxel.a;\
		}\
		\
		d += diam * step_length * (1.0 - voxel.a);





vec4 ConeTrace(vec3 cone_dir, float aperture, float mip_scaling, out float dist_travelled) {
	cone_dir = normalize(cone_dir);
	vec4 col = vec4(0);

	const float tan_half_angle = tan(aperture * 0.5);
	const float tan_eighth_angle = tan(aperture * 0.125);
	float step_size_correction_factor = (1.0 + tan_eighth_angle) / (1.0 - tan_eighth_angle);
	float step_length = step_size_correction_factor * 0.2;

	float d = 0.15;

	vec3 weight = abs(cone_dir);
	uint current_cascade = 0;

	while (col.a < 0.95 && current_cascade == 0 ) {
		CONE_TRACE_SKELETON(
		float mip = min(log2(diam / VOXEL_SIZE ), 6.0);
		vec4 sample_col = textureLod(voxel_mip_sampler, coord + vec3(0, 0, (cone_dir.x < 0 ? 3.0 : 0.0) / 6.0), mip) * vec4(weight.x);
		sample_col += textureLod(voxel_mip_sampler, coord + vec3(0, 0, (cone_dir.y < 0 ? 4.0 : 1.0) / 6.0), mip) * vec4(weight.y);
		sample_col += textureLod(voxel_mip_sampler, coord + vec3(0, 0, (cone_dir.z < 0 ? 5.0 : 2.0) / 6.0), mip) * vec4(weight.z);
		mip_col += sample_col;
		float cam_dist = length(ubo_common.voxel_aligned_cam_positions[0].xyz - step_pos);

		// Interpolate between cascade borders
		// if (cam_dist > 20.f) {
		// 	float mip2 = clamp(log2(diam / VOXEL_SIZE ) - 1.0, 0.0, 4.0);

		// 	vec3 coord_2 =  vec3(((step_pos - ubo_common.voxel_aligned_cam_positions[1].xyz) / (VOXEL_SIZE * 2.0) + vec3(VOXEL_RES * 0.5)) / VOXEL_RES);
		// 	coord_2.z /= 6.0;
		// 	vec4 add = vec4(0);
		// 	float interp = 1.0 - (25.f - cam_dist) * 0.2f;

		// 	add += textureLod(voxel_mip_sampler_cascade_1, coord_2 + vec3(0, 0, (cone_dir.x < 0 ? 3.0 : 0.0) / 6.0), max(mip2, 0.0)) * weight.x * 0.5;
		// 	add += textureLod(voxel_mip_sampler_cascade_1, coord_2 + vec3(0, 0, (cone_dir.y < 0 ? 4.0 : 1.0) / 6.0), max(mip2, 0.0)) * weight.y * 0.5;
		// 	add += textureLod(voxel_mip_sampler_cascade_1, coord_2 + vec3(0, 0, (cone_dir.z < 0 ? 5.0 : 2.0) / 6.0), max(mip2, 0.0)) * weight.z * 0.5;
		// 	mip_col = mix(mip_col, add, interp);
		// }
		)

	}
	dist_travelled = d;

	// while (col.a < 0.95 && current_cascade == 1) {
	// 	CONE_TRACE_SKELETON(
	// 	float mip = clamp(log2(diam / VOXEL_SIZE ) - 1.0, 0.0, 4.0);

	// 	mip_col += textureLod(voxel_mip_sampler_cascade_1, coord + vec3(0, 0, (cone_dir.x < 0 ? 3.0 : 0.0) / 6.0), max(mip, 0.0)) * weight.x * 0.5;
	// 	mip_col += textureLod(voxel_mip_sampler_cascade_1, coord + vec3(0, 0, (cone_dir.y < 0 ? 4.0 : 1.0) / 6.0), max(mip, 0.0)) * weight.y * 0.5;
	// 	mip_col += textureLod(voxel_mip_sampler_cascade_1, coord + vec3(0, 0, (cone_dir.z < 0 ? 5.0 : 2.0) / 6.0), max(mip, 0.0)) * weight.z * 0.5;

	// 	)

	// 	if (any(greaterThan(coord, vec3(0.8)))) {
	// 		//mip_col *= (1.0 - max(max(coord.x, coord.y), coord.z)) * 5.0;
	// 		//mip_col = vec4(10, 0, 0, 10);
	// 	}

	// }

	return col;
}
	
vec4 CalculateIndirectDiffuseLighting(vec3 v, vec3 f0, out float o_occlusion) {
	vec4 col = vec4(0);

    vec3 avg_normal = sampled_normal;
    
	vec3 non_parallel = abs(dot(avg_normal, vec3(0, 1, 0))) > 0.99999 ? vec3(0, 0, 1) : vec3(0, 1, 0);
	vec3 right = normalize(non_parallel - dot(avg_normal, non_parallel) * avg_normal);
	vec3 up = cross(right, avg_normal);

	float aperture = mix(DIFFUSE_APERTURE_MIN, DIFFUSE_APERTURE_MAX, roughness_metallic_ao.r);

	float occlusion = 0.f;

	for (int i = 0; i < 6; i++) {
		vec3 cone_dir = normalize(avg_normal + diffuse_cone_dirs[i].x * right + diffuse_cone_dirs[i].z * up);
		cone_dir = mix(normalize(reflect(-v, avg_normal)), cone_dir, roughness_metallic_ao.r);
		float dist;
		col += ConeTrace(cone_dir, aperture, 0.2, dist) * vec4(CookTorranceBRDF(cone_dir, v, f0, sampled_world_pos, avg_normal.rgb, 
		roughness_metallic_ao.r, roughness_metallic_ao.g, sampled_albedo), 1);

		occlusion += (1.0 - min(dist, 25.5) / 25.5);
	}
	o_occlusion = occlusion / 6.0;
	return col;
}


void main() {
    if (shader_id != 1)
    return;

	vec3 v = normalize(ubo_common.camera_pos.xyz - sampled_world_pos);
	vec3 r = reflect(-v, sampled_normal);
	float n_dot_v = max(dot(sampled_normal, v), 0.0);

	//reflection amount
	vec3 f0 = vec3(0.04); // TODO: Support different values for more metallic objects
	f0 = mix(f0, sampled_albedo.xyz, roughness_metallic_ao.g);

	float occlusion;
	vec4 res = CalculateIndirectDiffuseLighting(v, f0, occlusion);

    vec3 light = res.rgb + 0*(1.0 - occlusion) * CalculateAmbientLightContribution(n_dot_v, f0, r, roughness_metallic_ao.r, sampled_normal.xyz, roughness_metallic_ao.b, roughness_metallic_ao.g, sampled_albedo.rgb);
	imageStore(u_output_texture, tex_coords / 2, vec4(light, res.a) );
}
