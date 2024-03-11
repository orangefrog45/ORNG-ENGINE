#version 430 core

ORNG_INCLUDE "UtilINCL.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

const unsigned int NUM_SHADOW_CASCADES = 3;
ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy);


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

#define DIR_DEPTH_SAMPLER dir_depth_sampler
#define POINTLIGHT_DEPTH_SAMPLER pointlight_depth_sampler
#define SPOTLIGHT_DEPTH_SAMPLER spot_depth_sampler
#define SPECULAR_PREFILTER_SAMPLER specular_prefilter_sampler
#define DIFFUSE_PREFILTER_SAMPLER diffuse_prefilter_sampler
#define BRDF_LUT_SAMPLER brdf_lut_sampler

uniform vec3 u_aligned_camera_pos;

ORNG_INCLUDE "LightingINCL.glsl"

unsigned int shader_id = texelFetch(shader_id_sampler, tex_coords, 0).r;
vec3 sampled_world_pos = WorldPosFromDepth(texelFetch(view_depth_sampler, tex_coords, 0).r, tex_coords / vec2(imageSize(u_output_texture)));
vec3 sampled_normal = normalize(texelFetch(normal_sampler, tex_coords, 0).xyz);
vec3 sampled_albedo = texelFetch(albedo_sampler, tex_coords, 0).xyz;

vec3 roughness_metallic_ao = texelFetch(roughness_metallic_ao_sampler, tex_coords, 0).rgb;
float roughness = roughness_metallic_ao.r;
float metallic = roughness_metallic_ao.g;
float ao = roughness_metallic_ao.b;


vec4 convRGBA8ToVec4(uint val) {
  return vec4(float((val & 0x000000FF)),
      float((val & 0x0000FF00) >> 8U),
      float((val & 0x00FF0000) >> 16U),
      float((val & 0xFF000000) >> 24U));
}


vec3 ConeTrace(vec3 cone_dir, float aperture, float mip_scaling, float step_modifier) {
	cone_dir = normalize(cone_dir);
	vec4 col = vec4(0);

	const float tan_half_angle = tan(aperture * 0.5);
	const float tan_eighth_angle = tan(aperture * 0.125);
	float step_size_correction_factor = (1.0 + tan_eighth_angle) / (1.0 - tan_eighth_angle);
	float step_length = step_size_correction_factor * 0.3 ; // increasing makes diffuse softer

	float d = step_length ;

	vec3 weight = cone_dir * cone_dir;
	while (col.a < 0.95) {
		vec3 step_pos = sampled_world_pos + cone_dir * d + sampled_normal * 0.2 ;

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

const vec3 diffuseConeDirections[] =
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

	
vec3 CalculateIndirectDiffuseLighting() {
	vec3 col = vec3(0);

	vec3 non_parallel = abs(dot(sampled_normal, vec3(0, 1, 0))) > 0.99999 ? vec3(0, 0, 1) : vec3(0, 1, 0);
	vec3 right = normalize(non_parallel - dot(sampled_normal, non_parallel) * sampled_normal);
	vec3 up = cross(right, sampled_normal);

	float aperture = max(DIFFUSE_APERTURE_MAX, DIFFUSE_APERTURE_MIN) ;
	for (int i = 0; i < 6; i++) {
		vec3 cone_dir = normalize(sampled_normal + diffuseConeDirections[i].x * right + diffuseConeDirections[i].z * up);
		col += ConeTrace(cone_dir, aperture, 0.15, 1.0);
	}
	col *= (1.0 - roughness_metallic_ao.g);
	col /= 6.0;

	col += ConeTrace(normalize(reflect(-(ubo_common.camera_pos.xyz - sampled_world_pos), sampled_normal)), clamp(roughness_metallic_ao.r * PI * 0.5 , 0.5, PI ), 0.6, 0.25) * (1.0 - roughness_metallic_ao.r) ;
	return col ;
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
	vec3 f0 = vec3(0.04); // TODO: Support different values for more metallic objects
	f0 = mix(f0, sampled_albedo.xyz, metallic);

	total_light += CalculateDirectLightContribution(v, f0, sampled_world_pos.xyz, sampled_normal.xyz, roughness, metallic, sampled_albedo.rgb);
	total_light += CalculateAmbientLightContribution(n_dot_v, f0, r, roughness, sampled_normal.xyz, ao, metallic, sampled_albedo.rgb);
	//total_light += CalculateIndirectDiffuseLighting() * sampled_albedo.xyz * 0.25 ;

	vec3 light_color = max(vec3(total_light), vec3(0.0, 0.0, 0.0));

	imageStore(u_output_texture, tex_coords, vec4(light_color, 1.0));
};