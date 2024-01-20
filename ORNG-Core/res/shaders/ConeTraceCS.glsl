#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 1) uniform sampler2D albedo_sampler;
layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 4) uniform sampler2DArray spot_depth_sampler;
layout(binding = 5) uniform sampler3D pos_x_voxel_mip_sampler;
layout(binding = 6) uniform sampler3D pos_y_voxel_mip_sampler;
layout(binding = 7) uniform sampler2D normal_sampler;
layout(binding = 8) uniform sampler3D pos_z_voxel_mip_sampler;
layout(binding = 9) uniform sampler3D neg_x_voxel_mip_sampler;
layout(binding = 10) uniform sampler3D neg_y_voxel_mip_sampler;
layout(binding = 11) uniform sampler2D blue_noise_sampler;
layout(binding = 12) uniform usampler2D shader_id_sampler;
layout(binding = 13) uniform sampler3D neg_z_voxel_mip_sampler;
layout(binding = 16) uniform sampler2D view_depth_sampler;
layout(binding = 19) uniform sampler2D roughness_metallic_ao_sampler;
layout(binding = 20) uniform samplerCube diffuse_prefilter_sampler;
layout(binding = 21) uniform samplerCube specular_prefilter_sampler;
layout(binding = 22) uniform sampler2D brdf_lut_sampler;
layout(binding = 26) uniform samplerCubeArray pointlight_depth_sampler;
layout(binding = 27) uniform usampler3D voxel_grid_sampler;


layout(binding = 1, rgba16f) writeonly uniform image2D u_output_texture;

uniform vec3 u_aligned_camera_pos;


#define DIR_DEPTH_SAMPLER dir_depth_sampler
#define POINTLIGHT_DEPTH_SAMPLER pointlight_depth_sampler
#define SPOTLIGHT_DEPTH_SAMPLER spot_depth_sampler
#define SPECULAR_PREFILTER_SAMPLER specular_prefilter_sampler
#define DIFFUSE_PREFILTER_SAMPLER diffuse_prefilter_sampler
#define BRDF_LUT_SAMPLER brdf_lut_sampler


ORNG_INCLUDE "LightingINCL.glsl"

ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy * 2);

unsigned int shader_id = texelFetch(shader_id_sampler, tex_coords, 0).r;
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


vec3 ConeTrace(vec3 cone_dir, float aperture, float mip_scaling, float step_modifier) {
	cone_dir = normalize(cone_dir);
	vec4 col = vec4(0);

	const float tan_half_angle = tan(aperture * 0.5);
	const float tan_eighth_angle = tan(aperture * 0.125);
	float step_size_correction_factor = (1.0 + tan_eighth_angle) / (1.0 - tan_eighth_angle);
	float step_length = step_size_correction_factor * 0.3 ; // increasing makes diffuse softer

	float d = step_length ;

	vec3 weight = cone_dir * cone_dir;

	while (col.a < 0.95 ) {
		vec3 step_pos = sampled_world_pos + cone_dir * d  ;

		vec3 coord = ((step_pos - u_aligned_camera_pos ) * 5.0 + vec3(128)) / (256.0);
		if (any(greaterThan(coord, vec3(1))) || any(lessThan(coord, vec3(0))) )
		break;

		float diam = 2.0 * tan_half_angle * d;
		float mip = clamp(log2(diam / mip_scaling ), 0.0, 5.0);


		vec4 mip_col = vec4(0);
		float adjusted_mip = mip;
		mip_col += (cone_dir.x < 0 ? textureLod(neg_x_voxel_mip_sampler, coord, adjusted_mip) : textureLod(pos_x_voxel_mip_sampler, coord, adjusted_mip)) * weight.x;
		mip_col += (cone_dir.y < 0 ? textureLod(neg_y_voxel_mip_sampler, coord, adjusted_mip) : textureLod(pos_y_voxel_mip_sampler, coord, adjusted_mip)) * weight.y;
		mip_col += (cone_dir.z < 0 ? textureLod(neg_z_voxel_mip_sampler, coord, adjusted_mip) : textureLod(pos_z_voxel_mip_sampler, coord, adjusted_mip)) * weight.z;

		if (mip < 1.0) {
			//mip_col = mix(convRGBA8ToVec4(texture(voxel_grid_sampler, coord).r) / 255., mip_col, clamp(mip, 0, 1));
		}


		vec4 voxel = mip_col;
		
		if (voxel.a > 0.0) {
			float a = 1.0 - col.a;
			col.rgb += a  * voxel.rgb;
			col.a += a * voxel.a;
		}

		d += diam * step_modifier  ;
	}

	return col.rgb;
	//return textureLod(voxel_grid_sampler, vec3(((sampled_world_pos - u_aligned_camera_pos ) * 5.0 + vec3(128)) / 256.0), 1).xyz;
}
	
vec3 CalculateIndirectDiffuseLighting() {
	vec3 col = vec3(0);

    vec3 avg_normal = normalize(texelFetch(normal_sampler, tex_coords, 0 ).xyz);
    
	vec3 non_parallel = abs(dot(avg_normal, vec3(0, 1, 0))) > 0.99999 ? vec3(0, 0, 1) : vec3(0, 1, 0);
	vec3 right = normalize(non_parallel - dot(avg_normal, non_parallel) * avg_normal);
	vec3 up = cross(right, avg_normal);


	float aperture = max(DIFFUSE_APERTURE_MAX, DIFFUSE_APERTURE_MIN) ;
	for (int i = 0; i < 6; i++) {
		vec3 cone_dir = normalize(avg_normal + diffuse_cone_dirs[i].x * right + diffuse_cone_dirs[i].z * up);
		col += ConeTrace(cone_dir, aperture, 0.15,0.5);
	}
	col *= (1.0 - roughness_metallic_ao.g);
	col /= 6.0;



	//col += ConeTrace(normalize(reflect(-(ubo_common.camera_pos.xyz - sampled_world_pos), avg_normal)), clamp(roughness_metallic_ao.r * PI * 0.5 , 0.3, PI ), 0.3, 0.4) * (1.0 - roughness_metallic_ao.r) ;
	return col ;
}


void main() {
    if (shader_id != 1)
    return;

    vec3 light = CalculateIndirectDiffuseLighting() * sampled_albedo.xyz ;
	imageStore(u_output_texture, tex_coords / 2, vec4(light, 1.0));
}
